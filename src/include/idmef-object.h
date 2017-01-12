/*****
*
* Copyright (C) 2014-2017 CS-SI. All Rights Reserved.
* Author: Yoann Vandoorselaere <yoannv@gmail.com>
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

#ifndef _LIBPRELUDE_IDMEF_OBJECT_H
#define _LIBPRELUDE_IDMEF_OBJECT_H

#ifdef __cplusplus
  extern "C" {
#endif

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "prelude-io.h"
#include "prelude-list.h"

typedef struct idmef_object idmef_object_t;

idmef_class_id_t idmef_object_get_class(idmef_object_t *obj);

idmef_object_t *idmef_object_ref(idmef_object_t *obj);

void idmef_object_destroy(idmef_object_t *obj);

int idmef_object_compare(idmef_object_t *obj1, idmef_object_t *obj2);

int idmef_object_clone(idmef_object_t *obj, idmef_object_t **dst);

int idmef_object_copy(idmef_object_t *src, idmef_object_t *dst);

int idmef_object_print(idmef_object_t *obj, prelude_io_t *fd);

int idmef_object_print_json(idmef_object_t *obj, prelude_io_t *fd);

void idmef_object_add(prelude_list_t *head, idmef_object_t *obj);

void idmef_object_add_tail(prelude_list_t *head, idmef_object_t *obj);

void idmef_object_del(idmef_object_t *object);

void idmef_object_del_init(idmef_object_t *object);

void *idmef_object_get_list_entry(prelude_list_t *listm);

int idmef_object_new_from_json(idmef_object_t **object, const char * json_message);

#ifdef __cplusplus
  }
#endif

#endif /* _LIBPRELUDE_IDMEF_OBJECT_H */
