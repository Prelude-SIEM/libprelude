/*****
*
* Copyright (C) 2004, 2005 PreludeIDS Technologies. All Rights Reserved.
* Author: Yoann Vandoorselaere <yoann.v@prelude-ids.com>
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
#include <assert.h>

#include "libmissing.h"
#include "prelude-log.h"

#define PRELUDE_ERROR_SOURCE_DEFAULT PRELUDE_ERROR_SOURCE_IDMEF_CRITERIA
#include "prelude-error.h"
#include "prelude-inttypes.h"

#include "idmef.h"
#include "idmef-criteria.h"


struct idmef_criterion {
	idmef_path_t *path;
	idmef_criterion_value_t *value;
	idmef_criterion_operator_t operator;
};


struct idmef_criteria {
	prelude_list_t list;
        
	idmef_criterion_t *criterion;
        struct idmef_criteria *or;
        struct idmef_criteria *and;
};



/**
 * idmef_criterion_operator_to_string:
 * @op: #idmef_criterion_operator_t type.
 *
 * Transforms @op to string.
 *
 * Returns: A pointer to an operator string or NULL.
 */
const char *idmef_criterion_operator_to_string(idmef_criterion_operator_t op)
{
        int i;
        const struct {
                idmef_criterion_operator_t operator;
                const char *name;
        } tbl[] = {
                { IDMEF_CRITERION_OPERATOR_EQUAL, "=="                        },
                { IDMEF_CRITERION_OPERATOR_NOT_EQUAL, "!="                    },
                { IDMEF_CRITERION_OPERATOR_LESSER, "<"                        },
                { IDMEF_CRITERION_OPERATOR_GREATER, ">"                       },
                { IDMEF_CRITERION_OPERATOR_SUBSTR, "subsr"                    },
                { IDMEF_CRITERION_OPERATOR_REGEX, "=~"                        },
                { IDMEF_CRITERION_OPERATOR_IS_NULL, "!"                       },
                { IDMEF_CRITERION_OPERATOR_IS_NOT_NULL, ""                    },
                { IDMEF_CRITERION_OPERATOR_LESSER|IDMEF_CRITERION_OPERATOR_EQUAL, "<="  },
                { IDMEF_CRITERION_OPERATOR_GREATER|IDMEF_CRITERION_OPERATOR_EQUAL, ">=" },
        };

        for ( i = 0; tbl[i].operator != 0; i++ ) 
                if ( op == tbl[i].operator )
                        return tbl[i].name;

        return NULL;
}



/**
 * idmef_criterion_new:
 * @criterion: Address where to store the created #idmef_criterion_t object.
 * @path: Pointer to an #idmef_path_t object.
 * @value: Pointer to an #idmef_criterion_value_t object.
 * @op: #idmef_criterion_operator_t to use for matching this criterion.
 *
 * Creates a new #idmef_criterion_t object and store it in @criterion.
 * Matching this criterion will result in comparing the object value
 * pointed by @path against the provided @value, using @op.
 *
 * Returns: 0 on success, a negative value if an error occured.
 */
int idmef_criterion_new(idmef_criterion_t **criterion, idmef_path_t *path,
                        idmef_criterion_value_t *value, idmef_criterion_operator_t op)
{
        if ( ! value && op != IDMEF_CRITERION_OPERATOR_IS_NULL && op != IDMEF_CRITERION_OPERATOR_IS_NOT_NULL )
                return -1;
        
	*criterion = calloc(1, sizeof(**criterion));
	if ( ! *criterion )
		return prelude_error_from_errno(errno);

	(*criterion)->path = path;
	(*criterion)->value = value;
	(*criterion)->operator = op;

	return 0;
}



/**
 * idmef_criterion_destroy:
 * @criterion: Pointer to a #idmef_criterion_t object.
 *
 * Destroys @criterion and its content.
 */
