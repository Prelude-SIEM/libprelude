/*****
*
* Copyright (C) 2002-2006,2007,2008 PreludeIDS Technologies. All Rights Reserved.
* Author: Nicolas Delon <nicolas.delon@prelude-ids.com>
* Author: Krzysztof Zaraska <kzaraska@student.uci.agh.edu.pl>
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

#ifndef _LIBPRELUDE_IDMEF_VALUE_H
#define _LIBPRELUDE_IDMEF_VALUE_H


typedef struct idmef_value idmef_value_t;

#include "prelude-io.h"
#include "idmef-value-type.h"
#include "prelude-string.h"
#include "idmef-class.h"
#include "idmef-path.h"

#ifdef __cplusplus
 extern "C" {
#endif

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
int idmef_value_new_class(idmef_value_t **value, idmef_class_id_t classid, void *ptr);
int idmef_value_new_list(idmef_value_t **value);
int idmef_value_new_enum(idmef_value_t **value, idmef_class_id_t classid, const char *buf);
int idmef_value_new_enum_from_string(idmef_value_t **value, idmef_class_id_t classid, const char *buf);
int idmef_value_new_enum_from_numeric(idmef_value_t **value, idmef_class_id_t classid, int val);

int idmef_value_set_int8(idmef_value_t *value, int8_t val);
int idmef_value_set_uint8(idmef_value_t *value, uint8_t val);
int idmef_value_set_int16(idmef_value_t *value, int16_t val);
int idmef_value_set_uint16(idmef_value_t *value, uint16_t val);
int idmef_value_set_int32(idmef_value_t *value, int32_t val);
int idmef_value_set_uint32(idmef_value_t *value, uint32_t val);
int idmef_value_set_int64(idmef_value_t *value, int64_t val);
int idmef_value_set_uint64(idmef_value_t *value, uint64_t val);
int idmef_value_set_float(idmef_value_t *value, float val);
int idmef_value_set_double(idmef_value_t *value, double val);
int idmef_value_set_string(idmef_value_t *value, prelude_string_t *string);
int idmef_value_set_time(idmef_value_t *value, idmef_time_t *time);
int idmef_value_set_data(idmef_value_t *value, idmef_data_t *data);
int idmef_value_set_enum(idmef_value_t *value, idmef_class_id_t classid, const char *buf);
int idmef_value_set_enum_from_string(idmef_value_t *value, idmef_class_id_t classid, const char *buf);
int idmef_value_set_enum_from_numeric(idmef_value_t *value, idmef_class_id_t classid, int no);
int idmef_value_set_class(idmef_value_t *value, idmef_class_id_t classid, void *ptr);

int idmef_value_new(idmef_value_t **value, idmef_value_type_id_t type, void *ptr);
int idmef_value_new_from_path(idmef_value_t **value, idmef_path_t *path, const char *buf);
int idmef_value_new_from_string(idmef_value_t **value, idmef_value_type_id_t type, const char *buf);

int8_t idmef_value_get_int8(const idmef_value_t *val);
uint8_t idmef_value_get_uint8(const idmef_value_t *val);
int16_t idmef_value_get_int16(const idmef_value_t *val);
uint16_t idmef_value_get_uint16(const idmef_value_t *val);
int32_t idmef_value_get_int32(const idmef_value_t *val);
uint32_t idmef_value_get_uint32(const idmef_value_t *val);
int64_t idmef_value_get_int64(const idmef_value_t *val);
uint64_t idmef_value_get_uint64(const idmef_value_t *val);
int idmef_value_get_enum(const idmef_value_t *val);
float idmef_value_get_float(const idmef_value_t *val);
double idmef_value_get_double(const idmef_value_t *val);

idmef_time_t *idmef_value_get_time(const idmef_value_t *val);
idmef_data_t *idmef_value_get_data(const idmef_value_t *val);
prelude_string_t *idmef_value_get_string(const idmef_value_t *val);

int idmef_value_list_add(idmef_value_t *list, idmef_value_t *item);
prelude_bool_t idmef_value_is_list(const idmef_value_t *list);
prelude_bool_t idmef_value_list_is_empty(const idmef_value_t *list);

int idmef_value_have_own_data(idmef_value_t *value);
int idmef_value_dont_have_own_data(idmef_value_t *value);

idmef_value_type_id_t idmef_value_get_type(const idmef_value_t *value);
idmef_class_id_t idmef_value_get_class(const idmef_value_t *value);

void *idmef_value_get_object(const idmef_value_t *value);

int idmef_value_iterate(const idmef_value_t *value, int (*callback)(idmef_value_t *ptr, void *extra), void *extra);

int idmef_value_iterate_reversed(const idmef_value_t *value, int (*callback)(idmef_value_t *ptr, void *extra), void *extra);

idmef_value_t *idmef_value_get_nth(const idmef_value_t *val, int n);

int idmef_value_get_count(const idmef_value_t *val);

int idmef_value_clone(const idmef_value_t *val, idmef_value_t **dst);

idmef_value_t *idmef_value_ref(idmef_value_t *val);

int idmef_value_print(const idmef_value_t *val, prelude_io_t *fd);

int idmef_value_to_string(const idmef_value_t *val, prelude_string_t *out);

int idmef_value_get(const idmef_value_t *val, void *res);

int idmef_value_match(idmef_value_t *val1, idmef_value_t *val2, idmef_criterion_operator_t op);

int idmef_value_check_operator(const idmef_value_t *value, idmef_criterion_operator_t op);

int idmef_value_get_applicable_operators(const idmef_value_t *value, idmef_criterion_operator_t *result);

void idmef_value_destroy(idmef_value_t *val);

#ifndef SWIG

int _idmef_value_copy_internal(const idmef_value_t *val,
                               idmef_value_type_id_t res_type, idmef_class_id_t res_id, void *res);

int _idmef_value_cast(idmef_value_t *val, idmef_value_type_id_t target_type, idmef_class_id_t enum_class);
#endif

#ifdef __cplusplus
 }
#endif

#endif /* _LIBPRELUDE_IDMEF_VALUE_H */
