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

#ifndef _LIBPRELUDE_IDMEF_OBJECT_H
#define _LIBPRELUDE_IDMEF_OBJECT_H

typedef struct idmef_object idmef_object_t;
typedef struct idmef_object_list idmef_object_list_t;

#include <stdarg.h>

idmef_value_t *idmef_object_get(idmef_message_t *message,
				idmef_object_t *object);

int idmef_object_set(idmef_message_t *message, idmef_object_t *object, idmef_value_t *value);

idmef_object_t *idmef_object_new(const char * format, ...);
idmef_object_t *idmef_object_new_v(const char * format, va_list args);
idmef_object_t *idmef_object_new_fast(const char *buffer);

idmef_object_type_t idmef_object_get_object_type(idmef_object_t *object);

idmef_value_type_id_t idmef_object_get_value_type(idmef_object_t *object);

int idmef_object_set_number(idmef_object_t *object, uint8_t depth, uint8_t number);

int idmef_object_undefine_number(idmef_object_t *object, uint8_t depth);

int idmef_object_get_number(idmef_object_t *object, uint8_t depth);

int idmef_object_make_child(idmef_object_t *object, const char *child_name, int n);

int idmef_object_make_parent(idmef_object_t *object);

void idmef_object_destroy(idmef_object_t *object);

int idmef_object_compare(idmef_object_t *o1, idmef_object_t *o2);

idmef_object_t *idmef_object_clone(idmef_object_t *object);

idmef_object_t *idmef_object_ref(idmef_object_t *object);

char *idmef_object_get_numeric(idmef_object_t *object);
const char *idmef_object_get_name(idmef_object_t *object);

int idmef_object_is_ambiguous(idmef_object_t *object);

int idmef_object_has_lists(idmef_object_t *object);

idmef_object_list_t *idmef_object_list_new(void);

void idmef_object_list_destroy(idmef_object_list_t *object_list);

void idmef_object_list_add(idmef_object_list_t *object_list, idmef_object_t *object);

idmef_object_t *idmef_object_list_get_next(idmef_object_list_t *object_list, idmef_object_t *object);

int idmef_object_list_get_size(idmef_object_list_t *object_list);

#endif /* _LIBPRELUDE_IDMEF_OBJECT_H */
