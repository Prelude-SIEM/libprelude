/*****
*
* Copyright (C) 2002,2003 Krzysztof Zaraska <kzaraska@student.uci.agh.edu.pl>
* All Rights Reserved
*
* This file is part of the Prelude program.
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
* You should have received a copy of the GNU General Public License
* along with this program; see the file COPYING.  If not, write to
* the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
*
*****/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <stdarg.h>
#include <pthread.h>

#include "libmissing.h"
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

#include "idmef-tree-wrap.h"
#include "idmef-path.h"
#include "prelude-string.h"
#include "prelude-linked-object.h"



#define MAX_DEPTH     16
#define MAX_NAME_LEN 128

#define INDEX_UNDEFINED -2
#define INDEX_FORBIDDEN -3



typedef struct idmef_path_element {

        int index;

        idmef_class_id_t class;
        idmef_class_child_id_t position;
        idmef_value_type_id_t value_type;

} idmef_path_element_t;


struct idmef_path {
	PRELUDE_LINKED_OBJECT;

	pthread_mutex_t mutex;
	char name[MAX_NAME_LEN];
	int refcount;
	unsigned int depth;
        
        idmef_path_element_t elem[MAX_DEPTH];
};


static prelude_bool_t flush_cache = FALSE;
static prelude_hash_t *cached_path = NULL;
static pthread_mutex_t cached_path_mutex = PTHREAD_MUTEX_INITIALIZER;



static void flush_cache_if_wanted(void *ptr)
{
        if ( flush_cache )
                idmef_path_destroy(ptr);
}



/*
 * call with mutex held.
 */
static int initialize_path_cache_if_needed(void)
{
        if ( cached_path )
                return 0;
                        
        return prelude_hash_new(&cached_path, NULL, NULL, NULL, flush_cache_if_wanted);
}



