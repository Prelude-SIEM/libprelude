/*****
*
* Copyright (C) 2003,2004 Nicolas Delon <nicolas@prelude-ids.org>
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


#include <sys/types.h>
#include <stdlib.h>
#include <sys/time.h>

#ifdef __linux__
# define __USE_XOPEN
#endif

#include <time.h>
#include <ctype.h>
#include <string.h>
#include <stdio.h>

#include "libmissing.h"
#include "prelude-inttypes.h"
#include "prelude-string.h"
#include "prelude-error.h"
#include "ntp.h"
#include "prelude-log.h"
#include "common.h"
#include "idmef-time.h"



static char *parse_time_ymd(struct tm *tm, const char *buf)
{
        char *ptr;

        ptr = strptime(buf, "%Y-%m-%d", tm);
        if ( ! ptr )
                return NULL;

        /*
         * The IDMEF draft only permits the 'T' variant here
         */
        while ( isspace((int) *ptr) )
                ptr++;
        
        if ( *ptr == 'T' )
                ptr++;

        return ptr;
}



static char *parse_time_hmsu(struct tm *tm, uint32_t *usec, const char *buf)
{
        char *ptr;
        int fraction;

        ptr = strptime(buf, "%H:%M:%S", tm);
        if ( ! ptr )
                return NULL;

        if ( *ptr == '.' || *ptr == ',' ) {
                ptr++;

                if ( sscanf(ptr, "%u", &fraction) < 1 )
                        return NULL;

                *usec = fraction * 10000;

                while ( isdigit((int) *ptr) )
                        ptr++;
        }

        return ptr;
}



static int parse_time_gmt(struct tm *tm, uint32_t *gmtoff, const char *buf)
{
        int ret;
        unsigned int offset_hour, offset_min;

        /*
         * if UTC, do nothing.
         */
        if ( *buf == 'Z' )
                return 0;
        
        ret = sscanf(buf + 1, "%2u:%2u", &offset_hour, &offset_min);
        if ( ret != 2 )
                return -1;

        if ( *buf == '+' ) {
                tm->tm_min -= offset_min;
                tm->tm_hour -= offset_hour;
		*gmtoff = offset_hour * 3600 + offset_min * 60;
        }

        else if ( *buf == '-' ) {
                tm->tm_min += offset_min;
                tm->tm_hour += offset_hour;
		*gmtoff = - (offset_hour * 3600 + offset_min * 60);
        }
        
        else
                return -1;
        
        return 0;
}



/*
 * Convert a string to a time. The input format is the one specified
 * by the IDMEF draft (v. 0.10, section 3.2.6), with the following
 * changes:
 *
 *  - Date and time can be separated with a white space, instead of 'T'
 *
 *  - With timezone specification, only the first two digits are analyzed
 *    (e.g. both +01:23 and +0123 are valid, but both evaluate to UTC+01). 
 *
 *  - Timezone specification may be skipped (will assume UTC). 
 */
int idmef_time_set_from_string(idmef_time_t *time, const char *buf)
{
        char *ptr;
        struct tm tm;
        int is_localtime = 1, ret;
        
        memset(&tm, 0, sizeof(tm));
        tm.tm_isdst = -1;

        ptr = parse_time_ymd(&tm, buf);
        if ( ! ptr )
                return -1;
        
        if ( *ptr ) {
                ptr = parse_time_hmsu(&tm, &time->usec, ptr);
                if ( ! ptr )
                        return -1;

                if ( *ptr ) {
                        ret = parse_time_gmt(&tm, &time->gmt_offset, ptr);
                        if ( ret < 0 )
                                return -1;

                        is_localtime = 0;
                }
        }

        time->sec = is_localtime ? mktime(&tm) : prelude_timegm(&tm);

        return 0;
}



int idmef_time_new_from_string(idmef_time_t **time, const char *buf)
{
        int ret;
        
        ret = idmef_time_new(time);
        if ( ret < 0 )
                return ret;

        ret = idmef_time_set_from_string(*time, buf);
        if ( ret < 0 ) {
                free(*time);
                return ret;
        }

        return 0;
}



int idmef_time_set_from_ntpstamp(idmef_time_t *time, const char *buf)
{
        l_fp ts;
        struct timeval tv;
        unsigned ts_mask = TS_MASK;
        unsigned ts_roundbit = TS_ROUNDBIT;
        
        
        if ( sscanf(buf, "%x.%x", &ts.l_ui, &ts.l_uf) < 2 )
                return -1;

        /* 
         * This transformation is a reverse form of the one found in
         *  idmef_get_ntp_timestamp()
         */
        ts.l_ui -= JAN_1970;
        ts.l_uf -= ts_roundbit;
        ts.l_uf &= ts_mask;
        TSTOTV(&ts, &tv);

	time->sec = tv.tv_sec;
        time->usec = tv.tv_usec;

        return 0;
}




