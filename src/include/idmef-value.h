/*****
*
* Copyright (C) 2002,2003 Krzysztof Zaraska <kzaraska@student.uci.agh.edu.pl>
* Copyright (C) 2003 Nicolas Delon <delon.nicolas@wanadoo.fr>
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

#ifndef _LIBPRELUDE_IDMEF_VALUE_H
#define _LIBPRELUDE_IDMEF_VALUE_H

#include "prelude-string.h"
#include "idmef-value-type.h"
#include "idmef-type.h"


typedef struct idmef_value idmef_value_t;

#include "idmef-path.h"


int idmef_value_new_int8(idmef_value_t **value, int8_t val);
int idmef_value_new_uint8(idmef_value_t **value, uint8_t val);
int idmef_value_new_int16(idmef_value_t **value, int16_t val);
int idmef_value_new_uint16(idmef_value_t **value, uint16_t val);
int idmef_value_new_int32(idmef_value_t **value, int32_t val);
int idmef_value_new_uint32(idmef_value_t **value, uint32_t val);
int idmef_value_new_int64(idmef_value_t **value, int64_t val);
int idmef_value_new_uint64(idmef_value_t **value, uint64_t val);
int idmef_value_new_float(idmef_value_t **value, float val);
int idmef_value_new_double(idmef_value_t **value, double val);
int idmef_value_new_string(idmef_value_t **value, prelude_string_t *string);
int idmef_value_new_time(idmef_value_t **value, idmef_time_t *time);
int idmef_value_new_data(idmef_value_t **value, idmef_data_t *data);
int idmef_value_new_object(idmef_value_t **value, idmef_object_type_t object_type, void *ptr);
int idmef_value_new_list(idmef_value_t **value);
int idmef_value_new_enum(idmef_value_t **value, idmef_object_type_t type, const char *buf);
int idmef_value_new_enum_from_string(idmef_value_t **value, idmef_object_type_t type, const char *buf);
int idmef_value_new_enum_from_numeric(idmef_value_t **value, idmef_object_type_t type, int val);

int idmef_value_new(idmef_value_t **value, idmef_value_type_id_t type, void *ptr);
int idmef_value_new_from_path(idmef_value_t **value, idmef_path_t *path, const char *buf);
int idmef_value_new_from_string(idmef_value_t **value, idmef_value_type_id_t type, const char *buf);

int8_t idmef_value_get_int8(idmef_value_t *val);
uint8_t idmef_value_get_uint8(idmef_value_t *val);
int16_t idmef_value_get_int16(idmef_value_t *val);
uint16_t idmef_value_get_uint16(idmef_value_t *val);
int32_t idmef_value_get_int32(idmef_value_t *val);
uint32_t idmef_value_get_uint32(idmef_value_t *val);
int64_t idmef_value_get_int64(idmef_value_t *val);
uint64_t idmef_value_get_uint64(idmef_value_t *val);
int idmef_value_get_enum(idmef_value_t *val);
float idmef_value_get_float(idmef_value_t *val);
double idmef_value_get_double(idmef_value_t *val);

idmef_time_t *idmef_value_get_time(idmef_value_t *val);
idmef_data_t *idmef_value_get_data(idmef_value_t *val);
prelude_string_t *idmef_value_get_string(idmef_value_t *val);

int idmef_value_list_add(idmef_value_t *list, idmef_value_t *new);
prelude_bool_t idmef_value_is_list(idmef_value_t *list);
prelude_bool_t idmef_value_list_is_empty(idmef_value_t *list);

int idmef_value_have_own_data(idmef_value_t *value);
int idmef_value_dont_have_own_data(idmef_value_t *value);

idmef_value_type_id_t idmef_value_get_type(idmef_value_t *value);
idmef_object_type_t idmef_value_get_object_type(idmef_value_t *value);

void *idmef_value_get_object(idmef_value_t *value);

int idmef_value_iterate(idmef_value_t *value, void *extra, int (*callback)(idmef_value_t *ptr, void *extra));

idmef_value_t *idmef_value_get_nth(idmef_value_t *val, int n);

int idmef_value_get_count(idmef_value_t *val);

int idmef_value_clone(idmef_value_t *val, idmef_value_t **dst);

idmef_value_t *idmef_value_ref(idmef_value_t *val);

int idmef_value_to_string(idmef_value_t *val, prelude_string_t *out);

int idmef_value_get(idmef_value_t *val, void *res);

int idmef_value_match(idmef_value_t *val1, idmef_value_t *val2, idmef_value_relation_t relation);

int idmef_value_check_relation(idmef_value_t *value, idmef_value_relation_t relation);

const char *idmef_value_relation_to_string(idmef_value_relation_t relation);

void idmef_value_destroy(idmef_value_t *val);

#endif /* _LIBPRELUDE_IDMEF_VALUE_H */
