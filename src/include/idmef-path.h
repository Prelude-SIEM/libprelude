/*****
*
*
* Copyright (C) 2002, 2003, 2004, 2005 PreludeIDS Technologies. All Rights Reserved.
* Author: Yoann Vandoorselaere <yoann.v@prelude-ids.com>
* Author: Krzysztof Zaraska <kzaraska@student.uci.agh.edu.pl>
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

#ifndef _LIBPRELUDE_IDMEF_PATH_H
#define _LIBPRELUDE_IDMEF_PATH_H

typedef struct idmef_path idmef_path_t;

#include <stdarg.h>
#include "idmef-value.h"
#include "idmef-tree-wrap.h"

int idmef_path_get(idmef_path_t *path, idmef_message_t *message, idmef_value_t **ret);

int idmef_path_set(idmef_path_t *path, idmef_message_t *message, idmef_value_t *value);

int idmef_path_new(idmef_path_t **path, const char *format, ...);
int idmef_path_new_v(idmef_path_t **path, const char *format, va_list args);
int idmef_path_new_fast(idmef_path_t **path, const char *buffer);

idmef_class_id_t idmef_path_get_class(const idmef_path_t *path, int depth);

idmef_value_type_id_t idmef_path_get_value_type(const idmef_path_t *path, int depth);

int idmef_path_set_index(idmef_path_t *path, unsigned int depth, unsigned int index);

int idmef_path_undefine_index(idmef_path_t *path, unsigned int depth);

int idmef_path_get_index(const idmef_path_t *path, unsigned int depth);

int idmef_path_make_child(idmef_path_t *path, const char *child_name, unsigned int index);

int idmef_path_make_parent(idmef_path_t *path);

void idmef_path_destroy(idmef_path_t *path);

int idmef_path_ncompare(const idmef_path_t *p1, const idmef_path_t *p2, unsigned int depth);

int idmef_path_compare(const idmef_path_t *p1, const idmef_path_t *p2);

int idmef_path_clone(const idmef_path_t *src, idmef_path_t **dst);

idmef_path_t *idmef_path_ref(idmef_path_t *path);

const char *idmef_path_get_name(const idmef_path_t *path, int depth);

prelude_bool_t idmef_path_is_ambiguous(idmef_path_t *path);

int idmef_path_has_lists(idmef_path_t *path);

unsigned int idmef_path_get_depth(const idmef_path_t *path);

#endif /* _LIBPRELUDE_IDMEF_PATH_H */
