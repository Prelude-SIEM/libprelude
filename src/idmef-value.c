/*****
*
* Copyright (C) 2003-2005 PreludeIDS Technologies. All Rights Reserved.
* Author: Nicolas Delon <delon.nicolas@wanadoo.fr>
* Author: Yoann Vandoorselaere <yoann@prelude-ids.org>
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

#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <assert.h>

#include "prelude-list.h"
#include "prelude-log.h"
#include "prelude-inttypes.h"
#include "prelude-string.h"
#include "prelude-error.h"

#include "idmef.h"
#include "idmef-value-type.h"

#define CHUNK_SIZE 16


#define idmef_value_new_decl(mtype, vname, vtype)                    \
int idmef_value_new_ ## vname (idmef_value_t **value, vtype val) {   \
        int ret;              				             \
                                                                     \
	ret = idmef_value_create(value, IDMEF_VALUE_TYPE_ ## mtype); \
	if ( ret < 0 )			 	                     \
		return ret;                                          \
						                     \
	(*value)->type.data. vname ## _val = val;                    \
						                     \
	return 0;				                     \
}


#define idmef_value_get_decl(value, mtype, vname, vtype)	 \
vtype idmef_value_get_ ## vname (value *val)                     \
{							         \
	if ( val->type.id != IDMEF_VALUE_TYPE_ ## mtype )        \
		return (vtype) 0;			         \
							         \
	return val->type.data. vname ## _val;		         \
}



typedef struct compare {
        unsigned int match;
        idmef_value_t *val2;
	idmef_criterion_operator_t operator;
} compare_t;



struct idmef_value {
    	int list_elems;
    	int list_max;
	int refcount;
	int own_data;
	idmef_value_t **list;
	idmef_value_type_t type;
	idmef_class_id_t class;
};



static int string_isdigit(const char *s)
{
	while ( *s ) {
		if ( ! isdigit((int) *s) )
			return -1;
		s++;
	}

	return 0;
}



static int idmef_value_create(idmef_value_t **ret, idmef_value_type_id_t type_id)
{
	*ret = calloc(1, sizeof(**ret));        
        if ( ! *ret )
		return prelude_error_from_errno(errno);
        
	(*ret)->refcount = 1;
	(*ret)->own_data = 1;
        (*ret)->type.id = type_id;
                
	return 0;
}



idmef_value_new_decl(INT8, int8, int8_t)
idmef_value_new_decl(UINT8, uint8, uint8_t)
idmef_value_new_decl(INT16, int16, int16_t)
idmef_value_new_decl(UINT16, uint16, uint16_t)
idmef_value_new_decl(INT32, int32, int32_t)
idmef_value_new_decl(UINT32, uint32, uint32_t)
idmef_value_new_decl(INT64, int64, int64_t)
idmef_value_new_decl(UINT64, uint64, uint64_t)
idmef_value_new_decl(FLOAT, float, float)
idmef_value_new_decl(DOUBLE, double, double)

idmef_value_get_decl(const idmef_value_t, INT8, int8, int8_t)
idmef_value_get_decl(const idmef_value_t, UINT8, uint8, uint8_t)
idmef_value_get_decl(const idmef_value_t, INT16, int16, int16_t)
idmef_value_get_decl(const idmef_value_t, UINT16, uint16, uint16_t)
idmef_value_get_decl(const idmef_value_t, INT32, int32, int32_t)
idmef_value_get_decl(const idmef_value_t, UINT32, uint32, uint32_t)
idmef_value_get_decl(const idmef_value_t, INT64, int64, int64_t)
idmef_value_get_decl(const idmef_value_t, UINT64, uint64, uint64_t)
idmef_value_get_decl(const idmef_value_t, ENUM, enum, int)
idmef_value_get_decl(const idmef_value_t, FLOAT, float, float)
idmef_value_get_decl(const idmef_value_t, DOUBLE, double, double)
idmef_value_get_decl(idmef_value_t, STRING, string, prelude_string_t *)
idmef_value_get_decl(idmef_value_t, DATA, data, idmef_data_t *)
idmef_value_get_decl(idmef_value_t, TIME, time, idmef_time_t *)



int idmef_value_new_string(idmef_value_t **value, prelude_string_t *string)
{
        int ret;
        
	ret = idmef_value_create(value, IDMEF_VALUE_TYPE_STRING);
	if ( ret < 0 )
		return ret;
	
	(*value)->type.data.string_val = string;
	
	return ret;
}



int idmef_value_new_time(idmef_value_t **value, idmef_time_t *time)
{
        int ret;

	ret = idmef_value_create(value, IDMEF_VALUE_TYPE_TIME);
	if ( ret < 0 )
		return ret;

	(*value)->type.data.time_val = time;

	return ret;
}



int idmef_value_new_data(idmef_value_t **value, idmef_data_t *data)
{
        int ret;

	ret = idmef_value_create(value, IDMEF_VALUE_TYPE_DATA);
	if ( ret < 0 )
		return ret;

	(*value)->type.data.data_val = data;

	return ret;
}




int idmef_value_new_class(idmef_value_t **value, idmef_class_id_t class, void *object)
{
        int ret;
        
	ret = idmef_value_create(value, IDMEF_VALUE_TYPE_CLASS);
        if ( ret < 0 )
		return ret;
        
	(*value)->class = class;
	(*value)->type.data.object_val = object;
        
	return ret;
}



int idmef_value_new_enum_from_numeric(idmef_value_t **value, idmef_class_id_t class, int val)
{
	int ret;
						
	ret = idmef_value_create(value, IDMEF_VALUE_TYPE_ENUM);
	if ( ret < 0 )
		return ret;
        
	(*value)->class = class;
	(*value)->type.data.enum_val = val;

	return ret;
}



int idmef_value_new_enum_from_string(idmef_value_t **value, idmef_class_id_t class, const char *buf)
{
    	int ret;

    	ret = idmef_class_enum_to_numeric(class, buf);
  	if ( ret < 0 )
	    	return ret;
	
	return idmef_value_new_enum_from_numeric(value, class, ret);
}



int idmef_value_new_enum(idmef_value_t **value, idmef_class_id_t class, const char *buf)
{
        if ( string_isdigit(buf) == 0 )
                return idmef_value_new_enum_from_numeric(value, class, atoi(buf));
        else
		return idmef_value_new_enum_from_string(value, class, buf);
}



int idmef_value_new_list(idmef_value_t **value)
{
        int ret;
        
	ret = idmef_value_create(value, IDMEF_VALUE_TYPE_CLASS);
	if ( ret < 0 )
		return ret;

	(*value)->list = malloc(CHUNK_SIZE * sizeof(idmef_value_t *));
	if ( ! (*value)->list ) {
                free(*value);
	    	return prelude_error_from_errno(errno);
        }
        
	(*value)->list_elems = 0;
	(*value)->list_max = CHUNK_SIZE - 1;

	return 0;	
}



int idmef_value_list_add(idmef_value_t *list, idmef_value_t *new)
{        
	if ( list->list_elems == list->list_max ) {
                
		list->list = realloc(list->list, (list->list_max + 1 + CHUNK_SIZE) * sizeof(idmef_value_t *));
	        if ( ! list->list )
                        return prelude_error_from_errno(errno);

                list->list_max += CHUNK_SIZE;
	}
        
	list->list[list->list_elems++] = new;
	
	return 0;
}



prelude_bool_t idmef_value_is_list(idmef_value_t *list)
{
	return (list->list ? TRUE : FALSE);
}



prelude_bool_t idmef_value_list_is_empty(idmef_value_t *list)
{	
    	return (list->list_elems) ? FALSE : TRUE;
}




int idmef_value_new(idmef_value_t **value, idmef_value_type_id_t type, void *ptr)
{
        int ret;
        
    	ret = idmef_value_create(value, type);
    	if ( ret < 0 )
	    	return ret;

        (*value)->type.data.data_val = ptr;

        return 0;
}



int idmef_value_new_from_string(idmef_value_t **value, idmef_value_type_id_t type, const char *buf)
{
        int ret;
        
    	ret = idmef_value_create(value, type);
    	if ( ret < 0 )
	    	return ret;
        
        ret = idmef_value_type_read(&(*value)->type, buf);
        if ( ret < 0 ) {
                free(*value);
                return ret;
        }
                
	return 0;
}



int idmef_value_new_from_path(idmef_value_t **value, idmef_path_t *path, const char *buf)
{
        int ret;
        idmef_class_id_t class;
    	idmef_value_type_id_t value_type;
        		
	value_type = idmef_path_get_value_type(path, -1);
        if ( value_type < 0 )
                return value_type;

        if ( value_type != IDMEF_VALUE_TYPE_ENUM )
                ret = idmef_value_new_from_string(value, value_type, buf);
        else {
                class = idmef_path_get_class(path, -1);                
                if ( class < 0 )
                        return class;
                
                ret = idmef_value_new_enum(value, class, buf);
        }
        
        return ret;
}



static int idmef_value_set_own_data(idmef_value_t *value, prelude_bool_t own_data)
{
	int cnt;

        if ( ! value->list )
                value->own_data = own_data;

        else for ( cnt = 0 ; cnt < value->list_elems; cnt++ )
                idmef_value_set_own_data(value->list[cnt], own_data);
	
	return 0;
}




int idmef_value_have_own_data(idmef_value_t *value)
{
	return idmef_value_set_own_data(value, TRUE);
}



int idmef_value_dont_have_own_data(idmef_value_t *value)
{
	return idmef_value_set_own_data(value, FALSE);
}



idmef_value_type_id_t idmef_value_get_type(const idmef_value_t *value)
{
	return value->type.id;
}



idmef_class_id_t idmef_value_get_class(const idmef_value_t *value)
{
	return (value->type.id == IDMEF_VALUE_TYPE_CLASS ||
                value->type.id == IDMEF_VALUE_TYPE_ENUM) ? value->class : -1;
}



void *idmef_value_get_object(idmef_value_t *value)
{
	return (value->type.id == IDMEF_VALUE_TYPE_CLASS) ? value->type.data.object_val : NULL;
        
}



int idmef_value_iterate(idmef_value_t *value, int (*callback)(idmef_value_t *ptr, void *extra), void *extra)
{
	int i, ret;

        if ( ! value->list )
                return callback(value, extra);
        
        for ( i = 0; i < value->list_elems; i++ ) {
                
                ret = callback(value->list[i], extra);
                if ( ret < 0 )
                        return -1;
        }
		
	return 0;
}




idmef_value_t *idmef_value_get_nth(idmef_value_t *val, int n)
{	
	if ( ! val->list )
	    	return (n == 0) ? val : NULL;
        
	return (n >= 0 && n < val->list_elems) ? val->list[n] : NULL;
}




int idmef_value_get_count(const idmef_value_t *val)
{
	return val->list ? val->list_elems : 1;
}




static int idmef_value_list_clone(idmef_value_t *val, idmef_value_t **dst)
{
        int cnt, ret;
        
	ret = idmef_value_create(dst, val->type.id);
	if ( ret < 0 )
		return ret;

	(*dst)->list_elems = val->list_elems;
	(*dst)->list_max = val->list_max;
	(*dst)->list = malloc(((*dst)->list_elems + 1) * sizeof((*dst)->list));

	for ( cnt = 0; cnt < (*dst)->list_elems; cnt++ ) {

                ret = idmef_value_clone(val->list[cnt], &((*dst)->list[cnt]));
		if ( ret < 0 ) {
			while ( --cnt >= 0 )
				idmef_value_destroy((*dst)->list[cnt]);
                }

                free((*dst)->list);
                free(*dst);
                
                return -1;
	}

	return 0;
}



static int idmef_value_enum_clone(idmef_value_t *val, idmef_value_t **dst)
{
        int ret;
        
	ret = idmef_value_create(dst, IDMEF_VALUE_TYPE_ENUM);
	if ( ret < 0 )
		return ret;

	(*dst)->class = val->class;
	(*dst)->type.data.enum_val = val->type.data.enum_val;

	return 0;
}


int idmef_value_clone(idmef_value_t *val, idmef_value_t **dst)
{
        int ret;
        
	if ( val->list )
		return idmef_value_list_clone(val, dst);

	if ( val->type.id == IDMEF_VALUE_TYPE_ENUM )
		return idmef_value_enum_clone(val, dst);

	ret = idmef_value_create(dst, val->type.id);
	if ( ret < 0 )
		return ret;
	
        ret = idmef_value_type_clone(&val->type, &(*dst)->type);
        if ( ret < 0 )
                free(*dst);
        
	return ret;
}



idmef_value_t *idmef_value_ref(idmef_value_t *val)
{	
	val->refcount++;
	
	return val;
}



int idmef_value_to_string(const idmef_value_t *val, prelude_string_t *out)
{
	int ret;
        const char *str;
        
	if ( idmef_value_get_type(val) == IDMEF_VALUE_TYPE_ENUM ) {
                str = idmef_class_enum_to_string(idmef_value_get_class(val), idmef_value_get_enum(val));
                ret = prelude_string_cat(out, str);
	} else
		ret = idmef_value_type_write(&val->type, out);

	return ret;
}



int idmef_value_print(const idmef_value_t *val, prelude_io_t *fd)
{
        int ret;
        const char *str;
        prelude_string_t *out;
        
        if ( idmef_value_get_type(val) == IDMEF_VALUE_TYPE_ENUM ) {
                str = idmef_class_enum_to_string(idmef_value_get_class(val), idmef_value_get_enum(val));
                ret = prelude_io_write(fd, str, strlen(str));
        }

        else {
                ret = prelude_string_new(&out);
                if ( ret < 0 )
                        return ret;

                ret = idmef_value_type_write(&val->type, out);
                if ( ret < 0 ) {
                        prelude_string_destroy(out);
                        return ret;
                }

                ret = prelude_io_write(fd, prelude_string_get_string(out), prelude_string_get_len(out));
        }

        return ret;
}



int idmef_value_get(idmef_value_t *val, void *res)
{        
        return idmef_value_type_copy(&val->type, res);
}



static int idmef_value_match_internal(idmef_value_t *val1, void *extra)
{
        int ret;
	idmef_value_t *val2;
        compare_t *compare = extra;
	idmef_criterion_operator_t operator;

        if ( idmef_value_is_list(val1) )
                return idmef_value_iterate(val1, idmef_value_match_internal, extra);
        
	val2 = compare->val2;
	operator = compare->operator;

        assert(! val1 || ! val2 || val1->type.id == val2->type.id);
                
        ret = idmef_value_type_compare(&val1->type, &val2->type, operator);        
        if ( ret == 0 )
                compare->match++;
        
        return 0;
}



/**
 * idmef_value_match:
 * @val1: Pointer to a #idmef_value_t object.
 * @val2: Pointer to a #idmef_value_t object.
 * @operator: operator to use for matching.
 *
 * Match @val1 and @val2 using @operator.
 *
 * Returns: the number of match, 0 for none, a negative value if an error occured.
 */
