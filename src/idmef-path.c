/*****
*
* Copyright (C) 2002-2017 CS-SI. All Rights Reserved.
* Author: Krzysztof Zaraska
*         Yoann Vandoorselaere <yoann@prelude-ids.com>
*
* This file is part of the Prelude library.
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2, or (at your option)
* any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License along
* with this program; if not, write to the Free Software Foundation, Inc.,
* 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*
*****/

#include "libmissing.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <stdarg.h>
#include <limits.h>
#include <assert.h>

#include "glthread/lock.h"

#include "prelude-hash.h"
#include "prelude-log.h"
#include "prelude-inttypes.h"
#include "prelude-string.h"

#define PRELUDE_ERROR_SOURCE_DEFAULT PRELUDE_ERROR_SOURCE_IDMEF_PATH
#include "prelude-error.h"

#include "idmef-time.h"
#include "idmef-data.h"
#include "idmef-class.h"
#include "idmef-value.h"

#include "idmef-object-prv.h"
#include "idmef-tree-wrap.h"
#include "idmef-path.h"
#include "prelude-string.h"
#include "prelude-linked-object.h"
#include "common.h"


#define MAX_DEPTH                16
#define MAX_NAME_LEN            128
#define HASH_DEFAULT_SIZE       128

#define INDEX_UNDEFINED INT_MIN
#define INDEX_KEY       (INT_MIN + 1)
#define INDEX_FORBIDDEN (INT_MIN + 2)


typedef struct idmef_key_listed_object {
        IDMEF_OBJECT;
        prelude_list_t list;
        prelude_string_t *objkey;
} idmef_key_listed_object_t;


typedef struct idmef_path_element {

        int index;
        char *index_key;

        idmef_class_id_t class;
        idmef_class_child_id_t position;
        idmef_value_type_id_t value_type;

} idmef_path_element_t;


struct idmef_path {

        gl_lock_t mutex;
        char name[MAX_NAME_LEN];
        int refcount;
        unsigned int depth;
        idmef_class_id_t top_class;

        idmef_path_element_t elem[MAX_DEPTH];
};



static int _idmef_path_set(const idmef_path_t *path, idmef_class_id_t class, size_t i,
                           int index_override, void *ptr, idmef_value_t *value);


static prelude_bool_t flush_cache = FALSE;
static prelude_hash_t *cached_path = NULL;
static gl_lock_t cached_path_mutex = gl_lock_initializer;



static void path_lock_cb(void *data)
{
        idmef_path_t *path = data;
        gl_lock_lock(path->mutex);
}


static void path_reinit_cb(void *data)
{
        idmef_path_t *path = data;
        gl_lock_init(path->mutex);
}


static void path_unlock_cb(void *data)
{
        idmef_path_t *path = data;
        gl_lock_unlock(path->mutex);
}



static void flush_cache_if_wanted(void *ptr)
{
        if ( flush_cache )
                idmef_path_destroy(ptr);
}



static unsigned int path_hash_func(const void *path)
{
        const char *ptr = ((const idmef_path_t *) path)->name;
        unsigned int hv = *ptr;

        if ( hv )
                for ( ptr += 1; *ptr; ptr++ )
                        hv = (hv << 5) - hv + *ptr;

        return hv;

}


static int path_key_cmp_func(const void *p1, const void *p2)
{
        const idmef_path_t *path1 = p1;
        const idmef_path_t *path2 = p2;

        if ( path1->top_class != path2->top_class )
                return -1;

        return strcmp(path1->name, path2->name);
}



static int initialize_path_cache_if_needed(void)
{
        if ( cached_path )
                return 0;

        return prelude_hash_new2(&cached_path, HASH_DEFAULT_SIZE, path_hash_func, path_key_cmp_func, NULL, flush_cache_if_wanted);
}



static int build_name(idmef_path_t *path)
{
        unsigned int i;
        const char *name;
        idmef_class_id_t class;

        /*
         * we don't need gl_lock_{,un}lock since the path has no name
         * it means that it is not in the cache and thus, not shared
         */
        path->name[0] = '\0';
        class = IDMEF_CLASS_ID_MESSAGE;

        for ( i = 0; i < path->depth; i++ ) {

                if ( i > 0 )
                        strncat(path->name, ".", sizeof(path->name) - strlen(path->name));

                name = idmef_path_get_name(path, i);
                if ( ! name )
                        return prelude_error(PRELUDE_ERROR_IDMEF_PATH_INTEGRITY);

                strncat(path->name, name, sizeof(path->name) - strlen(path->name));

                if ( path->elem[i].index != INDEX_UNDEFINED && path->elem[i].index != INDEX_FORBIDDEN ) {
                        strncat(path->name, "(", sizeof(path->name) - strlen(path->name));

                        if ( path->elem[i].index != INDEX_KEY )
                                snprintf(path->name + strlen(path->name), sizeof(path->name) - strlen(path->name), "%d", path->elem[i].index);

                        else if ( path->elem[i].index_key )
                                strncat(path->name, path->elem[i].index_key, sizeof(path->name) - strlen(path->name));

                        strncat(path->name, ")", sizeof(path->name) - strlen(path->name));
                }

                class = idmef_class_get_child_class(class, path->elem[i].position);
                if ( class < 0 && i < path->depth - 1 )
                        return prelude_error(PRELUDE_ERROR_IDMEF_PATH_INTEGRITY);
        }

        return 0;
}




static int idmef_path_get_internal(idmef_value_t **ret, const idmef_path_t *path,
                                   unsigned int depth, void *parent, idmef_class_id_t parent_class);


static inline prelude_bool_t has_index_key(prelude_list_t *elem, const char *index_key)
{
        idmef_key_listed_object_t *obj = prelude_list_entry(elem, idmef_key_listed_object_t, list);
        return (obj->objkey && strcmp(prelude_string_get_string_or_default(obj->objkey, ""), index_key) == 0) ? TRUE : FALSE;
}