void idmef_criterion_destroy(idmef_criterion_t *criterion)
{
	idmef_path_destroy(criterion->path);

	if ( criterion->value ) /* can be NULL if operator is is_null or is_not_null */
		idmef_criterion_value_destroy(criterion->value);

	free(criterion);
}



/**
 * idmef_criterion_clone:
 * @criterion: Pointer to a #idmef_criterion_t object to clone.
 * @dst: Address where to store the cloned #idmef_criterion_t object.
 *
 * Clones @criterion and stores the cloned criterion within @dst.
 *
 * Returns: 0 on success, a negative value if an error occured.
 */
int idmef_criterion_clone(idmef_criterion_t *criterion, idmef_criterion_t **dst)
{
        int ret;
	idmef_path_t *path;
	idmef_criterion_value_t *value = NULL;

	ret = idmef_path_clone(criterion->path, &path);
	if ( ret < 0 )
		return ret;

	if ( criterion->value ) {
		ret = idmef_criterion_value_clone(criterion->value, &value);
		if ( ret < 0 ) {
			idmef_path_destroy(path);
			return ret;
		}
	} 

	ret = idmef_criterion_new(dst, path, value, criterion->operator);
	if ( ret < 0 ) {
		idmef_path_destroy(path);
		idmef_criterion_value_destroy(value);
		return ret;
	}

	return 0;
}



/**
 * idmef_criterion_print:
 * @criterion: Pointer to a #idmef_criterion_t object.
 * @fd: Pointer to a #prelude_io_t object.
 *
 * Dump @criterion to @fd in the form of:
 * [path] [operator] [value]
 *
 * Or if there is no value associated with the criterion:
 * [operator] [path]
 *
 * Returns: 0 on success, a negative value if an error occured.
 */
int idmef_criterion_print(const idmef_criterion_t *criterion, prelude_io_t *fd)
{
        int ret;
        prelude_string_t *out;

        ret = prelude_string_new(&out);
        if ( ret < 0 )
                return ret;

        ret = idmef_criterion_to_string(criterion, out);
        if ( ret < 0 ) {
                prelude_string_destroy(out);
                return ret;
        }

        ret = prelude_io_write(fd, prelude_string_get_string(out), prelude_string_get_len(out));
        prelude_string_destroy(out);

        return ret;
}



/**
 * idmef_criterion_to_string:
 * @criterion: Pointer to a #idmef_criterion_t object.
 * @out: Pointer to a #prelude_string_t object.
 *
 * Dump @criterion as a string to the @out buffer in the form of:
 * [path] [operator] [value]
 *
 * Or if there is no value associated with the criterion:
 * [operator] [path]
 *
 * Returns: 0 on success, a negative value if an error occured.
 */
int idmef_criterion_to_string(const idmef_criterion_t *criterion, prelude_string_t *out)
{
        const char *name, *operator;

        operator = idmef_criterion_operator_to_string(criterion->operator);
        assert(operator);

        name = idmef_path_get_name(criterion->path, -1);
        
        if ( ! criterion->value )
                return prelude_string_sprintf(out, "%s %s", operator, name);

        prelude_string_sprintf(out, "%s %s ", name, operator);

        return idmef_criterion_value_to_string(criterion->value, out);
}



/**
 * idmef_criterion_get_path:
 * @criterion: Pointer to a #idmef_criterion_t object.
 *
 * Used to access the #idmef_path_t object associated with @criterion.
 *
 * Returns: the #idmef_path_t object associated with @criterion.
 */
idmef_path_t *idmef_criterion_get_path(idmef_criterion_t *criterion)
{
	return criterion->path;
}



/**
 * idmef_criterion_get_value:
 * @criterion: Pointer to a #idmef_criterion_t object.
 *
 * Used to access the #idmef_criterion_value_t associated with @criterion.
 * There might be no value specifically if the provided #idmef_criterion_operator_t
 * was IDMEF_CRITERION_OPERATOR_NULL or IDMEF_CRITERION_OPERATOR_NOT_NULL.
 *
 * Returns: the #idmef_criterion_value_t object associated with @criterion.
 */
