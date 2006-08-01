/*****
*
* Copyright (C) 2003, 2004, 2005 PreludeIDS Technologies. All Rights Reserved.
* Author: Nicolas Delon <nicolas.delon@prelude-ids.com>
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

#ifndef _LIBPRELUDE_IDMEF_TIME_H
#define _LIBPRELUDE_IDMEF_TIME_H

#include "prelude-config.h"

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#if TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# if HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif

#ifdef __cplusplus
 extern "C" {
#endif

struct idmef_time {
        /* <private> */
        int refcount;
        uint32_t sec;
        uint32_t usec;
        int32_t gmt_offset;
};

typedef struct idmef_time idmef_time_t;

idmef_time_t *idmef_time_ref(idmef_time_t *time);
int idmef_time_new(idmef_time_t **time);

int idmef_time_new_from_time(idmef_time_t **time, const time_t *t);
int idmef_time_new_from_gettimeofday(idmef_time_t **time);
int idmef_time_new_from_string(idmef_time_t **time, const char *buf);
int idmef_time_new_from_ntpstamp(idmef_time_t **time, const char *buf);
int idmef_time_new_from_timeval(idmef_time_t **time, const struct timeval *tv);

void idmef_time_set_from_time(idmef_time_t *time, const time_t *t);
int idmef_time_set_from_gettimeofday(idmef_time_t *time);
int idmef_time_set_from_string(idmef_time_t *time, const char *buf);
int idmef_time_set_from_ntpstamp(idmef_time_t *time, const char *buf);
int idmef_time_set_from_timeval(idmef_time_t *time, const struct timeval *tv);

void idmef_time_destroy_internal(idmef_time_t *time);
void idmef_time_destroy(idmef_time_t *time);

int idmef_time_clone(const idmef_time_t *src, idmef_time_t **dst);
int idmef_time_copy(const idmef_time_t *src, idmef_time_t *dst);

void idmef_time_set_sec(idmef_time_t *time, uint32_t sec);
void idmef_time_set_usec(idmef_time_t *time, uint32_t usec);
void idmef_time_set_gmt_offset(idmef_time_t *time, int32_t gmtoff);

uint32_t idmef_time_get_sec(const idmef_time_t *time);
uint32_t idmef_time_get_usec(const idmef_time_t *time);
int32_t idmef_time_get_gmt_offset(const idmef_time_t *time);

int idmef_time_to_string(const idmef_time_t *time, prelude_string_t *out);
int idmef_time_to_ntpstamp(const idmef_time_t *time, prelude_string_t *out);

int idmef_time_compare(const idmef_time_t *time1, const idmef_time_t *time2);
         
#ifdef __cplusplus
 }
#endif
         
#endif /* _LIBPRELUDE_IDMEF_TIME_H */