static int set_index_key(const idmef_path_element_t *elem, void *ptr)
{
        idmef_key_listed_object_t *obj = ptr;

        if ( ! elem->index_key )
                return 0;

        if ( obj->objkey )
                prelude_string_destroy(obj->objkey);

        return prelude_string_new_dup(&obj->objkey, elem->index_key);
}



static int get_internal(idmef_value_t **value, const idmef_path_t *path, int depth, void *parent, idmef_class_id_t parent_class)
{
        int ret;
        idmef_value_type_t vt;
        idmef_value_type_id_t type;

        if ( parent_class >= 0 )
                return idmef_path_get_internal(value, path, depth, parent, parent_class);

        type = path->elem[depth - 1].value_type;

        ret = idmef_value_new(value, type, parent);
        if ( ret < 0 )
                return ret;

        vt.id = type;
        vt.data.string_val = (void *) parent;

        ret = idmef_value_type_ref(&vt);
        if ( ret < 0 )
                idmef_value_destroy(*value);

        return 1;
}



static int idmef_path_get_list_internal(idmef_value_t **value_list,
                                        const idmef_path_t *path, int depth,
                                        prelude_list_t *list, idmef_class_id_t parent_class)
{
        int ret;
        prelude_list_t *tmp;
        idmef_value_t *value;
        unsigned int cnt = 0;

        ret = idmef_value_new_list(value_list);
        if ( ret < 0 )
                return ret;

        prelude_list_for_each(list, tmp) {
                value = NULL;

                if ( path->elem[depth - 1].index_key && ! has_index_key(tmp, path->elem[depth - 1].index_key) )
                        continue;

                ret = get_internal(&value, path, depth, idmef_linked_object_get_object(tmp), parent_class);
                if ( ret < 0 ) {
                        idmef_value_destroy(*value_list);
                        return ret;
                }

                ret = idmef_value_list_add(*value_list, value);
                if ( ret < 0 ) {
                        idmef_value_destroy(*value_list);
                        return ret;
                }

                cnt++;
        }

        if ( ! cnt ) {
                idmef_value_destroy(*value_list);
                *value_list = NULL;
        }

        return cnt;
}


static prelude_list_t *list_get_nth(prelude_list_t *head, int which)
{
        int cnt = 0;
        prelude_list_t *tmp;

        if ( which >= 0 ) {
                prelude_list_for_each(head, tmp) {
                        if ( cnt++ == which )
                                return tmp;
                }
        }

        else {
                prelude_list_for_each_reversed(head, tmp) {
                        if ( --cnt == which )
                                return tmp;
                }
        }

        return NULL;
}



static int idmef_path_get_nth_internal(idmef_value_t **value, const idmef_path_t *path,
                                       unsigned int depth, prelude_list_t *list,
                                       idmef_class_id_t parent_class, int which, const char *index_key)
{
        prelude_list_t *tmp;

        tmp = list_get_nth(list, which);
        if ( ! tmp ) {
                *value = NULL;
                return 0;
        }

        return get_internal(value, path, depth, idmef_linked_object_get_object(tmp), parent_class);
}



static int idmef_path_get_internal(idmef_value_t **value, const idmef_path_t *path,
                                   unsigned int depth, void *parent, idmef_class_id_t parent_class)
{
        void *child;
        int ret, which;
        idmef_class_id_t child_class;
        idmef_class_child_id_t child_id;

        if ( depth < path->depth ) {

                child_id = path->elem[depth].position;

                ret = idmef_class_get_child(parent, parent_class, child_id, &child);
                if ( ret < 0 )
                        return ret;

                if ( ! child ) {
                        *value = NULL;
                        return 0;
                }

                child_class = idmef_class_get_child_class(parent_class, child_id);
                which = path->elem[depth].index;

                if ( which == INDEX_FORBIDDEN )
                        return idmef_path_get_internal(value, path, depth + 1, child, child_class);

                if ( which == INDEX_UNDEFINED || which == INDEX_KEY || which == IDMEF_LIST_APPEND || which == IDMEF_LIST_PREPEND )
                        return idmef_path_get_list_internal(value, path, depth + 1, child, child_class);

                return idmef_path_get_nth_internal(value, path, depth + 1, child, child_class, which, path->elem[depth].index_key);
        }

        if ( parent_class < 0 || (path->depth > 0 && path->elem[path->depth - 1].value_type == IDMEF_VALUE_TYPE_ENUM) ) {
                *value = parent;
                return 1;
        }

        ret = (ret = idmef_value_new_class(value, parent_class, parent) < 0) ? ret : 1;
        if ( ret == 1 )
                idmef_class_ref(parent_class, parent);

        return ret;
}


static void delete_listed_child(void *parent, idmef_class_id_t class, const idmef_path_element_t *elem)
{
        int ret;
        void *obj;
        prelude_list_t *head, *tmp, *bkp;

        ret = idmef_class_get_child(parent, class, elem->position, (void *) &head);
        if ( ret < 0 )
                return;

        prelude_list_for_each_safe(head, tmp, bkp) {
                obj = idmef_linked_object_get_object(tmp);

                if ( elem->index_key && ! has_index_key(obj, elem->index_key) )
                        continue;

                /*
                 * The object might be referenced from other place than
                 * this message, in which case idmef_class_destroy()
                 * will only decrease it's reference count.
                 *
                 * We manually call prelude_list_del_init() in order to
                 * disassociate the object from the message.
                 */
                prelude_list_del_init(tmp);
                idmef_class_destroy(idmef_class_get_child_class(class, elem->position), obj);
        }
}



