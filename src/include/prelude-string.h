/*****
*
* Copyright (C) 2004-2005,2006,2007 PreludeIDS Technologies. All Rights Reserved.
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

#ifndef _LIBPRELUDE_PRELUDE_STRING_H
#define _LIBPRELUDE_PRELUDE_STRING_H

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdarg.h>

#include "prelude-list.h"
#include "prelude-inttypes.h"

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


struct prelude_string {
        prelude_list_t list;

        int flags;
        int refcount;

        union {
                char *rwbuf;
                const char *robuf;
        } data;

        size_t size;
        size_t index;
};



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

void prelude_string_clear(prelude_string_t *string);

/*
 * string operation
 */
int prelude_string_cat(prelude_string_t *dst, const char *str);
int prelude_string_ncat(prelude_string_t *dst, const char *str, size_t len);

int prelude_string_sprintf(prelude_string_t *string, const char *fmt, ...)
                           __attribute__ ((__format__ (__printf__, 2, 3)));

int prelude_string_vprintf(prelude_string_t *string, const char *fmt, va_list ap)
                           __attribute__ ((__format__ (__printf__, 2, 0)));

int prelude_string_compare(const prelude_string_t *str1, const prelude_string_t *str2);

#define prelude_string_set_constant(string, str)                         \
        prelude_string_set_ref_fast((string), (str), strlen((str)))

#define prelude_string_new_constant(string, str)                         \
        prelude_string_new_ref_fast((string), (str), strlen((str)))

#ifdef __cplusplus
 }
#endif

#endif /* _LIBPRELUDE_PRELUDE_STRING_H */