int idmef_value_match(idmef_value_t *val1, idmef_value_t *val2, idmef_criterion_operator_t operator)
{
        int ret;
	compare_t compare;

        compare.match = 0;
	compare.val2 = val2;
	compare.operator = operator;
        
	ret = idmef_value_iterate(val1, idmef_value_match_internal, &compare);        
        if ( ret < 0 )
                return ret;
        
        return compare.match;
}



/**
 * idmef_value_check_operator:
 * @value: Pointer to a #idmef_value_t object.
 * @operator: Type of operator to check @value for.
 *
 * Check whether @operator can apply to @value.
 *
 * Returns: 0 on success, a negative value if an error occured.
 */
int idmef_value_check_operator(idmef_value_t *value, idmef_criterion_operator_t operator)
{
        return idmef_value_type_check_relation(&value->type, operator);
}



/**
 * idmef_value_destroy:
 * @val: Pointer to a #idmef_value_t object.
 *
 * Decrement refcount and destroy @value if it reach 0.
 */
void idmef_value_destroy(idmef_value_t *val)
{
	int i;
        
	if ( --val->refcount )
	    	return;
        
	if ( val->list ) {
		for ( i = 0; i < val->list_elems; i++ )
                        idmef_value_destroy(val->list[i]);
                
		free(val->list);
	}

	/*
         * Actual destructor starts here
         */
	if ( val->own_data )
		idmef_value_type_destroy(&val->type);

	free(val);
}

