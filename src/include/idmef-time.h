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
idmef_time_t *idmef_time_new_gettimeofday(void);
idmef_time_t *idmef_time_new_string(const char *buf);
idmef_time_t *idmef_time_new_ntp_timestamp(const char *buf);
idmef_time_t *idmef_time_new_db_timestamp(const char *buf);

void idmef_time_destroy_internal(idmef_time_t *time);
void idmef_time_destroy(idmef_time_t *time);

idmef_time_t *idmef_time_clone(const idmef_time_t *src);
int idmef_time_copy(idmef_time_t *dst, idmef_time_t *src);

void idmef_time_set_sec(idmef_time_t *time, uint32_t sec);
void idmef_time_set_usec(idmef_time_t *time, uint32_t usec);
void idmef_time_set_gmt_offset(idmef_time_t *time, uint32_t gmtoff);

double idmef_time_get_time(const idmef_time_t *time);
uint32_t idmef_time_get_sec(const idmef_time_t *time);
uint32_t idmef_time_get_usec(const idmef_time_t *time);
int32_t idmef_time_get_gmt_offset(const idmef_time_t *time);

int idmef_time_set_string(idmef_time_t *time, const char *buf);
int idmef_time_set_ntp_timestamp(idmef_time_t *time, const char *buf);
int idmef_time_set_db_timestamp(idmef_time_t *time, const char *buf);

int idmef_time_get_ntp_timestamp(const idmef_time_t *time, char *outptr, size_t size);
int idmef_time_get_timestamp(const idmef_time_t *time, char *outptr, size_t size);
int idmef_time_get_db_timestamp(const idmef_time_t *time, char *outptr, size_t size);
int idmef_time_get_idmef_timestamp(const idmef_time_t *time, char *outptr, size_t size);

#endif /* _LIBPRELUDE_IDMEF_TIME_H */
