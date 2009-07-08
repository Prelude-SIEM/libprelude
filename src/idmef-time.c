/*****
*
* Copyright (C) 2003-2005,2006,2007 PreludeIDS Technologies. All Rights Reserved.
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

/*
 * This is required on Solaris so that multiple call to
 * strptime() won't reset the tm structure.
 */
#define _STRPTIME_DONTZERO

#include "config.h"
#include "libmissing.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/types.h>

#if defined(__linux__) && ! defined(__USE_XOPEN)
# define __USE_XOPEN
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
        unsigned int fraction;

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



static int parse_time_gmt(struct tm *tm, int32_t *gmtoff, const char *buf)
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



/**
 * idmef_time_set_from_string:
 * @time: Pointer to an #idmef_time_t object.
 * @buf: Pointer to a string describing a time in an IDMEF conforming format.
 *
 * Fills @time object with information retrieved from the user provided
 * @buf, containing a string describing a time in a format conforming
 * to the IDMEF definition (v. 0.10, section 3.2.6).
 *
 * Additionally, the provided time might be separated with white spaces,
 * instead of the IDMEF defined 'T' character.
 *
 * If there is no UTC offset specified, we assume that the provided
 * time is local, and compute the GMT offset by ourselve.
 *
 * Returns: 0 on success, a negative value if an error occured.
 */
int idmef_time_set_from_string(idmef_time_t *time, const char *buf)
{
        int ret;
        char *ptr;
        struct tm tm;
        prelude_bool_t miss_gmt = TRUE;

        prelude_return_val_if_fail(time, prelude_error(PRELUDE_ERROR_ASSERTION));
        prelude_return_val_if_fail(buf, prelude_error(PRELUDE_ERROR_ASSERTION));

        memset(&tm, 0, sizeof(tm));
        tm.tm_isdst = -1;

        ptr = parse_time_ymd(&tm, buf);
        if ( ! ptr )
                return prelude_error_verbose(PRELUDE_ERROR_GENERIC, "error parsing date field, format should be: YY-MM-DD");

        if ( *ptr ) {
                ptr = parse_time_hmsu(&tm, &time->usec, ptr);
                if ( ! ptr )
                        return prelude_error_verbose(PRELUDE_ERROR_GENERIC, "error parsing time field, format should be: HH:MM:SS");

                if ( *ptr ) {
                        ret = parse_time_gmt(&tm, &time->gmt_offset, ptr);
                        if ( ret < 0 )
                                return prelude_error_verbose(PRELUDE_ERROR_GENERIC, "error parsing GMT offset field (Z)?(+|-)?HH:MM");

                        miss_gmt = FALSE;
                }
        }

        if ( miss_gmt ) {
                long gmtoff;
                prelude_get_gmt_offset_from_tm(&tm, &gmtoff);
                time->gmt_offset = (int32_t) gmtoff;
        }

        time->sec = miss_gmt ? mktime(&tm) : prelude_timegm(&tm);
        return 0;
}



/**
 * idmef_time_new_from_string:
 * @time: Address where to store the created #idmef_time_t object.
 * @buf: Pointer to a string describing a time in an IDMEF conforming format.
 *
 * Creates an #idmef_time_t object filled with information retrieved
 * from the user provided @buf, containing a string describing a time in a format
 * conforming to the IDMEF definition  (v. 0.10, section 3.2.6).
 *
 * Additionally, the provided time might be separated with white spaces, instead
 * of the IDMEF define 'T' character. The format might not specify a timezone
 * (will assume UTC in this case).
 *
 * The resulting #idmef_time_t object is stored in @time.
 *
 * Returns: 0 on success, a negative value if an error occured.
 */
