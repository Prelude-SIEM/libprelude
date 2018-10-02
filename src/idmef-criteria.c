/*****
*
* Copyright (C) 2004-2018 CS-SI. All Rights Reserved.
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
* You should have received a copy of the GNU General Public License along
* with this program; if not, write to the Free Software Foundation, Inc.,
* 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*
*****/

#include "config.h"
#include "libmissing.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <stdarg.h>

#define PRELUDE_ERROR_SOURCE_DEFAULT PRELUDE_ERROR_SOURCE_IDMEF_CRITERIA
#include "prelude.h"
#include "idmef-criteria.h"



struct idmef_criteria {
        int refcount;

        void *left;
        void *right;

        int operator;
};



/**
 * idmef_criteria_operator_to_string:
 * @op: #idmef_criteria_operator_t type.
 *
 * Transforms @op to string.
 *
 * Returns: A pointer to a boolean operator string or NULL.
 */
const char *idmef_criteria_operator_to_string(idmef_criteria_operator_t op)
{
        int i;
        const struct {
                int operator;
                const char *name;
        } tbl[] = {
                { IDMEF_CRITERIA_OPERATOR_AND, "&&"},
                { IDMEF_CRITERIA_OPERATOR_OR, "||"},
                { IDMEF_CRITERION_OPERATOR_EQUAL,     "="            },
                { IDMEF_CRITERION_OPERATOR_EQUAL_NOCASE, "=*"        },

                { IDMEF_CRITERION_OPERATOR_NOT_EQUAL, "!="           },
                { IDMEF_CRITERION_OPERATOR_NOT_EQUAL_NOCASE, "!=*"   },

                { IDMEF_CRITERION_OPERATOR_LESSER, "<"               },
                { IDMEF_CRITERION_OPERATOR_GREATER, ">"              },
                { IDMEF_CRITERION_OPERATOR_LESSER_OR_EQUAL, "<="     },
                { IDMEF_CRITERION_OPERATOR_GREATER_OR_EQUAL, ">="    },

                { IDMEF_CRITERION_OPERATOR_REGEX, "~"                },
                { IDMEF_CRITERION_OPERATOR_REGEX_NOCASE, "~*"        },
                { IDMEF_CRITERION_OPERATOR_NOT_REGEX, "!~"           },
                { IDMEF_CRITERION_OPERATOR_NOT_REGEX_NOCASE, "!~*"   },

                { IDMEF_CRITERION_OPERATOR_SUBSTR, "<>"              },
                { IDMEF_CRITERION_OPERATOR_SUBSTR_NOCASE, "<>*"      },
                { IDMEF_CRITERION_OPERATOR_NOT_SUBSTR, "!<>"         },
                { IDMEF_CRITERION_OPERATOR_NOT_SUBSTR_NOCASE, "!<>*" },

                { IDMEF_CRITERION_OPERATOR_NOT_NULL, ""              },
                { IDMEF_CRITERION_OPERATOR_NULL, "!"                 },
        };

        for ( i = 0; tbl[i].operator != 0; i++ )
                if ( op == tbl[i].operator )
                        return tbl[i].name;

        return NULL;
}



/**
 * idmef_criterion_new:
 * @criterion: Address where to store the created #idmef_criteria_t object.
 * @path: Pointer to an #idmef_path_t object.
 * @value: Pointer to an #idmef_criterion_value_t object.
 * @op: #idmef_criterion_operator_t to use for matching this criterion.
 *
 * Creates a new #idmef_criteria_t object and store it in @criterion.
 * Matching this criterion will result in comparing the object value
 * pointed by @path against the provided @value, using @op.
 *
 * Returns: 0 on success, a negative value if an error occured.
 */
int idmef_criterion_new(idmef_criteria_t **criterion, idmef_path_t *path,
                        idmef_criterion_value_t *value, idmef_criterion_operator_t op)
{
        int ret;

        prelude_return_val_if_fail(path != NULL, prelude_error(PRELUDE_ERROR_ASSERTION));
        prelude_return_val_if_fail(! (value == NULL && ! (op & IDMEF_CRITERION_OPERATOR_NULL)), prelude_error(PRELUDE_ERROR_ASSERTION));

        ret = idmef_criteria_new(criterion);
        if ( ret < 0 )
                return ret;

        (*criterion)->left = path;
        (*criterion)->right = value;
        (*criterion)->operator = op;

        return 0;
}




