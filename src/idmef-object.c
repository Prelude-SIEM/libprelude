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

#include "idmef-time.h"
#include "idmef-data.h"

#include "idmef-type.h"
#include "idmef-value.h"

#include "idmef-tree-wrap.h"
#include "idmef-object.h"
#include "prelude-string.h"
#include "prelude-linked-object.h"



#define MAX_DEPTH     16
#define MAX_NAME_LEN 128

#define INDEX_UNDEFINED 0xfe
#define INDEX_FORBIDDEN 0xff


#define MIN(a, b) ((a) > (b)) ? (a) : (b)


typedef struct {

        uint8_t no;
        idmef_child_t id;
        idmef_object_type_t object_type;
        idmef_value_type_id_t value_type;

} idmef_object_description_t;
        

struct idmef_object {
	PRELUDE_LINKED_OBJECT;

	pthread_mutex_t mutex;
	char name[MAX_NAME_LEN];
	int refcount;
	uint8_t depth;

        idmef_object_description_t desc[MAX_DEPTH];
};


typedef struct {
	idmef_object_t *object;
	uint8_t depth;
	idmef_value_t *top;
} data_t;



static prelude_hash_t *cached_objects = NULL;
static pthread_mutex_t cached_object_mutex = PTHREAD_MUTEX_INITIALIZER;


/*
 * call with mutex held.
 */
static int initialize_object_cache_if_needed(void)
{
        if ( cached_objects )
                return 0;
                        
        cached_objects = prelude_hash_new(NULL, NULL, NULL, NULL);
        if ( ! cached_objects ) 
                return -1;

        return 0;
}



static idmef_value_t *idmef_object_get_internal(idmef_object_t *, int,	void *, idmef_object_type_t);



static idmef_value_t *idmef_object_get_list_internal(idmef_object_t *object,
						     int depth,
						     prelude_list_t *list,
						     idmef_object_type_t parent_type)
{
	prelude_list_t *ptr;
	idmef_value_t *value_list, *value;

	value_list = idmef_value_new_list();
	if ( ! value_list )
		return NULL;

	prelude_list_for_each(ptr, list) {
		value = idmef_object_get_internal(object, depth, ptr, parent_type);
		if ( ! value ) {
			idmef_value_destroy(value_list);
			return NULL;
		}

		idmef_value_list_add(value_list, value);
	}

	return value_list;
}



static idmef_value_t *idmef_object_get_nth_internal(idmef_object_t *object,
						    int depth,
						    prelude_list_t *list,
						    idmef_object_type_t parent_type,
						    int which)
{
	int cnt;
	prelude_list_t *ptr;
        
	cnt = 0;
	prelude_list_for_each(ptr, list) {
		if ( cnt == which )
			return idmef_object_get_internal(object, depth, ptr, parent_type);
		cnt++;
	}

	return NULL;	
}



static idmef_value_t *idmef_object_get_internal(idmef_object_t *object, int depth,
						void *parent, idmef_object_type_t parent_type)
{
	void *child;
        uint8_t which;
        idmef_child_t child_id;
	idmef_object_type_t child_type;

        if ( depth < object->depth ) {

                child_id = object->desc[depth].id;

                child = idmef_type_get_child(parent, parent_type, child_id);
                if ( ! child )
                        return NULL;

		child_type = idmef_type_get_child_object_type(parent_type, child_id);
                
                which = object->desc[depth].no;

		if ( which == INDEX_FORBIDDEN )
			return idmef_object_get_internal(object, depth + 1, child, child_type);

		if ( which == INDEX_UNDEFINED )
			return idmef_object_get_list_internal(object, depth + 1, child, child_type);

		return idmef_object_get_nth_internal(object, depth + 1, child, child_type, which);
	}
                
        return (parent_type == -1) ? parent : idmef_value_new_object(parent, parent_type);
}



idmef_value_t *idmef_object_get(idmef_message_t *message, idmef_object_t *object)
{
	return idmef_object_get_internal(object, 0, message, IDMEF_OBJECT_TYPE_MESSAGE);
}



