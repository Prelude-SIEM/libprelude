/*****
*
* Copyright (C) 2002,2003, 2004 Krzysztof Zaraska <kzaraska@student.uci.agh.edu.pl>
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


#ifndef _LIBPRELUDE_IDMEF_TYPE_H
#define _LIBPRELUDE_IDMEF_TYPE_H

typedef int idmef_object_type_t;

typedef int idmef_child_t;

#include "idmef-value.h"

idmef_child_t idmef_type_find_child(idmef_object_type_t type, const char *name);

int idmef_type_child_is_list(idmef_object_type_t type, idmef_child_t child);

idmef_value_type_id_t idmef_type_get_child_type(idmef_object_type_t type, idmef_child_t child);

idmef_object_type_t idmef_type_get_child_object_type(idmef_object_type_t type, idmef_child_t child);

idmef_object_type_t idmef_type_get_child_enum_type(idmef_object_type_t type, idmef_child_t child);

char *idmef_type_get_child_name(idmef_object_type_t type, idmef_child_t child);

idmef_object_type_t idmef_type_find(const char *name);

int idmef_type_is_enum(idmef_object_type_t type);

int idmef_type_enum_to_numeric(idmef_object_type_t type, const char *val);

const char *idmef_type_enum_to_string(idmef_object_type_t type, int val);

int idmef_type_get_child(void *ptr, idmef_object_type_t type, idmef_child_t child, void **childptr);

int idmef_type_new_child(void *ptr, idmef_object_type_t type, idmef_child_t child, int n, void **childptr);

char *idmef_type_get_name(idmef_object_type_t type);

#endif /* _LIBPRELUDE_IDMEF_TYPE_H */
