/*****
*
* Copyright (C) 2004,2005,2006,2007 PreludeIDS Technologies. All Rights Reserved.
* Author: Nicolas Delon <nicolas.delon@prelude-ids.com>
* Author: Yoann Vandoorselaere <yoann@prelude-ids.com>
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

#include "config.h"
#include "libmissing.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <regex.h>
#include <stdarg.h>
#include <errno.h>

#include "common.h"
#include "prelude-log.h"
#include "prelude-error.h"
#include "prelude-inttypes.h"

#include "idmef.h"
#include "idmef-criterion-value.h"




struct match_cb {
        unsigned int nmatch;
        unsigned int match;
        idmef_criterion_value_t *cv;
        idmef_criterion_operator_t operator;
};


struct regex_value {
        regex_t regex;
        char *regex_string;
};



struct idmef_criterion_value {

        void *value;
        prelude_bool_t value_need_free;
        idmef_criterion_value_type_t type;

        int (*clone)(const idmef_criterion_value_t *cv, idmef_criterion_value_t *dst);
        int (*print)(const idmef_criterion_value_t *cv, prelude_io_t *fd);
        int (*to_string)(const idmef_criterion_value_t *cv, prelude_string_t *out);
        int (*match)(const idmef_criterion_value_t *cv, idmef_criterion_operator_t operator, idmef_value_t *value);
        void (*destroy)(idmef_criterion_value_t *cv);
};



/*
 * time stuff
 */
static int btime_parse_simple(const char *integer, int *out)
{
        *out = atoi(integer);
        return 0;
}




static int btime_parse_year(const char *integer, int *out)
{
        *out = atoi(integer) - 1900;
        return 0;
}



static int btime_parse_month(const char *value, int *out)
{
        int i;
        const char *months[] = {
                "january",
                "february",
                "march",
                "april",
                "may",
                "june",
                "july",
                "august",
                "september",
                "october",
                "november",
                "december"
        };

        if ( isdigit((int) *value) ) {
                *out = atoi(value) - 1;
                return 0;
        }

        for ( i = 0; i < (int) (sizeof(months) / sizeof(*months)); i++ ) {
                if ( strcasecmp(value, months[i]) == 0 ) {
                        *out = i; /* Numbered 0 to 11 */
                        return 0;
                }
        }

        return -1;
}


static int btime_parse_wday(const char *value, int *out)
{
        int i;
        const char *days[] = {
                "sunday",
                "monday",
                "tuesday",
                /* happy days ! ;) */
                "wednesday",
                "thursday",
                "friday",
                "saturday",
        };

        if ( isdigit((int) *value) ) {
                *out = atoi(value) - 1;
                return 0;
        }

        for ( i = 0; i < (int) (sizeof(days) / sizeof(*days)); i++ ) {

                if ( strcasecmp(value, days[i]) == 0 ) {
                        *out = i; /* Numbered 0 (sunday) to 6 */
                        return 0;
                }
        }

        return -1;
}


static int do_btime_match(const idmef_criterion_value_t *cv, idmef_criterion_operator_t op, idmef_value_t *value)
{
        time_t sec, wanted, matched;
        struct tm lt, *comp = cv->value;

        if ( idmef_value_get_type(value) != IDMEF_VALUE_TYPE_TIME )
                return -1;

        sec = idmef_time_get_sec(idmef_value_get_time(value));
        if ( ! gmtime_r(&sec, &lt) )
                return prelude_error_from_errno(errno);

        /*
         * Apply mask
         */
        if ( comp->tm_sec < 0 ) lt.tm_sec = -1;
        if ( comp->tm_min < 0 ) lt.tm_min = -1;
        if ( comp->tm_mon < 0 ) lt.tm_mon = -1;
        if ( comp->tm_hour < 0 ) lt.tm_hour = -1;
        if ( comp->tm_mday < 0 ) lt.tm_mday = -1;
        if ( comp->tm_year < 0 ) lt.tm_year = -1;
        if ( comp->tm_wday < 0 ) lt.tm_wday = -1;
        if ( comp->tm_yday < 0 ) lt.tm_yday = -1;

        wanted  = timegm(comp);
        matched = timegm(&lt);

        if ( op & IDMEF_CRITERION_OPERATOR_EQUAL && matched == wanted )
                return 1;

        if ( op & IDMEF_CRITERION_OPERATOR_LESSER && matched < wanted)
                return 1;

        if ( op & IDMEF_CRITERION_OPERATOR_GREATER && matched > wanted )
                return 1;

        return 0;
}