idmef_criterion_value_t *idmef_criterion_get_value(idmef_criterion_t *criterion)
{
	return criterion ? criterion->value : NULL;
}




/**
 * idmef_criterion_get_operator:
 * @criterion: Pointer to a #idmef_criterion_t object.
 *
 * Used to access the #idmef_criterion_operator_t enumeration associated with @criterion.
 *
 * Returns: the #idmef_criterion_operator_t associated with @criterion.
 */
idmef_criterion_operator_t idmef_criterion_get_operator(idmef_criterion_t *criterion)
{
        return criterion->operator;
}



/**
 * idmef_criterion_match:
 * @criterion: Pointer to a #idmef_criterion_t object.
 * @message: Pointer to a #idmef_message_t object to match against @criterion.
 *
 * Matches @message against the provided @criterion. This implies retrieving the
 * value associated with @criterion path, and matching it with the @idmef_criterion_value_t
 * object within @criterion.
 *
 * Returns: 1 for a match, 0 for no match, or a negative value if an error occured.
 */
int idmef_criterion_match(idmef_criterion_t *criterion, idmef_message_t *message)
{
        int ret;
	idmef_value_t *value;
	
	ret = idmef_path_get(criterion->path, message, &value);
        if ( ret < 0 )
                return ret;
        
        if ( ret == 0 ) 
		return (criterion->operator == IDMEF_CRITERION_OPERATOR_IS_NULL) ? 1 : 0;
        
        if ( ! criterion->value )
                return (criterion->operator == IDMEF_CRITERION_OPERATOR_IS_NOT_NULL) ? 1 : 0;
        
        ret = idmef_criterion_value_match(criterion->value, value, criterion->operator);
        idmef_value_destroy(value);

        if ( ret < 0 )
                return ret;
        
        return (ret > 0) ? 1 : 0;
}



/**
 * idmef_criteria_new:
 * @criteria: Address where to store the created #idmef_criteria_t object.
 *
 * Creates a new #idmef_criteria_t object and store it into @criteria.
 *
 * Returns: 0 on success, a negative value if an error occured.
 */
int idmef_criteria_new(idmef_criteria_t **criteria)
{
        *criteria = calloc(1, sizeof(**criteria));
	if ( ! *criteria )
		return prelude_error_from_errno(errno);

        (*criteria)->or = NULL;
        (*criteria)->and = NULL;

	return 0;
}



/**
 * idmef_criteria_destroy:
 * @criteria: Pointer to a #idmef_criteria_t object.
 *
 * Destroys @criteria and its content.
 */
void idmef_criteria_destroy(idmef_criteria_t *criteria)
{
	if ( criteria->criterion )
		idmef_criterion_destroy(criteria->criterion);

	if ( criteria->or )
		idmef_criteria_destroy(criteria->or);

	if ( criteria->and )
		idmef_criteria_destroy(criteria->and);
        
	free(criteria);
}



/**
 * idmef_criteria_clone:
 * @src: Pointer to a #idmef_criteria_t object to clone.
 * @dst: Address where to store the cloned #idmef_criteria_t object.
 *
 * Clones @src and stores the cloned criteria within @dst.
 *
 * Returns: 0 on success, a negative value if an error occured.
 */
int idmef_criteria_clone(idmef_criteria_t *src, idmef_criteria_t **dst)
{
        int ret;
        idmef_criteria_t *new;
        
        new = *dst = malloc(sizeof(*new));
        if ( ! new )
                return prelude_error_from_errno(errno);

        memcpy(new, src, sizeof(*new));

	if ( src->or ) {
		ret = idmef_criteria_clone(src->or, &new->or);
		if ( ret < 0 ) {
			idmef_criteria_destroy(new);
			return ret;
		}
	}

	if ( src->and ) {
		ret = idmef_criteria_clone(src->and, &new->and);
		if ( ret < 0 ) {
			idmef_criteria_destroy(new);
			return ret;
		}
	}
        
        ret = idmef_criterion_clone(src->criterion, &new->criterion);
        if ( ret < 0 ) {
                idmef_criteria_destroy(new);
                return ret;
        }
        
        return 0;
}



