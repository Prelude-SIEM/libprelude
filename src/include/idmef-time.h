/*****
*
* Copyright (C) 2003 Nicolas Delon <delon.nicolas@wanadoo.fr>
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

#ifndef _LIBPRELUDE_IDMEF_TIME_H
#define _LIBPRELUDE_IDMEF_TIME_H

#include <time.h>

#define idmef_object_type_time 1

struct idmef_time {
        int refcount;
        uint32_t sec;
        uint32_t usec;
        int32_t gmt_offset;
};

typedef struct idmef_time idmef_time_t;

idmef_time_t *idmef_time_ref(idmef_time_t *time);
idmef_time_t *idmef_time_new(void);
idmef_time_t *idmef_time_new_from_gettimeofday(void);
idmef_time_t *idmef_time_new_from_string(const char *buf);
idmef_time_t *idmef_time_new_from_ntpstamp(const char *buf);

void idmef_time_destroy_internal(idmef_time_t *time);
void idmef_time_destroy(idmef_time_t *time);

idmef_time_t *idmef_time_clone(const idmef_time_t *src);
int idmef_time_copy(idmef_time_t *dst, const idmef_time_t *src);

void idmef_time_set_from_time(idmef_time_t *time, const time_t *t);
void idmef_time_set_sec(idmef_time_t *time, uint32_t sec);
void idmef_time_set_usec(idmef_time_t *time, uint32_t usec);
void idmef_time_set_gmt_offset(idmef_time_t *time, uint32_t gmtoff);

uint32_t idmef_time_get_sec(const idmef_time_t *time);
uint32_t idmef_time_get_usec(const idmef_time_t *time);
int32_t idmef_time_get_gmt_offset(const idmef_time_t *time);

int idmef_time_set_from_string(idmef_time_t *time, const char *buf);
int idmef_time_set_from_ntpstamp(idmef_time_t *time, const char *buf);

int idmef_time_to_string(const idmef_time_t *time, prelude_string_t *out);
int idmef_time_to_ntpstamp(const idmef_time_t *time, prelude_string_t *out);


#define IDMEF_TIME_MAX_STRING_SIZE   64   /* YYYY-MM-DDThh:mm:ss.ssZ */
#define IDMEF_TIME_MAX_NTPSTAMP_SIZE 22   /* 0xNNNNNNNN.0xNNNNNNNN + \0  */


#endif /* _LIBPRELUDE_IDMEF_TIME_H */