static int _idmef_path_set_undefined_not_last(const idmef_path_t *path, const idmef_path_element_t *elem,
                                              idmef_class_id_t class, size_t i, void *ptr, idmef_value_t *value)
{
        int j = 0, ret;
        idmef_value_t *val;
        prelude_list_t *head, *tmp, *bkp;

        assert(idmef_class_is_child_list(class, elem->position));

        ret = idmef_class_get_child(ptr, class, elem->position, (void *) &head);
        if ( ret < 0 )
                return ret;

        prelude_list_for_each_safe(head, tmp, bkp) {
                if ( elem->index_key && ! has_index_key(tmp, elem->index_key) )
                        continue;

                if ( value && idmef_value_is_list(value) ) {
                        ret = idmef_value_get_nth2(value, j++, &val);
                        if ( ret <= 0 )
                                return ret;
                } else {
                        j = 1;
                        val = value;
                }

                ret = _idmef_path_set(path, elem->class, i + 1, 0, idmef_linked_object_get_object(tmp), val);
                if ( ret < 0 )
                        return ret;
        }

        for ( ; value && j < idmef_value_get_count(value); j++ ) {
                ret = _idmef_path_set(path, class, i, IDMEF_LIST_APPEND, ptr, idmef_value_get_nth(value, j));
                if ( ret < 0 )
                        return ret;

                ret = set_index_key(elem, idmef_linked_object_get_object(head->prev));
                if ( ret < 0 )
                        return ret;
        }

        return ret;
}



static void ad_magic_set_datatype(idmef_additional_data_t *data, idmef_value_type_id_t vtype)
{
        idmef_additional_data_type_t type;

        /*
         * In case the user has already set the type member, do nothing.
         */
        if ( _idmef_additional_data_type_is_set(data) != 0 )
                return;

        if ( vtype >= IDMEF_VALUE_TYPE_INT8 && vtype <= IDMEF_VALUE_TYPE_UINT64 )
                type = IDMEF_ADDITIONAL_DATA_TYPE_INTEGER;

        else if ( vtype == IDMEF_VALUE_TYPE_FLOAT || vtype == IDMEF_VALUE_TYPE_DOUBLE )
                type = IDMEF_ADDITIONAL_DATA_TYPE_REAL;

        else if ( vtype == IDMEF_VALUE_TYPE_STRING )
                type = IDMEF_ADDITIONAL_DATA_TYPE_STRING;

        else if ( vtype == IDMEF_VALUE_TYPE_DATA )
                type = IDMEF_ADDITIONAL_DATA_TYPE_BYTE_STRING;

       else if ( vtype == IDMEF_VALUE_TYPE_TIME )
                type = IDMEF_ADDITIONAL_DATA_TYPE_DATE_TIME;

        else
                return;

        idmef_additional_data_set_type(data, type);
}



static int _idmef_path_set(const idmef_path_t *path, idmef_class_id_t class, size_t i,
                           int index_override, void *ptr, idmef_value_t *value)
{
        void *prevptr = NULL;
        int ret, index, j;
        prelude_bool_t is_last_element;
        idmef_value_type_id_t tid;
        const idmef_path_element_t *elem;
        idmef_class_id_t parent_class;

        parent_class = (class == IDMEF_CLASS_ID_MESSAGE) ? class : path->elem[i - 1].class;

        for ( ; i < path->depth; i++ ) {

                elem = &path->elem[i];
                index = (index_override) ? index_override : elem->index;
                index_override = 0;
                is_last_element = (i == (path->depth - 1));

                if ( index == INDEX_UNDEFINED || index == INDEX_KEY ) {
                        if ( ! is_last_element )
                                return _idmef_path_set_undefined_not_last(path, elem, class, i, ptr, value);
                        else {
                                delete_listed_child(ptr, class, elem);

                                for ( j = 0; value && j < idmef_value_get_count(value); j++ ) {
                                        ret = _idmef_path_set(path, class, i, IDMEF_LIST_APPEND, ptr, idmef_value_get_nth(value, j));
                                        if ( ret < 0 )
                                                return ret;

                                        if ( index == INDEX_KEY ) {
                                                prelude_list_t *head;

                                                ret = idmef_class_get_child(ptr, class, elem->position, (void *) &head);
                                                if ( ret < 0 )
                                                        return ret;

                                                ret = set_index_key(elem, head->prev);
                                                if ( ret < 0 )
                                                        return ret;
                                        }
                                }

                                return 0;
                        }
                }

                parent_class = class;

                if ( value || ! is_last_element ) {
                        prevptr = ptr;

                        ret = idmef_class_new_child(ptr, class, elem->position, index, &ptr);
                        if ( ret < 0 )
                                return ret;

                        class = idmef_class_get_child_class(class, elem->position);
                        assert( ! (class < 0 && i < path->depth - 1) );
                }
        }

        elem = &path->elem[path->depth - 1];
        if ( ! value )
                return idmef_class_destroy_child(ptr, parent_class, elem->position, elem->index);

        tid = idmef_class_get_child_value_type(parent_class, elem->position);
        if ( parent_class == IDMEF_CLASS_ID_ADDITIONAL_DATA && tid == IDMEF_VALUE_TYPE_DATA && prevptr )
                ad_magic_set_datatype(prevptr, idmef_value_get_type(value));

        return _idmef_value_copy_internal(value, tid, class, ptr);
}



/**
 * idmef_path_get:
 * @path: Pointer to a #idmef_path_t object.
 * @obj: Pointer to a #idmef_object_t object.
 * @ret: Address where to store the retrieved #idmef_value_t.
 *
 * This function retrieves the value for @path within @message,
 * and stores it into the provided @ret address of type #idmef_value_t.
 *
 * Returns: The number of element retrieved, or a negative value if an error occured.
 */
int idmef_path_get(const idmef_path_t *path, void *obj, idmef_value_t **ret)
{
        idmef_object_t *object = obj;

        if ( object->_idmef_object_id != path->top_class )
                return prelude_error_verbose(PRELUDE_ERROR_GENERIC, "path for object '%s' used with '%s' object root",
                                             idmef_class_get_name(path->top_class), idmef_class_get_name(object->_idmef_object_id));

        return idmef_path_get_internal(ret, path, 0, object, object->_idmef_object_id);
}



