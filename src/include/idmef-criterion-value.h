/*****
*
* Copyright (C) 2004, 2005 PreludeIDS Technologies. All Rights Reserved.
* Author: Nicolas Delon <nicolas.delon@prelude-ids.com>
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

#ifndef _LIBPRELUDE_IDMEF_CRITERION_VALUE_H
#define _LIBPRELUDE_IDMEF_CRITERION_VALUE_H

#ifdef __cplusplus
 extern "C" {
#endif

typedef struct idmef_criterion_value idmef_criterion_value_t;

typedef enum {
        IDMEF_CRITERION_VALUE_TYPE_ERROR            = -1,
        IDMEF_CRITERION_VALUE_TYPE_VALUE            =  0,
        IDMEF_CRITERION_VALUE_TYPE_REGEX            =  1,
        IDMEF_CRITERION_VALUE_TYPE_BROKEN_DOWN_TIME =  2
} idmef_criterion_value_type_t;

         
#include "idmef-criteria.h"
#include "idmef-value.h"
 

int idmef_criterion_value_new(idmef_criterion_value_t **cv);

int idmef_criterion_value_new_regex(idmef_criterion_value_t **cv, const char *regex, idmef_criterion_operator_t op);

int idmef_criterion_value_new_value(idmef_criterion_value_t **cv, idmef_value_t *value,
                                    idmef_criterion_operator_t op);

int idmef_criterion_value_new_from_string(idmef_criterion_value_t **cv, idmef_path_t *path,
                                          const char *value, idmef_criterion_operator_t op);

int idmef_criterion_value_new_broken_down_time(idmef_criterion_value_t **cv, const char *time, idmef_criterion_operator_t op);
         
int idmef_criterion_value_clone(const idmef_criterion_value_t *src, idmef_criterion_value_t **dst);

void idmef_criterion_value_destroy(idmef_criterion_value_t *value);

int idmef_criterion_value_print(idmef_criterion_value_t *value, prelude_io_t *fd);

int idmef_criterion_value_to_string(idmef_criterion_value_t *value, prelude_string_t *out);

int idmef_criterion_value_match(idmef_criterion_value_t *cv, idmef_value_t *value, idmef_criterion_operator_t op);

const idmef_value_t *idmef_criterion_value_get_value(idmef_criterion_value_t *cv);

const char *idmef_criterion_value_get_regex(idmef_criterion_value_t *cv);

const struct tm *idmef_criterion_value_get_broken_down_time(idmef_criterion_value_t *cv);
         
idmef_criterion_value_type_t idmef_criterion_value_get_type(idmef_criterion_value_t *cv);

#ifdef __cplusplus
 }
#endif

         
#endif /* _LIBPRELUDE_IDMEF_CRITERION_VALUE_H */