int idmef_object_set(idmef_message_t *message, idmef_object_t *object, idmef_value_t *value)
{
    	int i;
    	void *ptr;
        idmef_value_type_id_t tid;
        idmef_object_description_t *desc;
    	idmef_object_type_t type, parent_type;
        
	ptr = message;
	parent_type = type = IDMEF_OBJECT_TYPE_MESSAGE;
        
	for ( i = 0; i < object->depth; i++ ) {
                desc = &object->desc[i];
                
	    	if ( desc->no == INDEX_UNDEFINED && idmef_type_child_is_list(type, desc->id) )
			return -1;
		
	    	ptr = idmef_type_new_child(ptr, type, desc->id, desc->no);
		if ( ! ptr )
		    	return -2;

                parent_type = type;
                
                type = idmef_type_get_child_object_type(type, desc->id);
                if ( type < 0 && i < object->depth - 1 )
                        return -3;
        }

        tid = idmef_type_get_child_type(parent_type, object->desc[object->depth - 1].id);
        
        if ( idmef_value_get_type(value) != tid )
                return -4;

        if ( idmef_value_get(ptr, value) < 0 )
                return -5;

	return 0;

}



/*
 * idmef_object_create returns:
 * -1 if something wrong happen
 * 0 for a new empty object
 * 1 for an existing object already in the cache
 */
static int idmef_object_create(const char *buffer, idmef_object_t **object)
{
        int ret;
        
	pthread_mutex_lock(&cached_object_mutex);

        ret = initialize_object_cache_if_needed();
        if ( ret < 0 ) {
                pthread_mutex_unlock(&cached_object_mutex);
                return -1;
        }
        
	*object = prelude_hash_get(cached_objects, buffer);
        
        pthread_mutex_unlock(&cached_object_mutex);

	if ( *object )
		return 1;

	*object = calloc(1, sizeof(**object));
	if ( ! *object ) {
		log(LOG_ERR, "memory exhausted.\n");
		return -1;
	}

        (*object)->refcount = 1;
	PRELUDE_INIT_LIST_HEAD(&(*object)->list);
	pthread_mutex_init(&(*object)->mutex, NULL);

	return 0;
}



/*
 * return 1 if we are reading the last object.
 * return 0 if reading an object.
 * return -1 if there is no more things to read.
 */
static int parse_object_token(char **sptr, char **out) 
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




static int idmef_object_parse_new(const char *buffer, idmef_object_t *object)
{
	uint16_t depth = 0;
        char *endptr, *ptr, *ptr2;
        idmef_child_t id = 0;
        idmef_object_type_t type, prev_type = 0;
	int len = 0, index = -1, is_last;
        
        len = strlen(buffer) + 1;
        if ( len >= sizeof(object->name) ) {
		log(LOG_ERR, "requested object len (%d) exceeds MAX_NAME_LEN (%d) - 1.\n", len, MAX_NAME_LEN);
		return -1;
	}

        memcpy(object->name, buffer, len);

        ptr = NULL;
        endptr = object->name;
        type = IDMEF_OBJECT_TYPE_MESSAGE;
        
        do {
                index = -1;
                is_last = parse_object_token(&endptr, &ptr);
                            
                ptr2 = strchr(ptr, '(');
                if ( ptr2 ) {
                        *ptr2 = '\0';
                        index = strtol(ptr2 + 1, NULL, 0);
                }
                
                id = idmef_type_find_child(type, ptr);
                if ( id < 0 ) 
			return -1;
                
                object->desc[depth].id = id;
                
                if ( index < 0 )
                        object->desc[depth].no = idmef_type_child_is_list(type, id) ? INDEX_UNDEFINED : INDEX_FORBIDDEN;
		else {
                        *ptr2 = '(';
                        
                        if ( ! idmef_type_child_is_list(type, id) )
			    	return -1;

                        object->desc[depth].no = index;
                }

		prev_type = type;

		/* The last object may not be a structure */
		type = idmef_type_get_child_object_type(type, id);
                if ( type < 0 && ! is_last ) 
			return -1;
                
		object->desc[depth].object_type = type;
                object->desc[depth].value_type = idmef_type_get_child_type(type, id);
                
                if ( ++depth == MAX_DEPTH ) {
                        log(LOG_ERR, "requested object depth (%d) exceeds MAX_DEPTH (%d).\n", depth, MAX_DEPTH);
                        return -1;
                }
                
        } while ( ! is_last );
        
	object->depth = depth;
	object->desc[depth - 1].value_type = idmef_type_get_child_type(prev_type, id);

	if ( object->desc[depth - 1].value_type == IDMEF_VALUE_TYPE_ENUM )
		object->desc[depth - 1].object_type = idmef_type_get_child_enum_type(prev_type, id);
	else
                object->desc[depth - 1].object_type = idmef_type_get_child_object_type(prev_type, id);

        return 0;
}