static int btime_match(const idmef_criterion_value_t *cv, idmef_criterion_operator_t operator, idmef_value_t *value)
{
        int ret;

        ret = do_btime_match(cv, operator, value);
        if ( ret < 0 )
                return ret;

        if ( operator & IDMEF_CRITERION_OPERATOR_NOT )
                return (ret == 1) ? 0 : 1;
        else
                return (ret == 1) ? 1 : 0;
}



static int btime_to_string(const idmef_criterion_value_t *cv, prelude_string_t *out)
{
        struct tm *lt = cv->value;

        if ( lt->tm_year != -1 )
                prelude_string_sprintf(out, "year:%d ", lt->tm_year + 1900);

        if ( lt->tm_yday != -1 )
                prelude_string_sprintf(out, "yday:%d ", lt->tm_yday);

        if ( lt->tm_mon != -1 )
                prelude_string_sprintf(out, "month:%d ", lt->tm_mon);

        if ( lt->tm_mday != -1 )
                prelude_string_sprintf(out, "mday:%d ", lt->tm_mday);

        if ( lt->tm_wday != -1 )
                prelude_string_sprintf(out, "wday:%d ", lt->tm_wday);

        if ( lt->tm_hour != -1 )
                prelude_string_sprintf(out, "hour:%d ", lt->tm_hour);

        if ( lt->tm_min != -1 )
                prelude_string_sprintf(out, "min:%d ", lt->tm_min);

        if ( lt->tm_sec != -1 )
                prelude_string_sprintf(out, "sec:%d ", lt->tm_sec);

        return 0;
}


static int btime_print(const idmef_criterion_value_t *cv, prelude_io_t *fd)
{
        int ret;
        prelude_string_t *out;

        ret = prelude_string_new(&out);
        if ( ret < 0 )
                return ret;

        ret = btime_to_string(cv, out);
        if ( ret < 0 ) {
                prelude_string_destroy(out);
                return ret;
        }

        ret = prelude_io_write(fd, prelude_string_get_string(out), prelude_string_get_len(out));
        prelude_string_destroy(out);

        return ret;
}



static int btime_clone(const idmef_criterion_value_t *src, idmef_criterion_value_t *dst)
{
        /*
         * The API does not allow to change the value of an existing
         * idmef_criterion_value_t object, so we can simply regerence the source.
         */
        dst->value_need_free = FALSE;
        dst->value = src->value;
        return 0;
}



static void btime_destroy(idmef_criterion_value_t *cv)
{
        if ( cv->value_need_free )
                free(cv->value);
}




/*
 * regex stuff
 */
static int regex_match(const idmef_criterion_value_t *cv, idmef_criterion_operator_t operator, idmef_value_t *value)
{
        int ret;
        const char *str = NULL;
        idmef_class_id_t class;
        struct regex_value *rv = cv->value;

        if ( idmef_value_get_type(value) == IDMEF_VALUE_TYPE_STRING )
                str = prelude_string_get_string(idmef_value_get_string(value));

        else if ( idmef_value_get_type(value) == IDMEF_VALUE_TYPE_ENUM ) {
                class = idmef_value_get_class(value);
                str = idmef_class_enum_to_string(class, idmef_value_get_enum(value));
        }

        else if ( idmef_value_get_type(value) == IDMEF_VALUE_TYPE_DATA ) {
                idmef_data_t *data;

                data = idmef_value_get_data(value);
                if ( idmef_data_get_type(data) == IDMEF_DATA_TYPE_CHAR_STRING )
                        str = idmef_data_get_data(data);
        }

        if ( ! str )
                return 0;

        ret = regexec(&rv->regex, str, 0, NULL, 0);
        if ( operator & IDMEF_CRITERION_OPERATOR_NOT )
                return (ret == REG_NOMATCH) ? 1 : 0;
        else
                return (ret != REG_NOMATCH) ? 1 : 0;
}