/**
 * idmef_path_set:
 * @path: Pointer to a #idmef_path_t object.
 * @obj: Pointer to a #idmef_object_t object.
 * @value: Pointer to a #idmef_value_t object.
 *
 * This function sets the provided @value for @path within @message.
 *
 * Returns: 0 on success, a negative value if an error occured.
 */
int idmef_path_set(const idmef_path_t *path, void *obj, idmef_value_t *value)
{
        idmef_object_t *object = obj;

        if ( path->depth < 1 )
                return prelude_error_verbose(PRELUDE_ERROR_GENERIC, "Path with depth of 0 are not allowed");

        if ( object->_idmef_object_id != path->top_class )
                return prelude_error_verbose(PRELUDE_ERROR_GENERIC, "path for object '%s' used with '%s' object root",
                                             idmef_class_get_name(path->top_class), idmef_class_get_name(object->_idmef_object_id));


        return _idmef_path_set(path, object->_idmef_object_id, 0, 0, object, value);
}



static int copy_path_name(idmef_path_t *path, const char *buffer)
{
        size_t len;

        len = strlen(buffer) + 1;
        if ( len >= sizeof(path->name) )
                return prelude_error(PRELUDE_ERROR_IDMEF_PATH_LENGTH);

        memcpy(path->name, buffer, len);

        return 0;
}



/*
 * idmef_object_create returns:
 * -1 if something wrong happen
 * 0 for a new empty object
 * 1 for an existing object already in the cache
 */
static int idmef_path_create(idmef_path_t **path, idmef_class_id_t rootclass, const char *buffer)
{
        int ret;
        idmef_path_t tmp;

        ret = copy_path_name(&tmp, buffer);
        if ( ret < 0 )
                return ret;

        tmp.top_class = rootclass;

        gl_lock_lock(cached_path_mutex);

        ret = initialize_path_cache_if_needed();
        if ( ret < 0 ) {
                gl_lock_unlock(cached_path_mutex);
                return ret;
        }

        *path = prelude_hash_get(cached_path, &tmp);
        gl_lock_unlock(cached_path_mutex);

        if ( *path )
                return 1;

        *path = calloc(1, sizeof(**path));
        if ( ! *path )
                return prelude_error_from_errno(errno);

        (*path)->refcount = 1;
        gl_lock_init((*path)->mutex);

        return 0;
}



/*
 * return 1 if we are reading the last object.
 * return 0 if reading an object.
 * return -1 if there is no more things to read.
 */
static int parse_path_token(char **sptr, char **out)
{
        char *buf = *sptr, *ptr;

        if ( *buf == '\0' )
                *buf++ = '.';

        *out = buf;

        ptr = strchr(buf, '.');
        if ( ! ptr )
                return -1;

        *ptr = '\0';
        *sptr = ptr;

        return 0;
}



static int idmef_path_parse_new(idmef_path_t *path, idmef_class_id_t rootclass, const char *buffer)
{
        int ret, index = -1, is_last;
        unsigned int depth = 0;
        char *endptr, *ptr, *ptr2, *end;
        idmef_class_child_id_t child = 0;
        idmef_class_id_t class;
        idmef_value_type_id_t vtype;

        ret = copy_path_name(path, buffer);
        if ( ret < 0 )
                return ret;

        end = ptr = NULL;
        endptr = path->name;
        path->top_class = class = rootclass;

        do {
                index = INDEX_UNDEFINED;
                is_last = parse_path_token(&endptr, &ptr);

                ptr2 = strchr(ptr, '(');
                if ( ptr2 ) {
                        char *errptr = NULL;
                        char *tok = ptr2 + 1;

                        end = strchr(ptr2, ')');
                        if ( ! end )
                                return prelude_error_verbose(PRELUDE_ERROR_GENERIC, "Malformed IDMEFPath index : missing ')' character");

                        *end = *ptr2 = '\0';
                        if ( strcmp(tok, "<<") == 0 )
                                index = IDMEF_LIST_PREPEND;

                        else if ( strcmp(tok, ">>") == 0 )
                                index = IDMEF_LIST_APPEND;

                        else if ( (*tok == '\'' || *tok == '"') && *(tok + strlen(tok) - 1) == *tok ) {
                                index = INDEX_KEY;
                                path->elem[depth].index_key = strndup(tok + 1, strlen(tok) - 2);
                        }

                        else if ( strcmp(ptr2 + 1, "*") != 0 ) {
                                index = strtol(ptr2 + 1, &errptr, 0);
                                if ( index == 0 && errptr == tok )
                                        return prelude_error_verbose(PRELUDE_ERROR_GENERIC, "Invalid index specified : `%s`", tok);
                        }
                        *end = ')';
                }

                child = idmef_class_find_child(class, ptr);
                if ( child < 0 )
                        return child;

                path->elem[depth].position = child;

                if ( ptr2 )
                        *ptr2 = '(';

                if ( index == INDEX_UNDEFINED )
                        path->elem[depth].index = idmef_class_is_child_list(class, child) ? INDEX_UNDEFINED : INDEX_FORBIDDEN;
                else {
                        if ( ! idmef_class_is_child_list(class, child) )
                                return prelude_error_verbose(PRELUDE_ERROR_IDMEF_PATH_INDEX_FORBIDDEN,
                                                             "Invalid IDMEF path element '%s': indexing not supported", ptr);

                        if ( index == INDEX_KEY && ! idmef_class_is_child_keyed_list(class, child) )
                                return prelude_error_verbose(PRELUDE_ERROR_IDMEF_PATH_INDEX_FORBIDDEN,
                                                             "Invalid IDMEF path element '%s': key not supported as index", ptr);

                        path->elem[depth].index = index;
                }

                /* The last object may not be a structure */
                vtype = path->elem[depth].value_type = idmef_class_get_child_value_type(class, child);
                if ( vtype != IDMEF_VALUE_TYPE_CLASS && ! is_last )
                        return prelude_error_verbose(PRELUDE_ERROR_GENERIC,
                                                     "IDMEF element '%s' is a leaf and thus has no child '%s'",
                                                     ptr, endptr + 1);

                class = path->elem[depth].class = idmef_class_get_child_class(class, child);
                if ( ++depth == MAX_DEPTH )
                        return prelude_error(PRELUDE_ERROR_IDMEF_PATH_DEPTH);

        } while ( ! is_last );

        path->depth = depth;

        return 0;
}




