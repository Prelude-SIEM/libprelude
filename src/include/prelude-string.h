/*****
*
* Copyright (C) 2004-2016 CS-SI. All Rights Reserved.
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
* You should have received a copy of the GNU General Public License along
* with this program; if not, write to the Free Software Foundation, Inc.,
* 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*
*****/

#ifndef _LIBPRELUDE_PRELUDE_STRING_H
#define _LIBPRELUDE_PRELUDE_STRING_H

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdarg.h>

#include "prelude-list.h"
#include "prelude-macros.h"
#include "prelude-inttypes.h"
#include "prelude-linked-object.h"

#ifdef __cplusplus
 extern "C" {
#endif


typedef struct prelude_string prelude_string_t;


int prelude_string_new(prelude_string_t **string);

int prelude_string_new_nodup(prelude_string_t **string, char *str);

int prelude_string_new_ref(prelude_string_t **string, const char *str);

int prelude_string_new_dup(prelude_string_t **string, const char *str);

int prelude_string_new_dup_fast(prelude_string_t **string, const char *str, size_t len);

void prelude_string_destroy(prelude_string_t *string);

void prelude_string_destroy_internal(prelude_string_t *string);

int prelude_string_new_nodup_fast(prelude_string_t **string, char *str, size_t len);

int prelude_string_new_ref_fast(prelude_string_t **string, const char *str, size_t len);

int prelude_string_set_dup_fast(prelude_string_t *string, const char *buf, size_t len);

int prelude_string_set_dup(prelude_string_t *string, const char *buf);

int prelude_string_set_nodup_fast(prelude_string_t *string, char *buf, size_t len);

int prelude_string_set_nodup(prelude_string_t *string, char *buf);

int prelude_string_set_ref_fast(prelude_string_t *string, const char *buf, size_t len);

int prelude_string_set_ref(prelude_string_t *string, const char *buf);

int prelude_string_copy_ref(const prelude_string_t *src, prelude_string_t *dst);

int prelude_string_copy_dup(const prelude_string_t *src, prelude_string_t *dst);

prelude_string_t *prelude_string_ref(prelude_string_t *string);

int prelude_string_clone(const prelude_string_t *src, prelude_string_t **dst);

size_t prelude_string_get_len(const prelude_string_t *string);

const char *prelude_string_get_string_or_default(const prelude_string_t *string, const char *def);

const char *prelude_string_get_string(const prelude_string_t *string);

int prelude_string_get_string_released(prelude_string_t *string, char **outptr);

prelude_bool_t prelude_string_is_empty(const prelude_string_t *string);

int prelude_string_truncate(prelude_string_t *string, size_t len);

void prelude_string_clear(prelude_string_t *string);

/*
 * string operation
 */
int prelude_string_cat(prelude_string_t *dst, const char *str);
int prelude_string_ncat(prelude_string_t *dst, const char *str, size_t len);

int prelude_string_sprintf(prelude_string_t *string, const char *fmt, ...)
                           PRELUDE_FMT_PRINTF(2, 3);

int prelude_string_vprintf(prelude_string_t *string, const char *fmt, va_list ap)
                           PRELUDE_FMT_PRINTF(2, 0);

int prelude_string_compare(const prelude_string_t *str1, const prelude_string_t *str2);

#define prelude_string_set_constant(string, str)                         \
        prelude_string_set_ref_fast((string), (str), strlen((str)))

#define prelude_string_new_constant(string, str)                         \
        prelude_string_new_ref_fast((string), (str), strlen((str)))

#ifdef __cplusplus
 }
#endif

#endif /* _LIBPRELUDE_PRELUDE_STRING_H */
