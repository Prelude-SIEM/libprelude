/*****
*
* Copyright (C) 2004-2005 Yoann Vandoorselaere <yoann@prelude-ids.org>
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

#ifndef _LIBPRELUDE_IDMEF_CRITERIA_H
#define _LIBPRELUDE_IDMEF_CRITERIA_H


typedef enum {
        IDMEF_CRITERION_OPERATOR_EQUAL       = 0x01,
        IDMEF_CRITERION_OPERATOR_NOT_EQUAL   = 0x02,
        IDMEF_CRITERION_OPERATOR_LESSER      = 0x04,
        IDMEF_CRITERION_OPERATOR_GREATER     = 0x08,
        IDMEF_CRITERION_OPERATOR_SUBSTR      = 0x10,
        IDMEF_CRITERION_OPERATOR_REGEX       = 0x20,
        IDMEF_CRITERION_OPERATOR_IS_NULL     = 0x40,
        IDMEF_CRITERION_OPERATOR_IS_NOT_NULL = 0x80
} idmef_criterion_operator_t;


typedef struct idmef_criteria idmef_criteria_t;
typedef struct idmef_criterion idmef_criterion_t;

#include "idmef-path.h"
#include "idmef-criterion-value.h"

const char *idmef_criterion_operator_to_string(idmef_criterion_operator_t operator);

int idmef_criterion_new(idmef_criterion_t **criterion, idmef_path_t *path,
                        idmef_criterion_value_t *value, idmef_criterion_operator_t operator);

void idmef_criterion_destroy(idmef_criterion_t *criterion);
int idmef_criterion_clone(idmef_criterion_t *src, idmef_criterion_t **dst);
int idmef_criterion_print(const idmef_criterion_t *criterion, prelude_io_t *fd);
int idmef_criterion_to_string(const idmef_criterion_t *criterion, prelude_string_t *out);
idmef_path_t *idmef_criterion_get_path(idmef_criterion_t *criterion);
idmef_criterion_value_t *idmef_criterion_get_value(idmef_criterion_t *criterion);
idmef_criterion_operator_t idmef_criterion_get_operator(idmef_criterion_t *criterion);
int idmef_criterion_match(idmef_criterion_t *criterion, idmef_message_t *message);

int idmef_criteria_new(idmef_criteria_t **criteria);
void idmef_criteria_destroy(idmef_criteria_t *criteria);
int idmef_criteria_clone(idmef_criteria_t *src, idmef_criteria_t **dst);
int idmef_criteria_print(idmef_criteria_t *criteria, prelude_io_t *fd);
int idmef_criteria_to_string(idmef_criteria_t *criteria, prelude_string_t *out);
prelude_bool_t idmef_criteria_is_criterion(idmef_criteria_t *criteria);
idmef_criterion_t *idmef_criteria_get_criterion(idmef_criteria_t *criteria);
void idmef_criteria_set_criterion(idmef_criteria_t *criteria, idmef_criterion_t *criterion);

void idmef_criteria_or_criteria(idmef_criteria_t *criteria, idmef_criteria_t *criteria2);

int idmef_criteria_and_criteria(idmef_criteria_t *criteria, idmef_criteria_t *criteria2);

int idmef_criteria_match(idmef_criteria_t *criteria, idmef_message_t *message);

idmef_criteria_t *idmef_criteria_get_or(idmef_criteria_t *criteria);

idmef_criteria_t *idmef_criteria_get_and(idmef_criteria_t *criteria);

int idmef_criteria_new_from_string(idmef_criteria_t **criteria, const char *str);

#endif /* _LIBPRELUDE_IDMEF_CRITERIA_H */