/**
 * idmef_path_new_from_root_fast:
 * @path: Address where to store the created #idmef_path_t object.
 * @rootclass: #idmef_class_id_t ID of the root path.
 * @buffer: Name of the path to create.
 *
 * Creates a #idmef_path_t object whom root is @rootclass pointing
 * to @buffer, and stores it within @path.
 *
 * Returns: 0 on success, or a negative value if an error occured.
 */
int idmef_path_new_from_root_fast(idmef_path_t **path, idmef_class_id_t rootclass, const char *buffer)
{
        int ret;

        prelude_return_val_if_fail(buffer, prelude_error(PRELUDE_ERROR_ASSERTION));

        ret = idmef_path_create(path, rootclass, buffer);
        if ( ret < 0 )
                return ret;

        if ( ret == 1 ) {
                idmef_path_ref(*path);
                return 0;
        }

        if ( *buffer == '\0' )
                (*path)->top_class = rootclass;
        else {
                ret = idmef_path_parse_new(*path, rootclass, buffer);
                if ( ret < 0 ) {
                        gl_lock_destroy((*path)->mutex);
                        free(*path);
                        return ret;
                }
        }

        gl_lock_lock(cached_path_mutex);

        if ( prelude_hash_add(cached_path, *path, *path) < 0 ) {

                gl_lock_destroy((*path)->mutex);
                free(*path);
                gl_lock_unlock(cached_path_mutex);
                return ret;
        }

        gl_lock_unlock(cached_path_mutex);

        idmef_path_ref(*path);

        return 0;
}




/**
 * idmef_path_new_fast:
 * @path: Address where to store the created #idmef_path_t object.
 * @buffer: Name of the path to create.
 *
 * Creates a #idmef_path_t object pointing to @buffer, and stores it within @path.
 *
 * Returns: 0 on success, or a negative value if an error occured.
 */
int idmef_path_new_fast(idmef_path_t **path, const char *buffer)
{
        return idmef_path_new_from_root_fast(path, IDMEF_CLASS_ID_MESSAGE, buffer);
}



/**
 * idmef_path_new_v:
 * @path: Address where to store the created #idmef_path_t object.
 * @format: Format string.
 * @args: Pointer to a variable argument list.
 *
 * Creates an #idmef_path_t object pointing to the provided format
 * string @format and @args, and stores it within @path.
 *
 * Returns: 0 on success, or a negative value if an error occured.
 */
int idmef_path_new_v(idmef_path_t **path, const char *format, va_list args)
{
        int ret;
        char buffer[MAX_NAME_LEN];

        prelude_return_val_if_fail(format, prelude_error(PRELUDE_ERROR_ASSERTION));

        ret = vsnprintf(buffer, sizeof(buffer), format, args);
        if ( ret < 0 || (size_t) ret > sizeof(buffer) - 1 )
                return prelude_error(PRELUDE_ERROR_IDMEF_PATH_LENGTH);

        return idmef_path_new_fast(path, buffer);
}



/**
 * idmef_path_new:
 * @path: Address where to store the created #idmef_path_t object.
 * @format: Format string.
 * @...: Arguments list.
 *
 * Creates an #idmef_path_t object pointing to the provided format
 * string @format and @..., and stores it within @path.
 *
 * Returns: 0 on success, or a negative value if an error occured.
 */
int idmef_path_new(idmef_path_t **path, const char *format, ...)
{
        int ret;
        va_list args;

        prelude_return_val_if_fail(format, prelude_error(PRELUDE_ERROR_ASSERTION));

        va_start(args, format);
        ret = idmef_path_new_v(path, format, args);
        va_end(args);

        return ret;
}



/**
 * idmef_path_get_class:
 * @path: Pointer to an #idmef_path_t object.
 * @depth: Depth of @path to retrieve the #idmef_class_id_t from.
 *
 * Retrieves the #idmef_class_id_t value for the element of @path
 * located at @depth. If depth is -1, the last element depth is addressed.
 *
 * Returns: The #idmef_class_id_t for the elemnt, or a negative value if an error occured.
 */
idmef_class_id_t idmef_path_get_class(const idmef_path_t *path, int depth)
{
        prelude_return_val_if_fail(path, prelude_error(PRELUDE_ERROR_ASSERTION));

        if ( depth < 0 )
                depth = path->depth - 1;

        if ( path->depth == 0 && depth < 0 )
                return path->top_class;

        return path->elem[depth].class;
}



/**
 * idmef_path_get_value_type:
 * @path: Pointer to an #idmef_path_t object.
 * @depth: Depth of @path to retrieve the #idmef_value_type_id_t from.
 *
 * Retrieves the #idmef_value_type_id_t identifying the type of value
 * acceptable for this path element, for the @path element located at
 * @depth. If depth is -1, the last element depth is addressed.
 *
 * Returns: The #idmef_value_type_id_t for the element, or a negative value if an error occured.
 */
idmef_value_type_id_t idmef_path_get_value_type(const idmef_path_t *path, int depth)
{
        prelude_return_val_if_fail(path, prelude_error(PRELUDE_ERROR_ASSERTION));

        if ( depth < 0 )
                depth = path->depth - 1;

        if ( path->depth == 0 && depth < 0 )
                return IDMEF_VALUE_TYPE_CLASS;

        return path->elem[depth].value_type;
}