static int build_name(idmef_path_t *path)
{
        unsigned int i;
        const char *name;
        char buf[16] = { 0 };
        idmef_class_id_t class;
        
        /* 
         * we don't need pthread_mutex_{,un}lock since the path has no name
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
                        snprintf(buf, sizeof(buf), "(%d)", path->elem[i].index);
                        strncat(path->name, buf, sizeof(path->name) - strlen(path->name));
                }

                class = idmef_class_get_child_class(class, path->elem[i].position);
                if ( class < 0 && i < path->depth - 1 )
                        return prelude_error(PRELUDE_ERROR_IDMEF_PATH_INTEGRITY);
        }

        return 0;
}




static int idmef_path_get_internal(idmef_value_t **ret, idmef_path_t *path, unsigned int depth,
                                   void *parent, idmef_class_id_t parent_class);


static int idmef_path_get_list_internal(idmef_value_t **value_list,
                                        idmef_path_t *path, int depth,
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

                if ( parent_class != -1 )
                        ret = idmef_path_get_internal(&value, path, depth, tmp, parent_class);
                else {                        
                        ret = idmef_value_new(&value, path->elem[depth - 1].value_type, tmp);                        
                        if ( ret == 0 ) {
                                idmef_value_dont_have_own_data(value);
                                ret = 1;
                        }
                }

                if ( ret <= 0 ) {
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

        if ( ! cnt )
                idmef_value_destroy(*value_list);
        
	return cnt;
}



static int idmef_path_get_nth_internal(idmef_value_t **value, idmef_path_t *path,
                                       unsigned int depth, prelude_list_t *list,
                                       idmef_class_id_t parent_class, int which)
{
	prelude_list_t *tmp;
	unsigned int cnt = 0;
        
	prelude_list_for_each(list, tmp) {
		if ( cnt++ == which )
			return idmef_path_get_internal(value, path, depth, tmp, parent_class);
	}

        if ( which == -1 )
                return idmef_path_get_internal(value, path, depth, tmp, parent_class);

	return 0;
}



static int idmef_path_get_internal(idmef_value_t **value, idmef_path_t *path,
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
                
                if ( ! child )
                        return 0;
                
		child_class = idmef_class_get_child_class(parent_class, child_id);                
                which = path->elem[depth].index;

		if ( which == INDEX_FORBIDDEN )
			return idmef_path_get_internal(value, path, depth + 1, child, child_class);
                
		if ( which == INDEX_UNDEFINED )
			return idmef_path_get_list_internal(value, path, depth + 1, child, child_class);
                
		return idmef_path_get_nth_internal(value, path, depth + 1, child, child_class, which);
	}

        if ( parent_class == -1 || path->elem[path->depth - 1].value_type == IDMEF_VALUE_TYPE_ENUM ) {
                *value = parent;
                return 1;
        }

        return (ret = idmef_value_new_class(value, parent_class, parent) < 0) ? ret : 1;
}



int idmef_path_get(idmef_path_t *path, idmef_message_t *message, idmef_value_t **ret)
{
	return idmef_path_get_internal(ret, path, 0, message, IDMEF_CLASS_ID_MESSAGE);
}



int idmef_path_set(idmef_path_t *path, idmef_message_t *message, idmef_value_t *value)
{
    	int i, ret;
    	void *ptr, *child;
        idmef_value_type_id_t tid;
        idmef_path_element_t *elem;
    	idmef_class_id_t class, parent_class;
        
	ptr = message;
	parent_class = class = IDMEF_CLASS_ID_MESSAGE;
        
	for ( i = 0; i < path->depth; i++ ) {
                elem = &path->elem[i];
                
	    	if ( elem->index == INDEX_UNDEFINED && idmef_class_is_child_list(class, elem->position) )
			return prelude_error(PRELUDE_ERROR_IDMEF_PATH_MISS_INDEX);

                ret = idmef_class_new_child(ptr, class, elem->position, elem->index, &child);
                if ( ret < 0 )
		    	return ret;
                
                ptr = child;
                parent_class = class;
                
                class = idmef_class_get_child_class(class, elem->position);                
                if ( class < 0 && i < path->depth - 1 )
                        abort();
        }

        tid = idmef_class_get_child_value_type(parent_class, path->elem[path->depth - 1].position);
        
        if ( idmef_value_get_type(value) != tid )
                abort();
        
        return idmef_value_get(value, ptr);
}



/*
 * idmef_object_create returns:
 * -1 if something wrong happen
 * 0 for a new empty object
 * 1 for an existing object already in the cache
 */
static int idmef_path_create(idmef_path_t **path, const char *buffer)
{
        int ret;
        
	pthread_mutex_lock(&cached_path_mutex);

        ret = initialize_path_cache_if_needed();
        if ( ret < 0 ) {
                pthread_mutex_unlock(&cached_path_mutex);
                return ret;
        }
        
	*path = prelude_hash_get(cached_path, buffer);
        pthread_mutex_unlock(&cached_path_mutex);

	if ( *path )
		return 1;
        
	*path = calloc(1, sizeof(**path));        
	if ( ! *path )
		return prelude_error_from_errno(errno);

        (*path)->refcount = 1;
	prelude_list_init(&(*path)->list);
	pthread_mutex_init(&(*path)->mutex, NULL);
        
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




static int idmef_path_parse_new(idmef_path_t *path, const char *buffer)
{
        size_t len;
        int index = -1, is_last;
        unsigned int depth = 0;
        char *endptr, *ptr, *ptr2;
        idmef_class_child_id_t child = 0;
        idmef_class_id_t class, prev_class = 0;
        
        len = strlen(buffer) + 1;
        if ( len >= sizeof(path->name) )
		return prelude_error(PRELUDE_ERROR_IDMEF_PATH_LENGTH);

        memcpy(path->name, buffer, len);

        ptr = NULL;
        endptr = path->name;
        class = IDMEF_CLASS_ID_MESSAGE;
        
        do {
                index = INDEX_UNDEFINED;
                is_last = parse_path_token(&endptr, &ptr);
                                
                ptr2 = strchr(ptr, '(');
                if ( ptr2 ) {
                        *ptr2 = '\0';
                        index = strtol(ptr2 + 1, NULL, 0);
                }
                
                child = idmef_class_find_child(class, ptr);
                if ( child < 0 ) 
			return child;
                
                path->elem[depth].position = child;
                
                if ( index == INDEX_UNDEFINED )
                        path->elem[depth].index = idmef_class_is_child_list(class, child) ? INDEX_UNDEFINED : INDEX_FORBIDDEN;
		else {
                        *ptr2 = '(';
                        
                        if ( ! idmef_class_is_child_list(class, child) )
			    	return prelude_error(PRELUDE_ERROR_IDMEF_PATH_INDEX_FORBIDDEN);

                        path->elem[depth].index = index;
                }

		prev_class = class;

		/* The last object may not be a structure */
		class = idmef_class_get_child_class(class, child);
                if ( class < 0 && ! is_last )
                        return -1;
                
		path->elem[depth].class = class;
                path->elem[depth].value_type = idmef_class_get_child_value_type(class, child);
                
                if ( ++depth == MAX_DEPTH )
                        return prelude_error(PRELUDE_ERROR_IDMEF_PATH_DEPTH);
                
        } while ( ! is_last );
        
	path->depth = depth;

        path->elem[depth - 1].class = idmef_class_get_child_class(prev_class, child);
        path->elem[depth - 1].value_type = idmef_class_get_child_value_type(prev_class, child);
        
        return 0;
}




int idmef_path_new_fast(idmef_path_t **path, const char *buffer)
{
        int ret;

	ret = idmef_path_create(path, buffer);
	if ( ret < 0 )
		return ret;
        
	if ( ret == 1 ) {
                idmef_path_ref(*path);
		return 0;
        }
        
        if ( *buffer == '\0' )
                (*path)->elem[0].class = IDMEF_CLASS_ID_MESSAGE;
        else {
                ret = idmef_path_parse_new(*path, buffer);
                if ( ret < 0 ) {
                        pthread_mutex_destroy(&(*path)->mutex);
                        free(*path);
                        return ret;
                }
        }

	pthread_mutex_lock(&cached_path_mutex);
        
	if ( prelude_hash_set(cached_path, (*path)->name, *path) < 0 ) {
                
                pthread_mutex_destroy(&(*path)->mutex);
		free(*path);
		pthread_mutex_unlock(&cached_path_mutex);
		return ret;
	}
        
	pthread_mutex_unlock(&cached_path_mutex);

	idmef_path_ref(*path);

        return 0;
}



int idmef_path_new_v(idmef_path_t **path, const char *format, va_list args)
{
        int ret;
	char buffer[MAX_NAME_LEN];

	ret = vsnprintf(buffer, sizeof(buffer), format, args);
	if ( ret < 0 || ret > sizeof(buffer) - 1 )
		return prelude_error_from_errno(PRELUDE_ERROR_IDMEF_PATH_LENGTH);

	return idmef_path_new_fast(path, buffer);
}



int idmef_path_new(idmef_path_t **path, const char *format, ...)
{
        int ret;
	va_list args;
        
	va_start(args, format);
	ret = idmef_path_new_v(path, format, args);
	va_end(args);

	return ret;
}



idmef_class_id_t idmef_path_get_class(const idmef_path_t *path, int depth)
{
        if ( depth < 0 )
                depth = path->depth - 1;
        
        if ( path->depth == 0 && depth < 0 )
                return IDMEF_CLASS_ID_MESSAGE;
        
	return path->elem[depth].class;
}



idmef_value_type_id_t idmef_path_get_value_type(const idmef_path_t *path, int depth)
{
        if ( depth < 0 )
                depth = path->depth - 1;

        if ( path->depth == 0 && depth < 0 )
                return IDMEF_VALUE_TYPE_CLASS;
                
        return path->elem[depth].value_type;
}



static inline int invalidate(idmef_path_t *path)
{
        int ret;
                
        pthread_mutex_lock(&path->mutex);
        
        if ( path->refcount == 1 ) {
                pthread_mutex_unlock(&path->mutex);
                return 0; /* not cached */
        }

        /*
         * We can modify the path only if the caller
         * is the only entity that has pointer to it.
         *
         * The path should have refcount equal to 1 or 2.
         * If the refcount is equal to 1, it means that only the caller
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
                pthread_mutex_unlock(&path->mutex);
                return -1;
        }

        if ( path->refcount == 2 ) {
                pthread_mutex_lock(&cached_path_mutex);
                ret = prelude_hash_elem_destroy(cached_path, path->name);
                pthread_mutex_unlock(&cached_path_mutex);
                
                if ( ret == 0 )
                        path->refcount--;  /* path was present in a hash */
                else {
                        pthread_mutex_unlock(&path->mutex);
                        return -1; /* path was not present in a hash and refcount != 1 */
                }
        }

        pthread_mutex_unlock(&path->mutex);

        return 0; /* successfully invalidated */
}



int idmef_path_set_index(idmef_path_t *path, unsigned int depth, unsigned int index)
{
        int ret;
        
        if ( depth > MAX_DEPTH || depth > path->depth )
                return prelude_error(PRELUDE_ERROR_IDMEF_PATH_DEPTH);
        
        if ( index == INDEX_FORBIDDEN )
                return prelude_error(PRELUDE_ERROR_IDMEF_PATH_INDEX_RESERVED);
        
        if ( path->elem[depth].index == INDEX_FORBIDDEN )
                return prelude_error(PRELUDE_ERROR_IDMEF_PATH_INDEX_FORBIDDEN);

        ret = invalidate(path);
        if ( ret < 0 ) 
                return ret;

        ret = build_name(path);
        if ( ret < 0 )
                return ret;
        
        path->elem[depth].index = index;

        return 0;
}



int idmef_path_undefine_index(idmef_path_t *path, unsigned int depth)
{
        return idmef_path_set_index(path, depth, INDEX_UNDEFINED);
}



int idmef_path_get_index(const idmef_path_t *path, unsigned int depth)
{
        if ( depth > (path->depth - 1) )
                return prelude_error(PRELUDE_ERROR_IDMEF_PATH_DEPTH);
        
        if ( path->elem[depth].index == INDEX_UNDEFINED )
                return prelude_error(PRELUDE_ERROR_IDMEF_PATH_INDEX_UNDEFINED);
        
        if ( path->elem[depth].index == INDEX_FORBIDDEN )
                return prelude_error(PRELUDE_ERROR_IDMEF_PATH_INDEX_FORBIDDEN);

        return path->elem[depth].index;
}



int idmef_path_make_child(idmef_path_t *path, const char *child_name, unsigned int index)
{
        int ret;
        char buf[16] = { 0 };
        idmef_class_id_t class;
        idmef_class_child_id_t child;
        
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



int idmef_path_make_parent(idmef_path_t *path)
{
        int ret;
        char *ptr;
        
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



void idmef_path_destroy(idmef_path_t *path)
{        
        pthread_mutex_lock(&path->mutex);
        
        if ( --path->refcount ) {
                pthread_mutex_unlock(&path->mutex);
                return;
        }
        
        prelude_list_del(&path->list);
        pthread_mutex_unlock(&path->mutex);
        pthread_mutex_destroy(&path->mutex);
        free(path);
}



int idmef_path_ncompare(const idmef_path_t *o1, const idmef_path_t *o2, unsigned int depth)
{
        unsigned int i;
        
        for ( i = 0; i < depth; i++ ) {
                
                if ( o1->elem[i].index != o2->elem[i].index )
                        return -1;
                
                if ( o1->elem[i].position - o2->elem[i].position )
                        return -1;
        }

        return 0;
}



int idmef_path_compare(const idmef_path_t *o1, const idmef_path_t *o2)
{
        if ( o1->depth != o2->depth )
                return -1;

        return idmef_path_ncompare(o1, o2, o1->depth);
}




int idmef_path_clone(const idmef_path_t *src, idmef_path_t **dst)
{
        *dst = calloc(1, sizeof(**dst));
        if ( ! *dst )
                return prelude_error_from_errno(errno);

        (*dst)->refcount = 1;
        (*dst)->depth = src->depth;
        prelude_list_init(&(*dst)->list);

        strncpy((*dst)->name, src->name, sizeof(src->name)); 
        memcpy((*dst)->elem, src->elem, src->depth * sizeof(idmef_path_element_t));
        
        pthread_mutex_init(&((*dst)->mutex), NULL);

        return 0;
}



idmef_path_t *idmef_path_ref(idmef_path_t *path)
{        
        pthread_mutex_lock(&path->mutex);
        path->refcount++;
        pthread_mutex_unlock(&path->mutex);

        return path;
}



prelude_bool_t idmef_path_is_ambiguous(idmef_path_t *path)
{
	int i;
        
	for ( i = 0; i < path->depth; i++ ) {
		if ( path->elem[i].index == INDEX_UNDEFINED )
			return TRUE;
	}

	return FALSE;
}



int idmef_path_has_lists(idmef_path_t *path)
{
	int i, ret = 0;

	for ( i = 0; i < path->depth; i++ ) {
		if ( path->elem[i].index != INDEX_FORBIDDEN )
			ret++;
	}
	
	return ret;
}



void _idmef_path_cache_destroy(void)
{
        if ( ! cached_path )
                return;

        flush_cache = TRUE;
        prelude_hash_destroy(cached_path);
        flush_cache = FALSE;
}



unsigned int idmef_path_get_depth(const idmef_path_t *path)
{
        return path->depth;
}



const char *idmef_path_get_name(const idmef_path_t *path, int depth)
{
        const char *ret;
        const idmef_path_element_t *elem;
        
        if ( depth < 0 )
                return path->name;
        
        elem = &path->elem[depth];
        
        if ( elem->class == -1 || elem->value_type == IDMEF_VALUE_TYPE_ENUM )
                ret = idmef_class_get_child_name(path->elem[depth - 1].class, elem->position);
        else
                ret = idmef_class_get_name(elem->class);
        
        return ret;
}