static int criterion_clone(const idmef_criteria_t *criterion, idmef_criteria_t **dst)
{
        int ret;
        idmef_path_t *path;
        idmef_criterion_value_t *value = NULL;

        prelude_return_val_if_fail(criterion, prelude_error(PRELUDE_ERROR_ASSERTION));

        ret = idmef_path_clone(criterion->left, &path);
        if ( ret < 0 )
                return ret;

        if ( criterion->right ) {
                ret = idmef_criterion_value_clone(criterion->right, &value);
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



static int criterion_to_string(const idmef_criteria_t *criterion, prelude_string_t *out)
{
        const char *name, *operator;

        prelude_return_val_if_fail(criterion && idmef_criteria_is_criterion(criterion), prelude_error(PRELUDE_ERROR_ASSERTION));
        prelude_return_val_if_fail(out, prelude_error(PRELUDE_ERROR_ASSERTION));

        operator = idmef_criteria_operator_to_string(criterion->operator);
        if ( ! operator )
                return -1;

        name = idmef_path_get_name(criterion->left, -1);

        if ( ! criterion->right )
                return prelude_string_sprintf(out, "%s%s%s", operator, (*operator) ? " " : "", name);

        prelude_string_sprintf(out, "%s %s ", name, operator);

        return idmef_criterion_value_to_string(criterion->right, out);
}



/**
 * idmef_criteria_get_path:
 * @criteria: Pointer to a #idmef_criteria_t object.
 *
 * Used to access the #idmef_path_t object associated with @criteria.
 *
 * Returns: the #idmef_path_t object associated with @criteria.
 */
idmef_path_t *idmef_criteria_get_path(const idmef_criteria_t *criterion)
{
        prelude_return_val_if_fail(criterion && idmef_criteria_is_criterion(criterion), NULL);
        return criterion->left;
}



/**
 * idmef_criteria_get_value:
 * @criterion: Pointer to a #idmef_criteria_t object.
 *
 * Used to access the #idmef_criterion_value_t associated with @criterion.
 * There might be no value specifically if the provided #idmef_criterion_operator_t
 * was IDMEF_CRITERION_OPERATOR_NULL or IDMEF_CRITERION_OPERATOR_NOT_NULL.
 *
 * Returns: the #idmef_criterion_value_t object associated with @criterion.
 */
idmef_criterion_value_t *idmef_criteria_get_value(const idmef_criteria_t *criterion)
{
        prelude_return_val_if_fail(criterion && idmef_criteria_is_criterion(criterion), NULL);
        return criterion->right;
}



static int criterion_match(const idmef_criteria_t *criterion, void *object)
{
        int ret;
        idmef_value_t *value = NULL;

        prelude_return_val_if_fail(criterion, prelude_error(PRELUDE_ERROR_ASSERTION));
        prelude_return_val_if_fail(idmef_criteria_is_criterion(criterion), prelude_error(PRELUDE_ERROR_ASSERTION));
        prelude_return_val_if_fail(object, prelude_error(PRELUDE_ERROR_ASSERTION));

        ret = idmef_path_get(criterion->left, object, &value);
        if ( ret < 0 )
                return ret;

        ret = idmef_criterion_value_match(criterion->right, value, criterion->operator);
        if ( value )
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

        (*criteria)->refcount = 1;

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
        prelude_return_if_fail(criteria);

        if ( --criteria->refcount )
                return;

        if ( idmef_criteria_is_criterion(criteria) ) {
                idmef_path_destroy(criteria->left);
                if ( criteria->right ) /* can be NULL if operator is is_null or is_not_null */
                        idmef_criterion_value_destroy(criteria->right);
        }

        else {
                if ( criteria->left )
                        idmef_criteria_destroy(criteria->left);

                if ( criteria->right )
                        idmef_criteria_destroy(criteria->right);
        }

        free(criteria);
}



/**
 * idmef_criteria_ref:
 * @criteria: Pointer to a #idmef_criteria_t object to reference.
 *
 * Increases @criteria reference count.
 *
 * idmef_criteria_destroy() will decrease the refcount until it reaches
 * 0, at which point @criteria will be destroyed.
 *
 * Returns: @criteria.
 */
idmef_criteria_t *idmef_criteria_ref(idmef_criteria_t *criteria)
{
        prelude_return_val_if_fail(criteria, NULL);

        criteria->refcount++;
        return criteria;
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

        prelude_return_val_if_fail(src, prelude_error(PRELUDE_ERROR_ASSERTION));

        if ( idmef_criteria_is_criterion(src) )
                return criterion_clone(src, dst);

        ret = idmef_criteria_new(dst);
        if ( ret < 0 )
                return ret;

        new = *dst;
        new->operator = src->operator;

        if ( src->left ) {
                ret = idmef_criteria_clone(src->left, (idmef_criteria_t **) &new->left);
                if ( ret < 0 ) {
                        idmef_criteria_destroy(new);
                        return ret;
                }
        }

        if ( src->right ) {
                ret = idmef_criteria_clone(src->right, (idmef_criteria_t **) &new->right);
                if ( ret < 0 ) {
                        idmef_criteria_destroy(new);
                        return ret;
                }
        }

        return 0;
}


void idmef_criteria_set_operator(idmef_criteria_t *criteria, idmef_criteria_operator_t op)
{
        prelude_return_if_fail(criteria);
        criteria->operator = op;
}



int idmef_criteria_get_operator(const idmef_criteria_t *criteria)
{
        prelude_return_val_if_fail(criteria, prelude_error(PRELUDE_ERROR_ASSERTION));
        return criteria->operator;
}



idmef_criteria_t *idmef_criteria_get_left(const idmef_criteria_t *criteria)
{
        prelude_return_val_if_fail(criteria && ! idmef_criteria_is_criterion(criteria), NULL);
        return criteria->left;
}



idmef_criteria_t *idmef_criteria_get_right(const idmef_criteria_t *criteria)
{
        prelude_return_val_if_fail(criteria && ! idmef_criteria_is_criterion(criteria), NULL);
        return criteria->right;
}



int idmef_criteria_print(const idmef_criteria_t *criteria, prelude_io_t *fd)
{
        int ret;
        prelude_string_t *out;

        prelude_return_val_if_fail(criteria, prelude_error(PRELUDE_ERROR_ASSERTION));
        prelude_return_val_if_fail(fd, prelude_error(PRELUDE_ERROR_ASSERTION));

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




static int criteria_to_string(const idmef_criteria_t *criteria, prelude_string_t *out, unsigned int depth)
{
        const char *operator;
        prelude_bool_t negated = FALSE;

        prelude_return_val_if_fail(criteria, prelude_error(PRELUDE_ERROR_ASSERTION));
        prelude_return_val_if_fail(out, prelude_error(PRELUDE_ERROR_ASSERTION));

        negated = criteria->operator == IDMEF_CRITERIA_OPERATOR_NOT;

        if ( idmef_criteria_is_criterion(criteria) )
                return criterion_to_string(criteria, out);

        if ( depth > 0 )
                prelude_string_cat(out, "(");

        if ( negated )
                prelude_string_cat(out, "!");

        if ( criteria->left )
                criteria_to_string(criteria->left, out, depth + 1);

        if ( ! negated ) {
                operator = idmef_criteria_operator_to_string(criteria->operator);
                if ( ! operator )
                        return -1;

                prelude_string_sprintf(out, " %s ", operator);
        }

        criteria_to_string(criteria->right, out, depth + 1);

        if ( depth > 0 )
                prelude_string_cat(out, ")");

        return 0;
}



int idmef_criteria_to_string(const idmef_criteria_t *criteria, prelude_string_t *out)
{
        return criteria_to_string(criteria, out, 0);
}



inline prelude_bool_t idmef_criteria_is_criterion(const idmef_criteria_t *criteria)
{
        prelude_return_val_if_fail(criteria, FALSE);
        return ! (criteria->operator & (IDMEF_CRITERIA_OPERATOR_OR|IDMEF_CRITERIA_OPERATOR_AND) || criteria->operator == IDMEF_CRITERIA_OPERATOR_NOT);
}



static int _idmef_criteria_append_criteria(idmef_criteria_t *criteria, idmef_criteria_t *criteria2, idmef_criteria_operator_t op)
{
        int ret;
        idmef_criteria_t *new;

        prelude_return_val_if_fail(criteria, prelude_error(PRELUDE_ERROR_ASSERTION));
        prelude_return_val_if_fail(criteria2, prelude_error(PRELUDE_ERROR_ASSERTION));

        ret = idmef_criteria_new(&new);
        if ( ret < 0 )
                return ret;

        new->operator = criteria->operator;
        new->left = criteria->left;
        new->right = criteria->right;

        criteria->operator = op;
        criteria->left = new;
        criteria->right = criteria2;

        return 0;
}



int idmef_criteria_or_criteria(idmef_criteria_t *criteria, idmef_criteria_t *criteria2)
{
        return _idmef_criteria_append_criteria(criteria, criteria2, IDMEF_CRITERIA_OPERATOR_OR);
}



int idmef_criteria_and_criteria(idmef_criteria_t *criteria, idmef_criteria_t *criteria2)
{
        return _idmef_criteria_append_criteria(criteria, criteria2, IDMEF_CRITERIA_OPERATOR_AND);
}



int idmef_criteria_join(idmef_criteria_t **criteria, idmef_criteria_t *left, idmef_criteria_operator_t op, idmef_criteria_t *right)
{
        int ret;

        prelude_return_val_if_fail(right, prelude_error(PRELUDE_ERROR_ASSERTION));
        prelude_return_val_if_fail(left || op == IDMEF_CRITERIA_OPERATOR_NOT, prelude_error(PRELUDE_ERROR_ASSERTION));

        ret = idmef_criteria_new(criteria);
        if ( ret < 0 )
                return ret;

        (*criteria)->operator = op;
        (*criteria)->left = left;
        (*criteria)->right = right;

        return 0;
}



/**
 * idmef_criteria_match:
 * @criteria: Pointer to a #idmef_criteria_t object.
 * @object: Pointer to a #idmef_object_t object.
 *
 * Matches @object against the provided criteria.
 *
 * Returns: 1 if criteria match, 0 if it did not, a negative value if an error occured.
 */
int idmef_criteria_match(const idmef_criteria_t *criteria, void *object)
{
        int ret;
        prelude_bool_t not = FALSE;

        prelude_return_val_if_fail(criteria, prelude_error(PRELUDE_ERROR_ASSERTION));
        prelude_return_val_if_fail(object, prelude_error(PRELUDE_ERROR_ASSERTION));

        if ( idmef_criteria_is_criterion(criteria) )
                return criterion_match(criteria, object);

        not = criteria->operator == IDMEF_CRITERIA_OPERATOR_NOT;

        if ( ! not ) {
                ret = idmef_criteria_match(criteria->left, object);
                if ( ret < 0 )
                        return ret;
        }

        if ( not || (ret == 0 && criteria->operator & IDMEF_CRITERIA_OPERATOR_OR) || (ret == 1 && criteria->operator & IDMEF_CRITERIA_OPERATOR_AND) )
                ret = idmef_criteria_match(criteria->right, object);

        return (criteria->operator & IDMEF_CRITERIA_OPERATOR_NOT) ? !ret : ret;
}



idmef_class_id_t idmef_criteria_get_class(const idmef_criteria_t *criteria)
{
        int pc, ret;

        while ( criteria ) {
                if ( idmef_criteria_is_criterion(criteria) ) {
                        pc = idmef_path_get_class(criteria->left, 0);
                        if ( pc == IDMEF_CLASS_ID_ALERT || IDMEF_CLASS_ID_HEARTBEAT )
                                return pc;
                }

                if ( idmef_criteria_get_left(criteria) ) {
                        ret = idmef_criteria_get_class(criteria->left);
                        if ( ret >= 0 )
                                return ret;
                }

                criteria = idmef_criteria_get_right(criteria);
        }

        return prelude_error_verbose(PRELUDE_ERROR_GENERIC, "could not get message class from criteria");
}