static inline int invalidate(idmef_path_t *path)
{
        int ret;

        gl_lock_lock(path->mutex);

        if ( path->refcount == 1 ) {
                gl_lock_unlock(path->mutex);
                return 0; /* not cached */
        }

        /*
         * We can modify the path only if the caller
         * is the only entity that has pointer to it.
         *
         * The path should have refcount equal to 1 or 2.
         * If refcount is equal to 1, it means that only the caller
         * has the pointer to the path, so we can modify it.
         *
         * If refcount is equal to 2, that means that the path
         * can also be present in the cached_path hash,
         * so it should be removed from the hash before modification.
         * If, however, refcount is 2 but the path is not present
         * in the hash, we cannot continue, as there exists
         * another entity that has pointer to the path.
         */

        if ( path->refcount > 2 ) {
                gl_lock_unlock(path->mutex);
                return -1;
        }

        if ( path->refcount == 2 ) {
                gl_lock_lock(cached_path_mutex);
                ret = prelude_hash_elem_destroy(cached_path, path);
                gl_lock_unlock(cached_path_mutex);

                if ( ret == 0 )
                        path->refcount--;  /* path was present in a hash */
                else {
                        gl_lock_unlock(path->mutex);
                        return -1; /* path was not present in a hash and refcount != 1 */
                }
        }

        gl_lock_unlock(path->mutex);

        return 0; /* successfully invalidated */
}



/**
 * idmef_path_set_index:
 * @path: Pointer to an #idmef_path_t object.
 * @depth: Depth of @path to set @index for.
 * @index: Index for the provided element @depth.
 *
 * Modifies @index for the element located at @depth of provided @path.
 * This function is only applicable for element that accept listed value.
 *
 * Returns: 0 on success, a negative value if an error occured.
 */
int idmef_path_set_index(idmef_path_t *path, unsigned int depth, int index)
{
        int ret;

        prelude_return_val_if_fail(path, prelude_error(PRELUDE_ERROR_ASSERTION));
        prelude_return_val_if_fail(depth < path->depth, prelude_error(PRELUDE_ERROR_IDMEF_PATH_DEPTH));

        if ( index == INDEX_FORBIDDEN || index == INDEX_KEY )
                return prelude_error(PRELUDE_ERROR_IDMEF_PATH_INDEX_RESERVED);

        if ( path->elem[depth].index == INDEX_FORBIDDEN )
                return prelude_error(PRELUDE_ERROR_IDMEF_PATH_INDEX_FORBIDDEN);

        ret = invalidate(path);
        if ( ret < 0 )
                return ret;

        if ( path->elem[depth].index == INDEX_KEY )
                free(path->elem[depth].index_key);

        path->elem[depth].index = index;

        ret = build_name(path);
        if ( ret < 0 )
                return ret;

        return 0;
}



/**
 * idmef_path_undefine_index:
 * @path: Pointer to an #idmef_path_t object.
 * @depth: Depth of @path to undefine the index for.
 *
 * Modifies the element located at @depth of provided @path so that it's
 * index is undefined.
 *
 * This function is only applicable for element that accept listed value.
 *
 * Returns: 0 on success, a negative value if an error occured.
 */
int idmef_path_undefine_index(idmef_path_t *path, unsigned int depth)
{
        prelude_return_val_if_fail(path, prelude_error(PRELUDE_ERROR_ASSERTION));

        return idmef_path_set_index(path, depth, INDEX_UNDEFINED);
}



/**
 * idmef_path_get_index:
 * @path: Pointer to an #idmef_path_t object.
 * @depth: Depth of @path to retrieve the index from.
 *
 * Gets the current index for element located at @depth of @path.
 * This function is only applicable for element that accepts listed value.
 *
 * Returns: The element index, or a negative value if an error occured.
 */
int idmef_path_get_index(const idmef_path_t *path, unsigned int depth)
{
        prelude_return_val_if_fail(path, prelude_error(PRELUDE_ERROR_ASSERTION));
        prelude_return_val_if_fail(depth < path->depth, prelude_error(PRELUDE_ERROR_IDMEF_PATH_DEPTH));

        if ( path->elem[depth].index == INDEX_UNDEFINED )
                return prelude_error(PRELUDE_ERROR_IDMEF_PATH_INDEX_UNDEFINED);

        if ( path->elem[depth].index == INDEX_FORBIDDEN )
                return prelude_error(PRELUDE_ERROR_IDMEF_PATH_INDEX_FORBIDDEN);

        return path->elem[depth].index;
}



int idmef_path_get_key(const idmef_path_t *path, unsigned int depth, const char **key)
{
        prelude_return_val_if_fail(path, prelude_error(PRELUDE_ERROR_ASSERTION));
        prelude_return_val_if_fail(depth < path->depth, prelude_error(PRELUDE_ERROR_IDMEF_PATH_DEPTH));

        if ( path->elem[depth].index == INDEX_UNDEFINED )
                return prelude_error(PRELUDE_ERROR_IDMEF_PATH_INDEX_UNDEFINED);

        if ( path->elem[depth].index == INDEX_FORBIDDEN )
                return prelude_error(PRELUDE_ERROR_IDMEF_PATH_INDEX_FORBIDDEN);

        if ( path->elem[depth].index != INDEX_KEY )
                return prelude_error(PRELUDE_ERROR_IDMEF_PATH_INDEX_FORBIDDEN);

        *key = path->elem[depth].index_key;
        return 0;
}



/**
 * idmef_path_make_child:
 * @path: Pointer to an #idmef_path_t object.
 * @child_name: Name of the child element to create.
 * @index: Index for @child_name, if applicable.
 *
 * Modifies @path so that it points to the child node identified by @child_name,
 * children of the current path. That is if the path is currently pointing to
 * alert.classification, and @child_name is set to "text", @path will be
 * modified to point to alert.classification.text.
 *
 * Returns: 0 on success, or a negative value if an error occured.
 */