static int regex_print(const idmef_criterion_value_t *cv, prelude_io_t *fd)
{
        struct regex_value *rv = cv->value;
        prelude_io_write(fd, rv->regex_string, strlen(rv->regex_string));
        return 0;
}



static int regex_to_string(const idmef_criterion_value_t *cv, prelude_string_t *out)
{
        struct regex_value *rv = cv->value;
        return prelude_string_cat(out, rv->regex_string);
}



static int regex_clone(const idmef_criterion_value_t *src, idmef_criterion_value_t *dst)
{
        /*
         * The API does not allow to change the value of an existing
         * idmef_criterion_value_t object, so we can simply regerence the source.
         */
        dst->value_need_free = FALSE;
        dst->value = src->value;
        return 0;
}



static void regex_destroy(idmef_criterion_value_t *cv)
{
        struct regex_value *rv;

        if ( ! cv->value_need_free )
                return;

        rv = cv->value;

        free(rv->regex_string);
        regfree(&rv->regex);
        free(rv);
}




/*
 * value stuff
 */
static int value_match(const idmef_criterion_value_t *cv, idmef_criterion_operator_t operator, idmef_value_t *value)
{
        return idmef_value_match(value, cv->value, operator);
}



static int value_print(const idmef_criterion_value_t *cv, prelude_io_t *fd)
{
        return idmef_value_print(cv->value, fd);
}



static int value_to_string(const idmef_criterion_value_t *cv, prelude_string_t *out)
{
        return idmef_value_to_string(cv->value, out);
}



static int value_clone(const idmef_criterion_value_t *cv, idmef_criterion_value_t *dst)
{
        int ret;
        idmef_value_t *res;

        ret = idmef_value_clone(cv->value, &res);
        dst->value = res;

        return ret;
}



static void value_destroy(idmef_criterion_value_t *cv)
{
        idmef_value_destroy(cv->value);
}



/*
 *
 */
static int do_match_cb(idmef_value_t *value, void *extra)
{
        int ret;
        struct match_cb *mcb = extra;
        idmef_criterion_value_t *cv = mcb->cv;
        idmef_criterion_operator_t operator = mcb->operator;

        if ( idmef_value_is_list(value) )
                return idmef_value_iterate(value, do_match_cb, mcb);

        ret = cv->match(cv, operator, value);
        if ( ret < 0 )
                return ret;

        if ( ret > 0 )
                mcb->match++;
        else
                mcb->nmatch++;

        return 0;
}


void idmef_criterion_value_destroy(idmef_criterion_value_t *value)
{
        value->destroy(value);
        free(value);
}



int idmef_criterion_value_clone(const idmef_criterion_value_t *src, idmef_criterion_value_t **dst)
{
        int ret;

        ret = idmef_criterion_value_new(dst);
        if ( ret < 0 )
                return ret;

        (*dst)->type = src->type;
        (*dst)->clone = src->clone;
        (*dst)->print = src->print;
        (*dst)->to_string = src->to_string;
        (*dst)->match = src->match;
        (*dst)->destroy = src->destroy;

        ret = src->clone(src, *dst);
        if ( ret < 0 ) {
                free(*dst);
                return ret;
        }

        return 0;
}



int idmef_criterion_value_print(idmef_criterion_value_t *cv, prelude_io_t *fd)
{
        return cv->print(cv, fd);
}



int idmef_criterion_value_to_string(idmef_criterion_value_t *cv, prelude_string_t *out)
{
        return cv->to_string(cv, out);
}



int idmef_criterion_value_match(idmef_criterion_value_t *cv, idmef_value_t *value,
                                idmef_criterion_operator_t op)
{
        int ret;
        struct match_cb mcb;

        mcb.cv = cv;
        mcb.operator = op;
        mcb.match = mcb.nmatch = 0;

        ret = idmef_value_iterate(value, do_match_cb, &mcb);
        if ( ret < 0 )
                return ret;

        /*
         * When comparing a path to a value using a negative operator, if the
         * excluded value was found at least once, return a negative match.
         *
         * This is required since the 'match' value might also be positive
         * in case the path contain a list of value.
         */
        if ( op & IDMEF_CRITERION_OPERATOR_NOT && mcb.nmatch > 0 )
                return 0;

        return mcb.match;
}