idmef_object_t *idmef_object_new_fast(const char *buffer)
{
        int ret;
	idmef_object_t *object;
        
	ret = idmef_object_create(buffer, &object);
	if ( ret < 0 )
		return NULL;
        
	if ( ret == 1 )
		return idmef_object_ref(object);

        if ( *buffer == '\0' )
                object->desc[0].object_type = IDMEF_OBJECT_TYPE_MESSAGE;
        else {
                
                ret = idmef_object_parse_new(buffer, object);
                if ( ret < 0 ) {
                        pthread_mutex_destroy(&object->mutex);
                        free(object);
                        return NULL;
                }
        }

	pthread_mutex_lock(&cached_object_mutex);
        	
	if ( prelude_hash_set(cached_objects, object->name, object) < 0 ) {
		pthread_mutex_destroy(&object->mutex);
		free(object);
		pthread_mutex_unlock(&cached_object_mutex);
		return NULL;
	}
        
	pthread_mutex_unlock(&cached_object_mutex);

	return idmef_object_ref(object);
}



idmef_object_t *idmef_object_new_v(const char *format, va_list args)
{
        int ret;
	char buffer[MAX_NAME_LEN];

	ret = vsnprintf(buffer, sizeof(buffer), format, args);
	if ( ret < 0 || ret > sizeof(buffer) - 1 )
		return NULL;

	return idmef_object_new_fast(buffer);
}



idmef_object_t *idmef_object_new(const char *format, ...)
{
	va_list args;
	idmef_object_t *object;

	va_start(args, format);
	object = idmef_object_new_v(format, args);
	va_end(args);

	return object;
}



idmef_object_type_t idmef_object_get_type(idmef_object_t *object)
{
        /*
         * FIXME ?
         */
        if ( object->depth == 0 )
		return IDMEF_OBJECT_TYPE_MESSAGE;
        
	return object->desc[object->depth - 1].object_type;
}



idmef_value_type_id_t idmef_object_get_value_type(idmef_object_t *object)
{
	if ( object->depth == 0 )
		return IDMEF_VALUE_TYPE_OBJECT;
	
	return object->desc[object->depth - 1].value_type;
}



static inline int invalidate(idmef_object_t *object)
{
        int ret;
        
	pthread_mutex_lock(&object->mutex);
        
	if ( object->refcount == 1 ) {
		pthread_mutex_unlock(&object->mutex);
		return 0; /* not cached */
	}

	/*
	 * We can modify the object only if the caller
	 * is the only entity that has pointer to it.
	 *
	 * The object should have refcount equal to 1 or 2.
	 * If the refcount is equal to 1, it means that only the caller
	 * has the pointer to the object, so we can modify it.
	 *
	 * If refcount is equal to 2, that means that the object
	 * can also be present in the cached_objects hash,
	 * so it should be removed from the hash before modification.
	 * If, however, refcount is 2 but the object is not present
	 * in the hash, we cannot continue, as there exists
	 * another entity that has pointer to the object.
	 */

	if ( object->refcount > 2 ) {
		pthread_mutex_unlock(&object->mutex);
		return -1;
	}

	if ( object->refcount == 2 ) {
		pthread_mutex_lock(&cached_object_mutex);
		ret = prelude_hash_elem_destroy(cached_objects, object->name);
                pthread_mutex_unlock(&cached_object_mutex);

		if ( ret == 0 )
			object->refcount--;  /* object was present in a hash */
		else {
			pthread_mutex_unlock(&object->mutex);
			return -1; /* object was not present in a hash and refcount != 1 */
		}
	}

	pthread_mutex_unlock(&object->mutex);

	return 0; /* successfully invalidated */
}



