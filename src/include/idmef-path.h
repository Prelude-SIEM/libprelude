/*****
*
*
* Copyright (C) 2002, 2003, 2004, 2005 PreludeIDS Technologies. All Rights Reserved.
* Author: Yoann Vandoorselaere <yoann.v@prelude-ids.com>
* Author: Krzysztof Zaraska <kzaraska@student.uci.agh.edu.pl>
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

#ifndef _LIBPRELUDE_IDMEF_PATH_H
#define _LIBPRELUDE_IDMEF_PATH_H

#ifdef __cplusplus
 extern "C" {
#endif

#ifndef __attribute__
/* This feature is available in gcc versions 2.5 and later.  */
# if __GNUC__ < 2 || (__GNUC__ == 2 && __GNUC_MINOR__ < 5) || __STRICT_ANSI__
#  define __attribute__(Spec) /* empty */
# endif
/* The __-protected variants of `format' and `printf' attributes
   are accepted by gcc versions 2.6.4 (effectively 2.7) and later.  */
# if __GNUC__ < 2 || (__GNUC__ == 2 && __GNUC_MINOR__ < 7)
#  define __format__ format
#  define __printf__ printf
# endif
#endif

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

typedef struct idmef_path idmef_path_t;

#include <stdarg.h>
#include "idmef-value.h"
#include "idmef-tree-wrap.h"

int idmef_path_get(const idmef_path_t *path, idmef_message_t *message, idmef_value_t **ret);

int idmef_path_set(const idmef_path_t *path, idmef_message_t *message, idmef_value_t *value);

int idmef_path_new(idmef_path_t **path, const char *format, ...)
                   __attribute__ ((__format__ (__printf__, 2, 3)));

int idmef_path_new_v(idmef_path_t **path, const char *format, va_list args)
                     __attribute__ ((__format__ (__printf__, 2, 0)));

int idmef_path_new_fast(idmef_path_t **path, const char *buffer);

idmef_class_id_t idmef_path_get_class(const idmef_path_t *path, int depth);

idmef_value_type_id_t idmef_path_get_value_type(const idmef_path_t *path, int depth);

int idmef_path_set_index(idmef_path_t *path, unsigned int depth, int index);

int idmef_path_undefine_index(idmef_path_t *path, unsigned int depth);

int idmef_path_get_index(const idmef_path_t *path, unsigned int depth);

int idmef_path_make_child(idmef_path_t *path, const char *child_name, int index);

int idmef_path_make_parent(idmef_path_t *path);

void idmef_path_destroy(idmef_path_t *path);

int idmef_path_ncompare(const idmef_path_t *p1, const idmef_path_t *p2, unsigned int depth);

int idmef_path_compare(const idmef_path_t *p1, const idmef_path_t *p2);

int idmef_path_clone(const idmef_path_t *src, idmef_path_t **dst);

idmef_path_t *idmef_path_ref(idmef_path_t *path);

const char *idmef_path_get_name(const idmef_path_t *path, int depth);

prelude_bool_t idmef_path_is_ambiguous(const idmef_path_t *path);

int idmef_path_has_lists(const idmef_path_t *path);

prelude_bool_t idmef_path_is_list(const idmef_path_t *path, int depth);

unsigned int idmef_path_get_depth(const idmef_path_t *path);

int idmef_path_check_operator(const idmef_path_t *path, idmef_criterion_operator_t op);

int idmef_path_get_applicable_operators(const idmef_path_t *path, idmef_criterion_operator_t *result);

#ifndef SWIG
void _idmef_path_cache_lock(void);

void _idmef_path_cache_reinit(void);

void _idmef_path_cache_unlock(void);

void _idmef_path_cache_destroy(void);
#endif

#ifdef __cplusplus
 }
#endif

#endif /* _LIBPRELUDE_IDMEF_PATH_H */