int idmef_time_new_from_string(idmef_time_t **time, const char *buf)
{
        int ret;

        prelude_return_val_if_fail(buf, prelude_error(PRELUDE_ERROR_ASSERTION));

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



/**
 * idmef_time_set_from_ntpstamp:
 * @time: Pointer to a #idmef_time_t object.
 * @buf: Pointer to a string containing an NTP timestamp.
 *
 * Fills the @time object with information provided within the @buf NTP timestamp.
 *
 * Returns: 0 on success, a negative value if an error occured.
 */
int idmef_time_set_from_ntpstamp(idmef_time_t *time, const char *buf)
{
        l_fp ts;
        struct timeval tv;
        unsigned ts_mask = TS_MASK;
        unsigned ts_roundbit = TS_ROUNDBIT;

        prelude_return_val_if_fail(time, prelude_error(PRELUDE_ERROR_ASSERTION));
        prelude_return_val_if_fail(buf, prelude_error(PRELUDE_ERROR_ASSERTION));

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
        time->gmt_offset = 0;

        return 0;
}



/**
 * idmef_time_to_ntpstamp:
 * @time: Pointer to an IDMEF time structure.
 * @out: Pointer to a #prelude_string_t output buffer.
 *
 * Translates @time to an user readable NTP timestamp string,
 * conforming to the IDMEF defined time format.
 *
 * Returns: number of bytes written on success, a negative value if an error occured.
 */
int idmef_time_to_ntpstamp(const idmef_time_t *time, prelude_string_t *out)
{
        l_fp ts;
        struct timeval tv;
        unsigned ts_mask = TS_MASK;             /* defaults to 20 bits (us) */
        unsigned ts_roundbit = TS_ROUNDBIT;     /* defaults to 20 bits (us) */
        int ret;

        prelude_return_val_if_fail(time, prelude_error(PRELUDE_ERROR_ASSERTION));
        prelude_return_val_if_fail(out, prelude_error(PRELUDE_ERROR_ASSERTION));

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
 * Translates @time to an user readable string conforming to the IDMEF
 * defined time format.
 *
 * Returns: number of bytes written on success, a negative value if an error occured.
 */
int idmef_time_to_string(const idmef_time_t *time, prelude_string_t *out)
{
        time_t t;
        struct tm utc;
        uint32_t hour_off, min_off, sec_off;

        prelude_return_val_if_fail(time, prelude_error(PRELUDE_ERROR_ASSERTION));
        prelude_return_val_if_fail(out, prelude_error(PRELUDE_ERROR_ASSERTION));

        t = time->sec + time->gmt_offset;

        if ( ! gmtime_r((const time_t *) &t, &utc) )
                return prelude_error_from_errno(errno);

        hour_off = time->gmt_offset / 3600;
        min_off = time->gmt_offset % 3600 / 60;
        sec_off = time->gmt_offset % 60;

        return prelude_string_sprintf(out, "%d-%.2d-%.2dT%.2d:%.2d:%.2d.%02u%+.2d:%.2d",
                                      utc.tm_year + 1900, utc.tm_mon + 1, utc.tm_mday,
                                      utc.tm_hour, utc.tm_min, utc.tm_sec, idmef_time_get_usec(time),
                                      hour_off, min_off);
}




/**
 * idmef_time_new_from_ntpstamp:
 * @time: Address where to store the created #idmef_time_t object.
 * @buf: Pointer to a string containing an NTP timestamp.
 *
 * Creates an #idmef_time_t object filled with information provided
 * from the @buf NTP timestamp, and stores it in @time.
 *
 * Returns: 0 on success, a negative value if an error occured.
 */
int idmef_time_new_from_ntpstamp(idmef_time_t **time, const char *buf)
{
        int ret;

        prelude_return_val_if_fail(buf, prelude_error(PRELUDE_ERROR_ASSERTION));

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



/**
 * idmef_time_set_from_timeval:
 * @time: Pointer to an #idmef_time_t object.
 * @tv: Pointer to a struct timeval (see gettimeofday()).
 *
 * Fills @time object filled with information provided within the @tv structure.
 *
 * Returns: 0 on success, a negative value if an error occured.
 */
int idmef_time_set_from_timeval(idmef_time_t *time, const struct timeval *tv)
{
        int ret;
        long gmtoff;

        prelude_return_val_if_fail(time, prelude_error(PRELUDE_ERROR_ASSERTION));
        prelude_return_val_if_fail(tv, prelude_error(PRELUDE_ERROR_ASSERTION));

        ret = prelude_get_gmt_offset_from_time((const time_t *) &tv->tv_sec, &gmtoff);
        if ( ret < 0 )
                return ret;

        time->sec = tv->tv_sec;
        time->usec = tv->tv_usec;
        time->gmt_offset = (int32_t) gmtoff;

        return 0;
}



/**
 * idmef_time_new_from_timeval:
 * @time: Address where to store the created #idmef_time_t object.
 * @tv: Pointer to a struct timeval (see gettimeofday()).
 *
 * Creates an #idmef_time_t object filled with information provided
 * within the @tv structure.
 *
 * Returns: 0 on success, a negative value if an error occured.
 */
int idmef_time_new_from_timeval(idmef_time_t **time, const struct timeval *tv)
{
        int ret;

        prelude_return_val_if_fail(tv, prelude_error(PRELUDE_ERROR_ASSERTION));

        ret = idmef_time_new(time);
        if ( ret < 0 )
                return ret;

        return idmef_time_set_from_timeval(*time, tv);
}



/**
 * idmef_time_set_from_gettimeofday:
 * @time: Pointer to an #idmef_time_t object.
 *
 * Fills @time with information retrieved using gettimeofday().
 *
 * Returns: 0 on success, a negative value if an error occured.
 */
int idmef_time_set_from_gettimeofday(idmef_time_t *time)
{
        int ret;
        struct timeval tv;

        prelude_return_val_if_fail(time, prelude_error(PRELUDE_ERROR_ASSERTION));

        ret = gettimeofday(&tv, NULL);
        if ( ret < 0 )
                return prelude_error_from_errno(errno);

        return idmef_time_set_from_timeval(time, &tv);
}



/**
 * idmef_time_new_from_gettimeofday:
 * @time: Address where to store the created #idmef_time_t object.
 *
 * Creates an #idmef_time_t object filled with information retrieved
 * using gettimeofday(), and stores it in @time.
 *
 * Returns: 0 on success, a negative value if an error occured.
 */
int idmef_time_new_from_gettimeofday(idmef_time_t **time)
{
        int ret;
        struct timeval tv;

        ret = gettimeofday(&tv, NULL);
        if ( ret < 0 )
                return prelude_error_from_errno(errno);

        return idmef_time_new_from_timeval(time, &tv);
}



/**
 * idmef_time_ref:
 * @time: Pointer to an #idmef_time_t object.
 *
 * Increases @time reference count.
 * idmef_time_destroy() won't destroy @time until the refcount
 * reach 0.
 *
 * Returns: The @time provided argument.
 */
idmef_time_t *idmef_time_ref(idmef_time_t *time)
{
        prelude_return_val_if_fail(time, NULL);

        time->refcount++;
        return time;
}



/**
 * idmef_time_new:
 * @time: Address where to store the created #idmef_time_t object.
 *
 * Creates an empty #idmef_time_t object and store it in @time.
 *
 * Returns: 0 on success, a negative value if an error occured.
 */
int idmef_time_new(idmef_time_t **time)
{
        *time = calloc(1, sizeof(**time));
        if ( ! *time )
                return prelude_error_from_errno(errno);

        (*time)->refcount = 1;

        return 0;
}



/**
 * idmef_time_clone:
 * @src: Pointer to a #idmef_time_t to clone.
 * @dst: Address where to store the cloned @src object.
 *
 * Clones @src and stores the result in the @dst address.
 *
 * Returns: 0 on success, a negative value if an error occured.
 */
int idmef_time_clone(const idmef_time_t *src, idmef_time_t **dst)
{
        int ret;

        prelude_return_val_if_fail(src, prelude_error(PRELUDE_ERROR_ASSERTION));

        ret = idmef_time_new(dst);
        if ( ret < 0 )
                return ret;

        return idmef_time_copy(src, *dst);
}



/**
 * idmef_time_set_from_time:
 * @time: Pointer to an #idmef_time_t object.
 * @t: Pointer to a time_t.
 *
 * Fills @time from the information described by @t.
 * @time won't contain micro seconds information, since theses are not
 * available within @t.
 */
void idmef_time_set_from_time(idmef_time_t *time, const time_t *t)
{
        long gmtoff;

        prelude_return_if_fail(time);
        prelude_return_if_fail(t);

        prelude_get_gmt_offset_from_time(t, &gmtoff);

        time->gmt_offset = (int32_t) gmtoff;
        time->sec = *t;
}



/**
 * idmef_time_new_from_time:
 * @time: Address where to store the created #idmef_time_t object.
 * @t: Pointer to a time_t.
 *
 * Creates a new #idmef_time_t object and store it in @time.
 * This object will be filled with information available in @t. The created
 * @time won't contain micro seconds information, since theses are not
 * available within @t.
 *
 * Returns: 0 on success, a negative value if an error occured.
 */
int idmef_time_new_from_time(idmef_time_t **time, const time_t *t)
{
        int ret;

        prelude_return_val_if_fail(t, prelude_error(PRELUDE_ERROR_ASSERTION));

        ret = idmef_time_new(time);
        if ( ret < 0 )
                return ret;

        idmef_time_set_from_time(*time, t);

        return 0;
}



/**
 * idmef_time_set_gmt_offset:
 * @time: Pointer to a #idmef_time_t.
 * @gmtoff: GMT offset for @time, in seconds.
 *
 * Sets the GMT offset @gmtoff, in seconds, within @time.
 *
 * WARNING: this is just an accessor function, and using it to
 * set @time current time also requires the use of idmef_time_set_sec()
 * and idmef_time_set_usec().
 */
void idmef_time_set_gmt_offset(idmef_time_t *time, int32_t gmtoff)
{
        prelude_return_if_fail(time);
        time->gmt_offset = gmtoff;
}



/**
 * idmef_time_set_sec:
 * @time: Pointer to a #idmef_time_t.
 * @sec: Number of seconds since the Epoch.
 *
 * Sets the number of second from the Epoch to @sec within @time.
 *
 * WARNING: this is just an accessor function, and using it to
 * set @time current time also requires the use of idmef_time_set_usec()
 * and idmef_time_set_gmt_offset().
 */
void idmef_time_set_sec(idmef_time_t *time, uint32_t sec)
{
        prelude_return_if_fail(time);
        time->sec = sec;
}



/**
 * idmef_time_set_usec:
 * @time: Pointer to a #idmef_time_t.
 * @usec: Number of micro seconds to set within @time.
 *
 * Sets the number of micro second to @usec within @time.
 *
 * WARNING: this is just an accessor function, and using it to
 * set @time current time also requires the use of idmef_time_set_sec()
 * and idmef_time_set_gmt_offset().
 */
void idmef_time_set_usec(idmef_time_t *time, uint32_t usec)
{
        prelude_return_if_fail(time);
        time->usec = usec;
}


/**
 * idmef_time_get_gmt_offset:
 * @time: Pointer to a #idmef_time_t.
 *
 * Returns the GMT offset that applies to @time.
 *
 * Returns: The GMT offset, in seconds.
 */
int32_t idmef_time_get_gmt_offset(const idmef_time_t *time)
{
        prelude_return_val_if_fail(time, 0);
        return time->gmt_offset;
}




/**
 * idmef_time_get_sec:
 * @time: Pointer to a #idmef_time_t.
 *
 * Returns the number of second since the Epoch (00:00:00 UTC, January 1, 1970),
 * previously set within @time.
 *
 * Returns: The number of seconds.
 */
uint32_t idmef_time_get_sec(const idmef_time_t *time)
{
        prelude_return_val_if_fail(time, 0);
        return time->sec;
}



/**
 * idmef_time_get_usec:
 * @time: Pointer to a #idmef_time_t.
 *
 * Returns the u-second member of @time.
 *
 * Returns: The number of u-seconds.
 */
uint32_t idmef_time_get_usec(const idmef_time_t *time)
{
        prelude_return_val_if_fail(time, 0);
        return time->usec;
}



/**
 * idmef_time_copy:
 * @src: Pointer to a #idmef_time_t to copy data from.
 * @dst: Pointer to a #idmef_time_t to copy data to.
 *
 * Copies @src internal to @dst.
 *
 * Returns: 0 on success, a negative value if an error occured.
 */
int idmef_time_copy(const idmef_time_t *src, idmef_time_t *dst)
{
        prelude_return_val_if_fail(src, prelude_error(PRELUDE_ERROR_ASSERTION));
        prelude_return_val_if_fail(dst, prelude_error(PRELUDE_ERROR_ASSERTION));

        dst->sec = src->sec;
        dst->usec = src->usec;
        dst->gmt_offset = src->gmt_offset;

        return 0;
}




void idmef_time_destroy_internal(idmef_time_t *time)
{
        /* nop */
}



/**
 * idmef_time_destroy:
 * @time: Pointer to an #idmef_time_t object.
 *
 * Destroys @time if refcount reach 0.
 */
void idmef_time_destroy(idmef_time_t *time)
{
        prelude_return_if_fail(time);

        if ( --time->refcount )
                return;

        free(time);
}



/**
 * idmef_time_compare:
 * @time1: Pointer to an #idmef_time_t object to compare with @time2.
 * @time2: Pointer to an #idmef_time_t object to compare with @time1.
 *
 * Returns: 0 if @time1 and @time2 match, 1 if @time1 is greater than
 * @time2, -1 if @time1 is lesser than @time2.
 */
int idmef_time_compare(const idmef_time_t *time1, const idmef_time_t *time2)
{
        unsigned long t1, t2;

        if ( ! time1 && ! time2 )
                return 0;

        else if ( ! time1 || ! time2 )
                return -1;

        t1 = time1->sec + time1->gmt_offset;
        t2 = time2->sec + time2->gmt_offset;

        if ( t1 == t2 ) {
                if ( time1->usec == time2->usec )
                        return 0;
                else
                        return (time1->usec < time2->usec) ? -1 : 1;
        }

        else return (t1 < t2) ? -1 : 1;
}