int idmef_time_to_ntpstamp(const idmef_time_t *time, prelude_string_t *out)
{
        l_fp ts;
        struct timeval tv;
        unsigned ts_mask = TS_MASK;             /* defaults to 20 bits (us) */
        unsigned ts_roundbit = TS_ROUNDBIT;     /* defaults to 20 bits (us) */
        int ret;

	tv.tv_sec = idmef_time_get_sec(time);
        tv.tv_usec = idmef_time_get_usec(time);

        sTVTOTS(&tv, &ts);

        ts.l_ui += JAN_1970;                    /* make it time since 1900 */
        ts.l_uf += ts_roundbit;
        ts.l_uf &= ts_mask;
        
        ret = prelude_string_sprintf(out, "0x%08lx.0x%08lx", (unsigned long) ts.l_ui, (unsigned long) ts.l_uf);

        return ret;
}



/**
 * idmef_time_to_string:
 * @time: Pointer to an IDMEF time structure.
 * @out: Pointer to a #prelude_string_t output buffer.
 *
 * Translate @time to an user readable string following the IDMEF
 * specification.
 *
 * Returns: number of bytes written on success, a negative value if an error occured.
 */
int idmef_time_to_string(const idmef_time_t *time, prelude_string_t *out)
{
        time_t t;
        struct tm utc;
        uint32_t hour_off, min_off, sec_off;

	t = time->sec + time->gmt_offset;	

        if ( ! gmtime_r((const time_t *) &t, &utc) )
                return prelude_error_from_errno(errno);

	hour_off = time->gmt_offset / 3600;
	min_off = time->gmt_offset % 3600 / 60;
	sec_off = time->gmt_offset % 60;
        
        return prelude_string_sprintf(out, "%d-%.2d-%.2dT%.2d:%.2d:%.2d.%02u+%.2d:%.2d",
                                      utc.tm_year + 1900, utc.tm_mon + 1, utc.tm_mday,
                                      utc.tm_hour, utc.tm_min, utc.tm_sec,
                                      idmef_time_get_usec(time) / 10000 % 60,
                                      hour_off, min_off);
}




int idmef_time_new_from_ntpstamp(idmef_time_t **time, const char *buf)
{
        int ret;

        ret = idmef_time_new(time);
        if ( ret < 0 )
                return ret;

        ret = idmef_time_set_from_ntpstamp(*time, buf);
        if ( ret < 0 ) {
                free(*time);
                return ret;
        }

        return 0; 
}



int idmef_time_set_from_timeval(idmef_time_t *time, const struct timeval *tv)
{
        int ret;
        uint32_t gmtoff;

        ret = prelude_get_gmt_offset(&gmtoff);
        if ( ret < 0 )
                return ret;

        time->sec = tv->tv_sec;
        time->usec = tv->tv_usec;
        time->gmt_offset = gmtoff;

        return 0;
}



int idmef_time_new_from_timeval(idmef_time_t **time, const struct timeval *tv)
{
        int ret;
	
        ret = idmef_time_new(time);
        if ( ret < 0 )
                return ret;

        return idmef_time_set_from_timeval(*time, tv);
}



int idmef_time_new_from_gettimeofday(idmef_time_t **time)
{
        int ret;
        struct timeval tv;

        ret = gettimeofday(&tv, NULL);
        if ( ret < 0 )
                return prelude_error_from_errno(errno);

        return idmef_time_new_from_timeval(time, &tv);
}




idmef_time_t *idmef_time_ref(idmef_time_t *time)
{
        time->refcount++;
        return time;
}




int idmef_time_new(idmef_time_t **time)
{
        *time = calloc(1, sizeof(**time));
        if ( ! *time )
                return prelude_error_from_errno(errno);
        
        (*time)->refcount = 1;
        
        return 0;
}



int idmef_time_clone(const idmef_time_t *src, idmef_time_t **dst)
{
        int ret;
        
        ret = idmef_time_new(dst);
        if ( ret < 0 )
                return ret;

	return idmef_time_copy(src, *dst);
}




void idmef_time_set_gmt_offset(idmef_time_t *time, int32_t gmtoff)
{
        time->gmt_offset = gmtoff;
}



void idmef_time_set_sec(idmef_time_t *time, uint32_t sec)
{
        time->sec = sec;
}



void idmef_time_set_from_time(idmef_time_t *time, const time_t *t)
{
	uint32_t gmtoff;

	prelude_get_gmt_offset(&gmtoff);
	
	time->gmt_offset = gmtoff;
	time->sec = *t;
}



void idmef_time_set_usec(idmef_time_t *time, uint32_t usec)
{
        time->usec = usec;
}



int32_t idmef_time_get_gmt_offset(const idmef_time_t *time)
{
        return time->gmt_offset;
}



uint32_t idmef_time_get_sec(const idmef_time_t *time)
{
        return time->sec;
}




uint32_t idmef_time_get_usec(const idmef_time_t *time)
{
        return time->usec;
}



int idmef_time_copy(const idmef_time_t *src, idmef_time_t *dst)
{
        dst->sec = src->sec;
        dst->usec = src->usec;
	dst->gmt_offset = src->gmt_offset;
        
        return 0;
}




void idmef_time_destroy_internal(idmef_time_t *time)
{
        /* nop */
}



void idmef_time_destroy(idmef_time_t *time)
{
        if ( ! time || --time->refcount )
                return;

        free(time);
}