idmef_criteria_t *idmef_criteria_get_or(idmef_criteria_t *criteria)
{
        return criteria->or;
}



idmef_criteria_t *idmef_criteria_get_and(idmef_criteria_t *criteria)
{
        return criteria->and;
}



int idmef_criteria_print(idmef_criteria_t *criteria, prelude_io_t *fd)
{
        int ret;
        prelude_string_t *out;

        ret = prelude_string_new(&out);
        if ( ret < 0 )
                return ret;

        ret = idmef_criteria_to_string(criteria, out);
        if ( ret < 0 )
                return ret;

        ret = prelude_io_write(fd, prelude_string_get_string(out), prelude_string_get_len(out));
        prelude_string_destroy(out);

        return ret;
}



int idmef_criteria_to_string(idmef_criteria_t *criteria, prelude_string_t *out)
{
	if ( ! criteria )
		return -1;

	if ( criteria->or )
                prelude_string_sprintf(out, "((");

        idmef_criterion_to_string(criteria->criterion, out);
        
        if ( criteria->and ) {
                prelude_string_sprintf(out, " && ");
		idmef_criteria_to_string(criteria->and, out);
        }
        
        if ( criteria->or ) {
                prelude_string_sprintf(out, ") || (");
		idmef_criteria_to_string(criteria->or, out);
                prelude_string_sprintf(out, "))");
        }
        
	return 0;
}



prelude_bool_t idmef_criteria_is_criterion(idmef_criteria_t *criteria)
{
	return (criteria->criterion != NULL) ? TRUE : FALSE;
}



idmef_criterion_t *idmef_criteria_get_criterion(idmef_criteria_t *criteria)
{
	return criteria->criterion;
}



void idmef_criteria_or_criteria(idmef_criteria_t *criteria, idmef_criteria_t *criteria2)
{
        idmef_criteria_t *last = NULL;
        
        while ( criteria ) {
                last = criteria;
                criteria = criteria->or;
        }
        
        last->or = criteria2;
}



int idmef_criteria_and_criteria(idmef_criteria_t *criteria, idmef_criteria_t *criteria2)
{
        int ret;
        idmef_criteria_t *new, *last = NULL;
        
        while ( criteria ) {
                last = criteria;
                
                if ( criteria->or ) {
                        ret = idmef_criteria_clone(criteria2, &new);
                        if ( ret < 0 )
                                return ret;
                        
                        ret = idmef_criteria_and_criteria(criteria->or, new);
                        if ( ret < 0 )
                                return ret;
                }
                
                criteria = criteria->and;
        }
        
        last->and = criteria2;

        return 0;
}




void idmef_criteria_set_criterion(idmef_criteria_t *criteria, idmef_criterion_t *criterion)
{
        criteria->criterion = criterion;
}




/**
 * idmef_criteria_match:
 * @criteria: Pointer to a #idmef_criteria_t object.
 * @message: Pointer to a #idmef_message_t message.
 *
 * Matches @message against the provided criteria.
 *
 * Returns: 1 if criteria match, 0 if it did not, a negative value if an error occured.
 */
int idmef_criteria_match(idmef_criteria_t *criteria, idmef_message_t *message)
{
        int ret;
        idmef_criteria_t *next;
        
        ret = idmef_criterion_match(criteria->criterion, message);
        if ( ret < 0 )
                return ret;
        
        if ( ret == 0 )
                next = criteria->or;
        else
                next = criteria->and;
        
        if ( ! next )
                return ret;
                
        return idmef_criteria_match(next, message);
}
