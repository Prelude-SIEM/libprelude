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

#include "idmef-value-type.h"

typedef struct idmef_value idmef_value_t;

#include "idmef-type.h"

#define idmef_value_new_hdr(n, t) idmef_value_t *idmef_value_new_ ## n ( t  val ) 

idmef_value_new_hdr(int16, int16_t);
idmef_value_new_hdr(uint16, uint16_t);
idmef_value_new_hdr(int32, int32_t);
idmef_value_new_hdr(uint32, uint32_t);
idmef_value_new_hdr(int64, int64_t);
idmef_value_new_hdr(uint64, uint64_t);
idmef_value_new_hdr(float, float);
idmef_value_new_hdr(double, double);
/* idmef_value_new_hdr(void, void *); */
/* idmef_value_new_hdr(char, char *); */
/* idmef_value_new_hdr(uchar, uchar_t *); */

idmef_value_t *idmef_value_new_string(idmef_string_t *string);
idmef_value_t *idmef_value_new_time(idmef_time_t *time);
idmef_value_t *idmef_value_new_data(idmef_data_t *data);
idmef_value_t *idmef_value_new_object(void *object, idmef_type_t object_type);
idmef_value_t *idmef_value_new_list(void);
idmef_value_t *idmef_value_new_enum(idmef_type_t type, const char *buf);
idmef_value_t *idmef_value_new_enum_string(idmef_type_t type, const char *buf);
idmef_value_t *idmef_value_new_enum_numeric(idmef_type_t type, int val);
idmef_value_t *idmef_value_new_generic(idmef_value_type_id_t type, const char *buf);

#define idmef_value_get_hdr(n, t) t idmef_value_get_ ## n (idmef_value_t *val)

idmef_value_get_hdr(int16, int16_t);
idmef_value_get_hdr(uint16, uint16_t);
idmef_value_get_hdr(int32, int32_t);
idmef_value_get_hdr(uint32, uint32_t);
idmef_value_get_hdr(int64, int64_t);
idmef_value_get_hdr(uint64, uint64_t);
idmef_value_get_hdr(enum, int);
idmef_value_get_hdr(float, float);
idmef_value_get_hdr(double, double);

#define idmef_value_get_struct_hdr(n) idmef_ ## n ## _t *idmef_value_get_ ## n (idmef_value_t *val)

idmef_value_get_struct_hdr(string);
idmef_value_get_struct_hdr(time);
idmef_value_get_struct_hdr(data);

int idmef_value_list_add(idmef_value_t *list, idmef_value_t *new);
int idmef_value_is_list(idmef_value_t *list);
int idmef_value_list_empty(idmef_value_t *list);

int idmef_value_have_own_data(idmef_value_t *value);
int idmef_value_dont_have_own_data(idmef_value_t *value);

idmef_value_type_id_t idmef_value_get_type(idmef_value_t *value);
idmef_type_t idmef_value_get_idmef_type(idmef_value_t *value);

void *idmef_value_get_object(idmef_value_t *value);

int idmef_value_iterate(idmef_value_t *value, void *extra, int (*callback)(idmef_value_t *ptr, void *extra));

idmef_value_t *idmef_value_get_nth(idmef_value_t *val, int n);

int idmef_value_get_count(idmef_value_t *val);

idmef_value_t *idmef_value_clone(idmef_value_t *val);

idmef_value_t *idmef_value_ref(idmef_value_t *val);

int idmef_value_to_string(idmef_value_t *val, char *buf, int len);

int idmef_value_get(void *res, idmef_value_t *val);

int idmef_value_match(idmef_value_t *val1, idmef_value_t *val2, idmef_relation_t relation);

void idmef_value_destroy(idmef_value_t *val);

#endif /* _LIBPRELUDE_IDMEF_VALUE_H */