int idmef_path_make_child(idmef_path_t *path, const char *child_name, int index)
{
        int ret;
        char buf[16] = { 0 };
        idmef_class_id_t class;
        idmef_class_child_id_t child;

        prelude_return_val_if_fail(path, prelude_error(PRELUDE_ERROR_ASSERTION));
        prelude_return_val_if_fail(child_name, prelude_error(PRELUDE_ERROR_ASSERTION));

        if ( path->depth > MAX_DEPTH - 1 )
                return prelude_error(PRELUDE_ERROR_IDMEF_PATH_DEPTH);

        class = idmef_path_get_class(path, -1);

        child = idmef_class_find_child(class, child_name);
        if ( child < 0 )
                return child;

        ret = invalidate(path);
        if ( ret < 0 )
                return ret;

        if ( index >= 0 && idmef_class_is_child_list(class, child) )
             snprintf(buf, sizeof(buf), "(%d)", index);

        snprintf(path->name + strlen(path->name), sizeof(path->name) - strlen(path->name), "%s%s%s",
                 (path->depth > 0) ? "." : "", child_name, buf);

        path->depth++;

        path->elem[path->depth - 1].position = child;
        if ( idmef_class_is_child_list(class, child) )
                path->elem[path->depth - 1].index = (index < 0) ? INDEX_UNDEFINED : index;
        else
                path->elem[path->depth - 1].index = INDEX_FORBIDDEN;

        path->elem[path->depth - 1].class = idmef_class_get_child_class(class, child);
        path->elem[path->depth - 1].value_type = idmef_class_get_child_value_type(class, child);

        return 0;
}


/**
 * idmef_path_make_parent:
 * @path: Pointer to an #idmef_path_t object.
 *
 * Removes the last element of the path. That is, if @path is currently pointing to
 * alert.classification, @path will be modified to point to alert.
 *
 * Returns: 0 on success, or a negative value if an error occured.
 */
int idmef_path_make_parent(idmef_path_t *path)
{
        int ret;
        char *ptr;

        prelude_return_val_if_fail(path, prelude_error(PRELUDE_ERROR_ASSERTION));

        if ( path->depth == 0 )
                return prelude_error(PRELUDE_ERROR_IDMEF_PATH_PARENT_ROOT);

        ret = invalidate(path);
        if ( ret < 0 )
                return ret;

        path->depth--;

        if ( path->name[0] ) {
                ptr = strrchr(path->name, '.');
                if ( ! ptr )
                        ptr = path->name; /* top-level path */

                *ptr = '\0';
        }

        return 0;
}



/**
 * idmef_path_destroy:
 * @path: Pointer to an #idmef_path_t object.
 *
 * Destroys the provided @path object.
 */
void idmef_path_destroy(idmef_path_t *path)
{
        prelude_return_if_fail(path);

        gl_lock_lock(path->mutex);

        if ( --path->refcount ) {
                gl_lock_unlock(path->mutex);
                return;
        }

        gl_lock_unlock(path->mutex);
        gl_lock_destroy(path->mutex);
        free(path);
}



/**
 * idmef_path_ncompare:
 * @p1: Pointer to an #idmef_path_t object.
 * @p2: Pointer to another #idmef_path_t object.
 * @depth: Maximum depth to use for path comparison.
 *
 * Compares @p1 and @p2 elements up to @depth.
 *
 * Returns: 0 if both element match, a negative value otherwise.
 */
int idmef_path_ncompare(const idmef_path_t *p1, const idmef_path_t *p2, unsigned int depth)
{
        unsigned int i;

        prelude_return_val_if_fail(p1, prelude_error(PRELUDE_ERROR_ASSERTION));
        prelude_return_val_if_fail(p2, prelude_error(PRELUDE_ERROR_ASSERTION));

        for ( i = 0; i < depth; i++ ) {

                if ( p1->elem[i].index != p2->elem[i].index )
                        return -1;

                if ( p1->elem[i].index == INDEX_KEY && strcmp(p1->elem[i].index_key, p2->elem[i].index_key) != 0 )
                        return -1;

                if ( p1->elem[i].position - p2->elem[i].position )
                        return -1;
        }

        return 0;
}



/**
 * idmef_path_compare:
 * @p1: Pointer to an #idmef_path_t object.
 * @p2: Pointer to another #idmef_path_t object.
 *
 * Compares @p1 and @p2 elements.
 *
 * Returns: 0 if both element match, a negative value otherwise.
 */
int idmef_path_compare(const idmef_path_t *p1, const idmef_path_t *p2)
{
        prelude_return_val_if_fail(p1, prelude_error(PRELUDE_ERROR_ASSERTION));
        prelude_return_val_if_fail(p2, prelude_error(PRELUDE_ERROR_ASSERTION));

        if ( p1->depth != p2->depth )
                return -1;

        return idmef_path_ncompare(p1, p2, p1->depth);
}




/**
 * idmef_path_clone:
 * @src: Pointer to an #idmef_path_t object.
 * @dst: Address where to store the copy of @src.
 *
 * Clones @src and stores the result in the provided @dst address.
 *
 * Returns: 0 on success, a negative value otherwise.
 */
int idmef_path_clone(const idmef_path_t *src, idmef_path_t **dst)
{
        unsigned int i;

        prelude_return_val_if_fail(src, prelude_error(PRELUDE_ERROR_ASSERTION));

        *dst = calloc(1, sizeof(**dst));
        if ( ! *dst )
                return prelude_error_from_errno(errno);

        memcpy(*dst, src, sizeof(**dst));

        for ( i = 0; i < src->depth; i++ ) {
                if ( ! src->elem[i].index_key )
                        continue;

                (*dst)->elem[i].index_key = strdup(src->elem[i].index_key);
                if ( ! (*dst)->elem[i].index_key ) {
                        while ( --i ) {
                                if ( (*dst)->elem[i].index_key )
                                        free((*dst)->elem[i].index_key);
                        }

                        free(*dst);
                        return prelude_error_from_errno(errno);
                }
        }

        (*dst)->refcount = 1;
        gl_lock_init((*dst)->mutex);

        return 0;
}