int idmef_criterion_value_new(idmef_criterion_value_t **cv)
{
        *cv = calloc(1, sizeof(**cv));
        if ( ! *cv )
                return prelude_error_from_errno(errno);

        (*cv)->value_need_free = TRUE;

        return 0;
}



static int btime_parse_gmtoff(const char *param, int *out)
{
        *out = atoi(param) * 60 * 60;
        return 0;
}


static int btime_parse(struct tm *lt, const char *time)
{
        int ret;
        size_t i;
        long gmt_offset;
        char *end = NULL;
        struct {
                const char *field;
                size_t len;
                void *ptr;
                int (*func)(const char *param, int *output);
        } tbl[] = {
                { "month", 5, &lt->tm_mon, btime_parse_month  },
                { "wday", 4, &lt->tm_wday, btime_parse_wday   },
                { "year", 4, &lt->tm_year, btime_parse_year   },
                { "mday", 4, &lt->tm_mday, btime_parse_simple },
                { "yday", 4, &lt->tm_yday, btime_parse_simple },
                { "hour", 4, &lt->tm_hour, btime_parse_simple },
                { "min", 3, &lt->tm_min, btime_parse_simple   },
                { "sec", 3, &lt->tm_sec, btime_parse_simple   },
                { "gmtoff", 6, &gmt_offset, btime_parse_gmtoff }
        };

        ret = prelude_get_gmt_offset(&gmt_offset);
        if ( ret < 0 )
                return ret;

        do {
                for ( i = 0; i < sizeof(tbl) / sizeof(*tbl); i++ ) {

                        if ( strncmp(time, tbl[i].field, tbl[i].len) != 0 )
                                continue;

                        if ( time[tbl[i].len] != ':' )
                                continue;

                        time += tbl[i].len + 1;

                        end = strchr(time, ' ');
                        if ( end ) {
                                *end++ = 0;
                                while ( *end == ' ' ) end++;
                        }

                        ret = tbl[i].func(time, tbl[i].ptr);
                        if ( ret < 0 )
                                return -1;

                        time = end + 1;
                        break;
                }

                if ( i == sizeof(tbl) / sizeof(*tbl) )
                        return -1;

                time = end;
        } while ( time );

        if ( lt->tm_hour != -1 )
                lt->tm_hour -= (gmt_offset / (60 * 60));

        return 0;
}




int idmef_criterion_value_new_broken_down_time(idmef_criterion_value_t **cv, const char *time, idmef_criterion_operator_t op)
{
        int ret;
        struct tm *lt;

        ret = idmef_criterion_value_new(cv);
        if ( ret < 0 )
                return ret;

        lt = malloc(sizeof(struct tm));
        if ( ! lt ) {
                free(*cv);
                return prelude_error_from_errno(errno);
        }

        memset(lt, -1, sizeof(*lt));

        ret = btime_parse(lt, time);
        if ( ret < 0 ) {
                free(lt);
                free(*cv);
                return ret;
        }

        (*cv)->value = lt;
        (*cv)->match = btime_match;
        (*cv)->clone = btime_clone;
        (*cv)->print = btime_print;
        (*cv)->destroy = btime_destroy;
        (*cv)->to_string = btime_to_string;
        (*cv)->type = IDMEF_CRITERION_VALUE_TYPE_BROKEN_DOWN_TIME;

        return 0;
}