int idmef_object_set_number(idmef_object_t *object, uint8_t depth, uint8_t number)
{
	if ( depth > object->depth )
		return -2;

	if ( depth > MAX_DEPTH ) {
		log(LOG_ERR, "MAX_DEPTH exceeded.\n");
		return -3;
	}

	if ( number == INDEX_FORBIDDEN ) {
		log(LOG_ERR, "can't set number to INDEX_FORBIDDEN.\n");
		return -4;
	}

	if ( object->desc[depth].no == INDEX_FORBIDDEN )
		return -5;

	if ( invalidate(object) < 0 )
		return -6;

	object->name[0] = '\0';

	object->desc[depth].no = number;

	return 0;
}


int idmef_object_undefine_number(idmef_object_t *object, uint8_t depth)
{
	return idmef_object_set_number(object, depth, INDEX_UNDEFINED);
}


int idmef_object_get_number(idmef_object_t *object, uint8_t depth)
{
	if ( depth > MAX_DEPTH ) {
		log(LOG_ERR, "MAX_DEPTH exceeded.\n");
		return -2;
	}

	if ( depth > object->depth )
		return -3;

	if ( object->desc[depth].no == INDEX_UNDEFINED || object->desc[depth].no == INDEX_FORBIDDEN )
		return -4;

	return object->desc[depth].no;
}



int idmef_object_make_child(idmef_object_t *object, const char *child_name, int n)
{
        idmef_object_type_t type;
	idmef_child_t child;

	if ( n == INDEX_FORBIDDEN ) {
		log(LOG_ERR, "can't set number to INDEX_FORBIDDEN.\n");
		return -2;
	}

	if ( object->depth > MAX_DEPTH - 1 ) {
		log(LOG_ERR, "maximum object depth (MAX_DEPTH=%d) exceeded.\n");
		return -3;
	}

	child = idmef_type_find_child(idmef_object_get_type(object), child_name);
	if ( child < 0 )
		return -4;
        
	if ( invalidate(object) < 0 )
		return -5;

	/* current drive^H^H^H^H^Hname is no longer valid */
	object->name[0] = '\0'; 

	type = idmef_object_get_type(object);

	object->depth++;

	object->desc[object->depth - 1].id = child;
	if ( idmef_type_child_is_list(type, child) ) {
		if ( n < 0 )
			object->desc[object->depth - 1].no = INDEX_UNDEFINED;
		else
			object->desc[object->depth - 1].no = n;
	} else
		object->desc[object->depth - 1].no = INDEX_FORBIDDEN;

	
	object->desc[object->depth - 1].value_type = idmef_type_get_child_type(type, child);

	if ( object->desc[object->depth - 1].value_type == IDMEF_VALUE_TYPE_ENUM )
		object->desc[object->depth - 1].object_type = idmef_type_get_child_enum_type(type, child);
	else
		object->desc[object->depth - 1].object_type = idmef_type_get_child_object_type(type, child);
		
	return 0;
}




int idmef_object_make_parent(idmef_object_t *object)
{
	char *ptr;

	if ( object->depth == 0 )
		return -1;

	if ( invalidate(object) < 0 )
		return -2;

	object->depth--;

	if ( object->name[0] ) {
		ptr = strrchr(object->name, '.');
		if ( ptr )
			*ptr = '\0';
		else
			object->name[0] = '\0'; /* top-level object */
	}

	return 0;
}



void idmef_object_destroy(idmef_object_t *object)
{
	pthread_mutex_lock(&object->mutex);

	if ( --object->refcount ) {
		pthread_mutex_unlock(&object->mutex);
	    	return;
	}

	prelude_list_del(&object->list);
	pthread_mutex_unlock(&object->mutex);
	pthread_mutex_destroy(&object->mutex);
	free(object);
}




