/*****
*
* Copyright (C) 2004-2019 CS-SI. All Rights Reserved.
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

#ifndef _LIBPRELUDE_IDMEF_CRITERIA_H
#define _LIBPRELUDE_IDMEF_CRITERIA_H

#ifdef __cplusplus
 extern "C" {
#endif


typedef enum {

        IDMEF_CRITERIA_OPERATOR_NOT               = 0x8000,
        IDMEF_CRITERIA_OPERATOR_AND               = 0x0040,
        IDMEF_CRITERIA_OPERATOR_OR                = 0x0080,

} idmef_criteria_operator_t;


typedef enum {

        IDMEF_CRITERION_OPERATOR_NOT               = 0x8000,
        IDMEF_CRITERION_OPERATOR_NOCASE            = 0x4000,

        IDMEF_CRITERION_OPERATOR_EQUAL             = 0x0001,
        IDMEF_CRITERION_OPERATOR_EQUAL_NOCASE      = IDMEF_CRITERION_OPERATOR_EQUAL|IDMEF_CRITERION_OPERATOR_NOCASE,
        IDMEF_CRITERION_OPERATOR_NOT_EQUAL         = IDMEF_CRITERION_OPERATOR_NOT|IDMEF_CRITERION_OPERATOR_EQUAL,
        IDMEF_CRITERION_OPERATOR_NOT_EQUAL_NOCASE  = IDMEF_CRITERION_OPERATOR_NOT_EQUAL|IDMEF_CRITERION_OPERATOR_EQUAL_NOCASE,

        IDMEF_CRITERION_OPERATOR_LESSER            = 0x0002,
        IDMEF_CRITERION_OPERATOR_LESSER_OR_EQUAL   = IDMEF_CRITERION_OPERATOR_LESSER|IDMEF_CRITERION_OPERATOR_EQUAL,

        IDMEF_CRITERION_OPERATOR_GREATER           = 0x0004,
        IDMEF_CRITERION_OPERATOR_GREATER_OR_EQUAL  = IDMEF_CRITERION_OPERATOR_GREATER|IDMEF_CRITERION_OPERATOR_EQUAL,

        IDMEF_CRITERION_OPERATOR_SUBSTR            = 0x0008,
        IDMEF_CRITERION_OPERATOR_SUBSTR_NOCASE     = IDMEF_CRITERION_OPERATOR_SUBSTR|IDMEF_CRITERION_OPERATOR_NOCASE,
        IDMEF_CRITERION_OPERATOR_NOT_SUBSTR        = IDMEF_CRITERION_OPERATOR_NOT|IDMEF_CRITERION_OPERATOR_SUBSTR,
        IDMEF_CRITERION_OPERATOR_NOT_SUBSTR_NOCASE = IDMEF_CRITERION_OPERATOR_NOT|IDMEF_CRITERION_OPERATOR_SUBSTR_NOCASE,

        IDMEF_CRITERION_OPERATOR_REGEX             = 0x0010,
        IDMEF_CRITERION_OPERATOR_REGEX_NOCASE      = IDMEF_CRITERION_OPERATOR_REGEX|IDMEF_CRITERION_OPERATOR_NOCASE,
        IDMEF_CRITERION_OPERATOR_NOT_REGEX         = IDMEF_CRITERION_OPERATOR_NOT|IDMEF_CRITERION_OPERATOR_REGEX,
        IDMEF_CRITERION_OPERATOR_NOT_REGEX_NOCASE  = IDMEF_CRITERION_OPERATOR_NOT|IDMEF_CRITERION_OPERATOR_REGEX_NOCASE,

        IDMEF_CRITERION_OPERATOR_NULL              = 0x0020,
        IDMEF_CRITERION_OPERATOR_NOT_NULL          = IDMEF_CRITERION_OPERATOR_NULL|IDMEF_CRITERION_OPERATOR_NOT,
} idmef_criterion_operator_t;


typedef struct idmef_criteria idmef_criteria_t;

#include "idmef-path.h"
#include "idmef-criterion-value.h"

const char *idmef_criteria_operator_to_string(idmef_criteria_operator_t op);

int idmef_criterion_new(idmef_criteria_t **criterion, idmef_path_t *path,
                        idmef_criterion_value_t *value, idmef_criterion_operator_t op);

idmef_criteria_t *idmef_criteria_ref(idmef_criteria_t *criteria);

int idmef_criteria_new(idmef_criteria_t **criteria);
void idmef_criteria_destroy(idmef_criteria_t *criteria);
int idmef_criteria_clone(idmef_criteria_t *src, idmef_criteria_t **dst);
int idmef_criteria_print(const idmef_criteria_t *criteria, prelude_io_t *fd);
int idmef_criteria_to_string(const idmef_criteria_t *criteria, prelude_string_t *out);
prelude_bool_t idmef_criteria_is_criterion(const idmef_criteria_t *criteria);

int idmef_criteria_or_criteria(idmef_criteria_t *criteria, idmef_criteria_t *criteria2);

int idmef_criteria_and_criteria(idmef_criteria_t *criteria, idmef_criteria_t *criteria2);

int idmef_criteria_join(idmef_criteria_t **criteria, idmef_criteria_t *left, idmef_criteria_operator_t op, idmef_criteria_t *right);

int idmef_criteria_match(const idmef_criteria_t *criteria, void *object);

void idmef_criteria_set_operator(idmef_criteria_t *criteria, idmef_criteria_operator_t op);

int idmef_criteria_get_operator(const idmef_criteria_t *criteria);

idmef_criteria_t *idmef_criteria_get_left(const idmef_criteria_t *criteria);

idmef_criteria_t *idmef_criteria_get_right(const idmef_criteria_t *criteria);

idmef_path_t *idmef_criteria_get_path(const idmef_criteria_t *criterion);

idmef_criterion_value_t *idmef_criteria_get_value(const idmef_criteria_t *criterion);

int idmef_criteria_new_from_string(idmef_criteria_t **criteria, const char *str);

idmef_class_id_t idmef_criteria_get_class(const idmef_criteria_t *criteria);

#ifdef __cplusplus
 }
#endif

#endif /* _LIBPRELUDE_IDMEF_CRITERIA_H */