int idmef_criterion_value_new_regex(idmef_criterion_value_t **cv, const char *regex, idmef_criterion_operator_t op)
{
        int ret;
        struct regex_value *rv;
        int flags = REG_EXTENDED|REG_NOSUB;

        ret = idmef_criterion_value_new(cv);
        if ( ret < 0 )
                return ret;

        rv = (*cv)->value = malloc(sizeof(*rv));
        if ( ! rv ) {
                free(*cv);
                return prelude_error_from_errno(errno);
        }

        rv->regex_string = strdup(regex);
        if ( ! rv->regex_string ) {
                free(rv);
                free(*cv);
                return prelude_error_from_errno(errno);
        }

        if ( op & IDMEF_CRITERION_OPERATOR_NOCASE )
                flags |= REG_ICASE;

        ret = regcomp(&rv->regex, rv->regex_string, flags);
        if ( ret != 0 ) {
                char errbuf[1024];

                regerror(ret, &rv->regex, errbuf, sizeof(errbuf));

                free(rv->regex_string);
                free(rv);
                free(*cv);

                return prelude_error_verbose(PRELUDE_ERROR_IDMEF_CRITERION_INVALID_REGEX,
                                             "error compiling regex: %s", errbuf);
        }

        (*cv)->match = regex_match;
        (*cv)->clone = regex_clone;
        (*cv)->print = regex_print;
        (*cv)->destroy = regex_destroy;
        (*cv)->to_string = regex_to_string;
        (*cv)->type = IDMEF_CRITERION_VALUE_TYPE_REGEX;

        return 0;
}



int idmef_criterion_value_new_value(idmef_criterion_value_t **cv,
                                    idmef_value_t *value, idmef_criterion_operator_t op)
{
        int ret;

        ret = idmef_value_check_operator(value, op);
        if ( ret < 0 )
                return ret;

        ret = idmef_criterion_value_new(cv);
        if ( ret < 0 )
                return ret;

        (*cv)->value = value;
        (*cv)->match = value_match;
        (*cv)->clone = value_clone;
        (*cv)->print = value_print;
        (*cv)->destroy = value_destroy;
        (*cv)->to_string = value_to_string;
        (*cv)->type = IDMEF_CRITERION_VALUE_TYPE_VALUE;

        return 0;
}




int idmef_criterion_value_new_from_string(idmef_criterion_value_t **cv,
                                          idmef_path_t *path, const char *value,
                                          idmef_criterion_operator_t operator)
{
        int ret;
        idmef_value_t *val;
        idmef_value_type_id_t tid;

        tid = idmef_path_get_value_type(path, -1);

        if ( tid == IDMEF_VALUE_TYPE_TIME ) {
                ret = idmef_criterion_value_new_broken_down_time(cv, value, operator);
                if ( ret == 0 )
                        return ret;
        }

        else if ( operator & IDMEF_CRITERION_OPERATOR_REGEX &&
                  (tid == IDMEF_VALUE_TYPE_STRING || tid == IDMEF_VALUE_TYPE_ENUM || tid == IDMEF_VALUE_TYPE_DATA) )
                return idmef_criterion_value_new_regex(cv, value, operator);

        /*
         * It's more understandable for the user if we check the operator
         * prior to checking the value.
         */
        ret = idmef_value_type_check_operator(tid, operator);
        if ( ret < 0 )
                return ret;

        if ( tid == IDMEF_VALUE_TYPE_ENUM && operator & IDMEF_CRITERION_OPERATOR_SUBSTR )
                ret = idmef_value_new_from_string(&val, IDMEF_VALUE_TYPE_STRING, value);
        else
                ret = idmef_value_new_from_path(&val, path, value);

        if ( ret < 0 )
                return ret;

        ret = idmef_criterion_value_new_value(cv, val, operator);
        if ( ret < 0 ) {
                idmef_value_destroy(val);
                return ret;
        }

        return 0;
}



const idmef_value_t *idmef_criterion_value_get_value(idmef_criterion_value_t *cv)
{
        return cv->type == IDMEF_CRITERION_VALUE_TYPE_VALUE ? cv->value : NULL;
}



const struct tm *idmef_criterion_value_get_broken_down_time(idmef_criterion_value_t *cv)
{
        return cv->type == IDMEF_CRITERION_VALUE_TYPE_BROKEN_DOWN_TIME ? cv->value : NULL;
}



const char *idmef_criterion_value_get_regex(idmef_criterion_value_t *cv)
{
        return cv->type == IDMEF_CRITERION_VALUE_TYPE_REGEX ? ((struct regex_value *)cv->value)->regex_string : NULL;
}


idmef_criterion_value_type_t idmef_criterion_value_get_type(idmef_criterion_value_t *cv)
{
        return cv->type;
}