int idmef_object_compare(idmef_object_t *o1, idmef_object_t *o2)
{
        int diff = 0;
	uint16_t i, depth;

	depth = MIN(o1->depth, o2->depth);
        
	for ( i = 0; i < depth; i++ ) {
		diff = o1->desc[i].id - o2->desc[i].id;
		if ( diff != 0 )
                        return diff;
	}
        
        if ( o1->depth != o2->depth )
                return o1->depth - o2->depth;
        
        for ( i = 0; i < depth; i++ ) {
                diff = o1->desc[i].no - o2->desc[i].no;
                if ( diff != 0 )
                        break;
        }

	return diff;
}




idmef_object_t *idmef_object_clone(idmef_object_t *object)
{
	idmef_object_t *new;

 	new = calloc(1, sizeof(*new));
	if ( ! new ) {
		log(LOG_ERR, "memory exhausted\n");
		return NULL;
	}

	new->refcount = 1;
	new->depth = object->depth;

	PRELUDE_INIT_LIST_HEAD(&new->list);

	strncpy(new->name, object->name, sizeof(object->name)); 
        memcpy(new->desc, object->desc, object->depth * sizeof(idmef_object_description_t));
        
        pthread_mutex_init(&new->mutex, NULL);

	return new;
}



idmef_object_t *idmef_object_ref(idmef_object_t *object)
{
 	pthread_mutex_lock(&object->mutex);
	object->refcount++;
	pthread_mutex_unlock(&object->mutex);

	return object;
}


static int build_name(idmef_object_t *object)
{
        uint16_t i;
        idmef_object_type_t type;
        char buf2[16], *name;

	/* 
	 * we don't need pthread_mutex_{,un}lock since the object has no name
	 * it means that it is not in the cache and thus, not shared
	 */

        object->name[sizeof(object->name) - 1] = '\0';
        buf2[sizeof(buf2) - 1] = '\0';

        type = IDMEF_OBJECT_TYPE_MESSAGE;

        for ( i = 0; i < object->depth; i++ ) {

		if ( i > 0 )
                	strncat(object->name, ".", sizeof(object->name) - 1);

                name = idmef_type_get_child_name(type, object->desc[i].id);
                if ( ! name ) {
			log(LOG_ERR, "object integrity error.\n");
                        return -1;
		}

                strncat(object->name, name, sizeof(object->name) - 1);

                if ( object->desc[i].no != INDEX_UNDEFINED && object->desc[i].no != INDEX_FORBIDDEN ) {
                        snprintf(buf2, sizeof(buf2) - 1, "(%hhu)", object->desc[i].no);
                        strncat(object->name, buf2, sizeof(object->name) - 1);
                }

                type = idmef_type_get_child_object_type(type, object->desc[i].id);
                if ( type < 0 && i < object->depth - 1 ) {
			log(LOG_ERR, "object integrity error.\n");
                        return -1;
		}
        }

        return 0;
}



const char *idmef_object_get_name(idmef_object_t *object)
{
	if ( object->name[0] == '\0' && object->depth ) {
		if ( build_name(object) < 0 )
			return NULL;
	}

	return object->name;
}



char *idmef_object_get_numeric(idmef_object_t *object)
{
        int i;
        char *ret;
        prelude_string_t *string;

        string = prelude_string_new();
        if ( ! string )
                return NULL;

        prelude_string_sprintf(string, "%hu", object->desc[0].id);
        
	for ( i = 1; i < object->depth; i++ ) {

                prelude_string_sprintf(string, ".%hu", object->desc[i].id);
                
	    	if ( object->desc[i].no != INDEX_UNDEFINED && object->desc[i].no != INDEX_FORBIDDEN )
                        prelude_string_sprintf(string, "(%hhu)", object->desc[i].no);
	}

        ret = prelude_string_get_string_released(string);
        prelude_string_destroy(string);
        
	return ret;
}



int idmef_object_is_ambiguous(idmef_object_t *object)
{
	int i;
        
	for ( i = 0; i < object->depth; i++ ) {
		if ( object->desc[i].no == INDEX_UNDEFINED )
			return 0;
	}

	return -1;
}



int idmef_object_has_lists(idmef_object_t *object)
{
	int i, ret = 0;

        /*
         * FIXME: return value
         */ 
	for ( i = 0; i < object->depth; i++ ) {
		if ( object->desc[i].no != INDEX_FORBIDDEN )
			ret++;
	}
	
	return ret;
}
