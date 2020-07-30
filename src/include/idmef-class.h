/*****
*
* Copyright (C) 2002-2020 CS GROUP - France. All Rights Reserved.
* Author: Krzysztof Zaraska <kzaraska@student.uci.agh.edu.pl>
*
* This file is part of the Prelude library.
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU Lesser General Public License as published by
* the Free Software Foundation; either version 2.1, or (at your option)
* any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU Lesser General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public License along
* with this program; if not, write to the Free Software Foundation, Inc.,
* 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*
*****/


#ifndef _LIBPRELUDE_IDMEF_CLASS_H
#define _LIBPRELUDE_IDMEF_CLASS_H

#ifdef __cplusplus
 extern "C" {
#endif

typedef int idmef_class_id_t;
typedef int idmef_class_child_id_t;

#include "idmef-value.h"

/*
 *
 */
prelude_bool_t idmef_class_is_child_list(idmef_class_id_t classid, idmef_class_child_id_t child);

prelude_bool_t idmef_class_is_child_keyed_list(idmef_class_id_t classid, idmef_class_child_id_t child);

prelude_bool_t idmef_class_is_child_union_member(idmef_class_id_t classid, idmef_class_child_id_t child);

int idmef_class_get_child_union_id(idmef_class_id_t classid, idmef_class_child_id_t child);

idmef_class_id_t idmef_class_get_child_class(idmef_class_id_t classid, idmef_class_child_id_t child);

size_t idmef_class_get_child_count(idmef_class_id_t classid);

idmef_value_type_id_t idmef_class_get_child_value_type(idmef_class_id_t classid, idmef_class_child_id_t child);

const char **idmef_class_get_child_attributes(idmef_class_id_t classid, idmef_class_child_id_t child);



/*
 *
 */
int idmef_class_enum_to_numeric(idmef_class_id_t classid, const char *val);

const char *idmef_class_enum_to_string(idmef_class_id_t classid, int val);


/*
 *
 */
int idmef_class_get_child(void *ptr, idmef_class_id_t classid, idmef_class_child_id_t child, void **childptr);

int idmef_class_new_child(void *ptr, idmef_class_id_t classid, idmef_class_child_id_t child, int n, void **childptr);

int idmef_class_destroy_child(void *ptr, idmef_class_id_t classid, idmef_class_child_id_t child, int n);


/*
 *
 */
idmef_class_id_t idmef_class_find(const char *name);

idmef_class_child_id_t idmef_class_find_child(idmef_class_id_t classid, const char *name);

/*
 *
 */
const char *idmef_class_get_name(idmef_class_id_t classid);

const char *idmef_class_get_child_name(idmef_class_id_t classid, idmef_class_child_id_t child);


/*
 *
 */
int idmef_class_copy(idmef_class_id_t classid, const void *src, void *dst);

int idmef_class_clone(idmef_class_id_t classid, const void *src, void **dst);

int idmef_class_compare(idmef_class_id_t classid, const void *c1, const void *c2);

int idmef_class_ref(idmef_class_id_t classid, void *obj);

int idmef_class_print(idmef_class_id_t classid, void *obj, prelude_io_t *fd);

int idmef_class_print_json(idmef_class_id_t classid, void *obj, prelude_io_t *fd);

int idmef_class_destroy(idmef_class_id_t classid, void *obj);

prelude_bool_t idmef_class_is_listed(idmef_class_id_t classid);

#ifdef __cplusplus
 }
#endif

#endif /* _LIBPRELUDE_IDMEF_CLASS_H */