/**
 * idmef_path_check_operator:
 * @path: Pointer to a #idmef_path_t object.
 * @op: Operator to check compatibility with.
 *
 * Check whether @op can apply to value pointed to by @path.
 *
 * Returns: 0 on success, a negative value if an error occured.
 */
int idmef_path_check_operator(const idmef_path_t *path, idmef_criterion_operator_t op)
{
        prelude_return_val_if_fail(path, prelude_error(PRELUDE_ERROR_ASSERTION));
        return idmef_value_type_check_operator(idmef_path_get_value_type(path, -1), op);
}



/**
 * idmef_path_get_applicable_operators:
 * @path: Pointer to a #idmef_path_t object.
 * @result: Pointer to storage for applicable operator.
 *
 * Retrieve all applicable operator that might be used by the type of
 * value pointed to by @path.
 *
 * Returns: 0 on success, a negative value if an error occured.
 */
int idmef_path_get_applicable_operators(const idmef_path_t *path, idmef_criterion_operator_t *result)
{
        prelude_return_val_if_fail(path && result, prelude_error(PRELUDE_ERROR_ASSERTION));
        return idmef_value_type_get_applicable_operators(idmef_path_get_value_type(path, -1), result);
}



/**
 * idmef_path_ref:
 * @path: Pointer to an #idmef_path_t object.
 *
 * Increases @path reference count.
 *
 * idmef_path_destroy() will destroy the refcount until it reaches 0,
 * at which point the path will be destroyed.
 *
 * Returns: The provided @path is returned.
 */
idmef_path_t *idmef_path_ref(idmef_path_t *path)
{
        prelude_return_val_if_fail(path, NULL);

        gl_lock_lock(path->mutex);
        path->refcount++;
        gl_lock_unlock(path->mutex);

        return path;
}



/**
 * idmef_path_is_ambiguous:
 * @path: Pointer to an #idmef_path_t object.
 *
 * Returns TRUE if @path contain elements that are supposed
 * to be listed, but for which no index were provided.
 *
 * Returns: TRUE if the object is ambiguous, FALSE otherwise.
 */
prelude_bool_t idmef_path_is_ambiguous(const idmef_path_t *path)
{
        size_t i;

        prelude_return_val_if_fail(path, FALSE);

        for ( i = 0; i < path->depth; i++ ) {
                if ( path->elem[i].index == INDEX_UNDEFINED )
                        return TRUE;
        }

        return FALSE;
}



/**
 * idmef_path_has_lists:
 * @path: Pointer to an #idmef_path_t object.
 *
 * Returns: the number of listed element within @path.
 */
int idmef_path_has_lists(const idmef_path_t *path)
{
        size_t i;
        int ret = 0;

        prelude_return_val_if_fail(path, 0);

        for ( i = 0; i < path->depth; i++ ) {
                if ( path->elem[i].index != INDEX_FORBIDDEN )
                        ret++;
        }

        return ret;
}



prelude_bool_t idmef_path_is_list(const idmef_path_t *path, int depth)
{
        prelude_return_val_if_fail(path, FALSE);

        if ( depth < 0 )
                depth = path->depth - 1;

        return path->elem[depth].index != INDEX_FORBIDDEN;
}




/**
 * idmef_path_get_depth:
 * @path: Pointer to an #idmef_path_t object.
 *
 * Returns: @depth number of elements.
 */
unsigned int idmef_path_get_depth(const idmef_path_t *path)
{
        prelude_return_val_if_fail(path, 0);
        return path->depth;
}



/**
 * idmef_path_get_name:
 * @path: Pointer to an #idmef_path_t object.
 * @depth: Depth of the @path element to get the name from.
 *
 * Returns the full path name if the provided @depth is -1, or the specific
 * element name if depth is set. That is, for a @path pointing to
 * "alert.classification.text": A depth of -1 would return "alert.classification.text";
 * a depth of 0 would return "alert"; a depth of 1 would return "classification"; and
 * a depth of 2 would return "text".
 *
 * Returns: @path name relative to the provided @dept.
 */
const char *idmef_path_get_name(const idmef_path_t *path, int depth)
{
        const char *ret;
        const idmef_path_element_t *elem;

        prelude_return_val_if_fail(path, NULL);
        prelude_return_val_if_fail(depth < 0 || (size_t) depth < path->depth, NULL);

        if ( depth < 0 )
                return path->name;

        elem = &path->elem[depth];

        if ( depth > 0 && (elem->class < 0 || elem->value_type == IDMEF_VALUE_TYPE_ENUM) )
                ret = idmef_class_get_child_name(path->elem[depth - 1].class, elem->position);
        else
                ret = idmef_class_get_name(elem->class);

        return ret;
}



void _idmef_path_cache_lock(void)
{
        gl_lock_lock(cached_path_mutex);

        if ( cached_path )
                prelude_hash_iterate(cached_path, path_lock_cb);
}



void _idmef_path_cache_reinit(void)
{
        gl_lock_init(cached_path_mutex);

        if ( cached_path )
                prelude_hash_iterate(cached_path, path_reinit_cb);
}




void _idmef_path_cache_unlock(void)
{
        if ( cached_path )
                prelude_hash_iterate(cached_path, path_unlock_cb);

        gl_lock_unlock(cached_path_mutex);
}



void _idmef_path_cache_destroy(void)
{
        if ( ! cached_path )
                return;

        flush_cache = TRUE;
        prelude_hash_destroy(cached_path);
        flush_cache = FALSE;
}
