/*****
*
* Copyright (C) 2003 Yoann Vandoorselaere <yoann@prelude-ids.org>
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

#ifndef _LIBPRELUDE_IDMEF_STRING_H
#define _LIBPRELUDE_IDMEF_STRING_H

#include "idmef-data.h"

typedef idmef_data_t idmef_string_t;


/*
 * Make sure that this ID does not conflict with any in idmef-tree-wrap.h
 */
#define idmef_type_string 0


idmef_string_t *idmef_string_new(void);

idmef_string_t *idmef_string_new_nodup(char *str);

idmef_string_t *idmef_string_new_ref(const char *str);

idmef_string_t *idmef_string_new_dup(const char *str);

idmef_string_t *idmef_string_new_dup_fast(const char *str, size_t len);

void idmef_string_destroy(idmef_string_t *string);

void idmef_string_destroy_internal(idmef_string_t *string);

idmef_string_t *idmef_string_new_nodup_fast(char *str, size_t len);

idmef_string_t *idmef_string_new_ref_fast(const char *str, int len);

int idmef_string_set_dup_fast(idmef_string_t *string, const char *str, size_t len);

int idmef_string_set_dup(idmef_string_t *string, const char *str);

int idmef_string_set_nodup_fast(idmef_string_t *string, char *str, size_t len);

int idmef_string_set_nodup(idmef_string_t *string, char *str);

int idmef_string_set_ref_fast(idmef_string_t *string, const char *str, size_t len);

int idmef_string_set_ref(idmef_string_t *string, const char *str);

int idmef_string_copy_ref(idmef_string_t *dst, const idmef_string_t *src);

int idmef_string_copy_dup(idmef_string_t *dst, const idmef_string_t *src);

idmef_string_t *idmef_string_clone(const idmef_string_t *src);

size_t idmef_string_get_len(idmef_string_t *string);

char *idmef_string_get_string(const idmef_string_t *string);

int idmef_string_is_empty(const idmef_string_t *string);


/*
 * FIXME: backward compatibility
 */
#define idmef_string idmef_string_get_string
#define idmef_string_len idmef_string_get_len


                                                         
#define idmef_string_set_constant(istr, str) 			\
	idmef_string_set_ref_fast(istr, (str), sizeof((str)))

#define idmef_string_new_constant(str)				\
	idmef_string_new_ref_fast((str), sizeof((str)))
                                                         
#endif /* _LIBPRELUDE_IDMEF_STRING_H */
