/*****
*
* Copyright (C) 2004 Nicolas Delon <delon.nicolas@wanadoo.fr>
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

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <sys/types.h>
#include <stdarg.h>
#include <errno.h>

#include "libmissing.h"
#include "common.h"
#include "prelude-log.h"
#include "prelude-error.h"
#include "prelude-inttypes.h"

#include "idmef.h"
#include "idmef-criterion-value.h"


struct idmef_criterion_value_non_linear_time {
	int year;
	int month;
	int yday;
	int mday;
	int wday;
	int hour;
	int min;
	int sec;
};

enum non_linear_time_elem {
	elem_year,
	elem_month,
	elem_yday,
	elem_mday,
	elem_wday,
	elem_hour,
	elem_min,
	elem_sec
};

struct idmef_criterion_value {
	idmef_criterion_value_type_t type;
	union {
		idmef_value_t *fixed;
		idmef_criterion_value_non_linear_time_t *non_linear_time;
	} val;
};



int idmef_criterion_value_non_linear_time_new(idmef_criterion_value_non_linear_time_t **cv)
{
	idmef_criterion_value_non_linear_time_t *time;

	*cv = time = malloc(sizeof (*time));
	if ( ! time )
		return prelude_error_from_errno(errno);

	time->year = -1;
	time->month = -1;
	time->yday = -1;
	time->mday = -1;
	time->wday = -1;
	time->hour = -1;
	time->min = -1;
	time->sec = -1;

	return 0;
}



static int get_next_key_value_pair(const char **input, char **key, char **value)
{
	char *ptr;

	if ( ! **input )
		return 0;

	ptr = strchr(*input, ':');
	if ( ! ptr )
		return -1;

	*key = strndup(*input, ptr - *input);
	if ( ! *key )
		return prelude_error_from_errno(errno);

	*input = ptr + 1;

	ptr = strchr(*input, ' ');
	if ( ptr ) {
		*value = strndup(*input, ptr - *input);
		if ( ! *value ) {
			free(*key);
			return prelude_error_from_errno(errno);
		}

		*input = ptr;
		while ( **input == ' ' )
			(*input)++;

	} else {
		*value = strdup(*input);
		if ( ! *value ) {
			free(*key);
			return prelude_error_from_errno(errno);
		}

		while ( **input )
			(*input)++;
	}

	return 1;
}



static void tm_to_non_linear_time(idmef_criterion_value_non_linear_time_t *time,
				  struct tm *tm,
				  enum non_linear_time_elem until)
{
	memset(time, -1, sizeof (*time));

	switch ( until ) {
	case elem_sec:
		time->sec = tm->tm_sec;

	case elem_min:
		time->min = tm->tm_min;

	case elem_hour:
		time->hour = tm->tm_hour;

	case elem_wday: case elem_mday: case elem_yday:
		time->mday = tm->tm_mday;

	case elem_month:
		time->month = tm->tm_mon + 1;

	case elem_year:
		time->year = tm->tm_year + 1900;
	}
}


static int is_keyword(const char *keyword, const char *value, int *offset)
{
	size_t keyword_len;

	keyword_len = strlen(keyword);

	if ( strncmp(value, keyword, keyword_len)  != 0 )
		return 0;

	value += keyword_len;

	switch ( *value ) {
	case '+':
		*offset = 1;
		break;

	case '-':
		*offset = -1;
		break;

	case '\0':
		*offset = 0;
		return 1;

	default:
		return -1;
	}

	value += 1;

	*offset *= atoi(value);

	return 1;
}


static int tm_calculate(struct tm *tm, int gmt_offset,
			enum non_linear_time_elem elem, int value)
{
	time_t t;

	switch ( elem ) {
	case elem_sec:
		tm->tm_sec += value;
		break;

	case elem_min:
		tm->tm_min += value;
		break;

	case elem_hour:
		tm->tm_hour += value;
		break;

	case elem_yday: case elem_mday: case elem_wday:
		tm->tm_mday += value;
		break;

	case elem_month:
		tm->tm_mon += value;
		break;

	case elem_year:
		tm->tm_year += value;
		break;
	}

	t = mktime(tm);
	t += gmt_offset;

	if ( ! gmtime_r(&t, tm) )
		return -1;

	return 0;
}



static int get_tm_and_offset(struct tm *tm, int32_t *gmt_offset)
{
	time_t current_time;

	time(&current_time);

	if ( prelude_get_gmt_offset(gmt_offset) < 0 )
		return -1;

	current_time += *gmt_offset;

	memset(tm, 0, sizeof (*tm));
	tm->tm_isdst = -1;

	if ( ! gmtime_r(&current_time, tm) )
		return -1;

	return 0;
}



static int parse_last(idmef_criterion_value_non_linear_time_t *t,
		      enum non_linear_time_elem elem,
		      const char *value)
{
	int ret;
	int value_offset;
	struct tm tm;
	int32_t gmt_offset;

	ret = is_keyword("last", value, &value_offset);
	if ( ret <= 0 )
		return ret;

	if ( get_tm_and_offset(&tm, &gmt_offset) < 0 )
		return -1;

	if ( tm_calculate(&tm, gmt_offset, elem, value_offset - 1) < 0 )
		return -1;

	tm_to_non_linear_time(t, &tm, elem_sec);

	return 1;		
}



static int parse_current(idmef_criterion_value_non_linear_time_t *t,
			 enum non_linear_time_elem elem,
			 const char *value)
{
	int ret;
	int value_offset;
	struct tm tm;
	int32_t gmt_offset;

	ret = is_keyword("current", value, &value_offset);
	if ( ret <= 0 )
		return ret;

	if ( get_tm_and_offset(&tm, &gmt_offset) < 0 )
		return -1;

	if ( tm_calculate(&tm, gmt_offset, elem, value_offset) < 0 )
		return -1;

	tm_to_non_linear_time(t, &tm, elem);

	return 1;
}



static int parse_keyword(idmef_criterion_value_non_linear_time_t *t,
			 enum non_linear_time_elem elem,
			 const char *value)
{
	int ret;

	ret = parse_current(t, elem, value);
	if ( ret != 0 )
		return ret;

	return parse_last(t, elem, value);
}



static int parse_year(idmef_criterion_value_non_linear_time_t *time, const char *value)
{
	time->year = atoi(value);

	return 0;
}



static int parse_month(idmef_criterion_value_non_linear_time_t *time, const char *value)
{
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
	int i;

	for ( i = 0; i < sizeof (months) / sizeof (months[0]); i++ ) {
		if ( strcasecmp(value, months[i]) == 0 || atoi(value) == i + 1 ) {
			time->month = i + 1;
			return 0;
		}
	}

	return -1;
}



static int parse_yday(idmef_criterion_value_non_linear_time_t *time, const char *value)
{
	idmef_criterion_value_non_linear_time_set_yday(time, atoi(value));

	return 1;
}



static int parse_mday(idmef_criterion_value_non_linear_time_t *time, const char *value)
{
	idmef_criterion_value_non_linear_time_set_mday(time, atoi(value));

	return 1;
}



static int parse_wday(idmef_criterion_value_non_linear_time_t *time, const char *value)
{
	const char *days[] = {
		"monday",
		"tuesday",
		/* happy days ! ;) */
		"wednesday",
		"thursday",
		"friday",
		"saturday",
		"sunday"
	};
	int i;

	for ( i = 0; i < sizeof (days) / sizeof (days[0]); i++ ) {
		if ( strcasecmp(value, days[i]) == 0 || atoi(value) == i + 1 ) {
			idmef_criterion_value_non_linear_time_set_wday(time, i + 1);
			return 0;
		}
	}

	return -1;
}



static int parse_hour(idmef_criterion_value_non_linear_time_t *time, const char *value)
{
	time->hour = atoi(value);

	return 0;
}



static int parse_min(idmef_criterion_value_non_linear_time_t *time, const char *value)
{
	time->min = atoi(value);

	return 0;
}



static int parse_sec(idmef_criterion_value_non_linear_time_t *time, const char *value)
{
	time->sec = atoi(value);

	return 0;
}



static int parse_non_linear_time(idmef_criterion_value_non_linear_time_t *time, const char *buffer)
{
	const struct {
		char *name;
		enum non_linear_time_elem elem;
		int (*func)(idmef_criterion_value_non_linear_time_t *time, const char *value);
	} keys[] = {
		{ "year",	elem_year,	parse_year	},
		{ "month",	elem_month,	parse_month	},
		{ "yday",	elem_yday,	parse_yday	},
		{ "day",	elem_mday,	parse_mday	},
		{ "mday",	elem_mday,	parse_mday	},
		{ "wday",	elem_wday,	parse_wday	},
		{ "hour",	elem_hour,	parse_hour	},
		{ "min",	elem_min,	parse_min	},
		{ "sec",	elem_sec,	parse_sec	}
	};
	int i;
	int ret1, ret2;
	char *key, *value;

	while ( (ret1 = get_next_key_value_pair(&buffer, &key, &value)) > 0 ) {
		ret2 = -1;

		for ( i = 0; i < sizeof (keys) / sizeof (keys[0]); i++ ) {
			if ( strcmp(key, keys[i].name) == 0 ) {
				ret2 = parse_keyword(time, keys[i].elem, value);
				if ( ret2 == 0 )
					ret2 = keys[i].func(time, value);
				break;
			}
		}

		free(key);
		free(value);

		if ( ret2 < 0 )
			return -1;
	}

	return ret1;
}



int idmef_criterion_value_non_linear_time_new_string(idmef_criterion_value_non_linear_time_t **cv, const char *buffer)
{
        int ret;
        
	ret = idmef_criterion_value_non_linear_time_new(cv);
	if ( ret < 0 )
		return ret;

        ret = parse_non_linear_time(*cv, buffer);
        if ( ret < 0 ) {
		idmef_criterion_value_non_linear_time_destroy(*cv);
		return ret;
	}

	return 0;
}



void idmef_criterion_value_non_linear_time_destroy(idmef_criterion_value_non_linear_time_t *time)
{
	free(time);
}



int idmef_criterion_value_non_linear_time_clone(const idmef_criterion_value_non_linear_time_t *src,
                                                idmef_criterion_value_non_linear_time_t **dst)
{
	*dst = malloc(sizeof(**dst));
	if ( ! dst )
		return prelude_error_from_errno(errno);

	memcpy(*dst, src, sizeof(**dst));

	return 0;
}


#define idmef_criterion_value_non_linear_time_set(name)							\
void idmef_criterion_value_non_linear_time_set_ ## name (idmef_criterion_value_non_linear_time_t *time,	\
							 int name)					\
{													\
	time->name = name;										\
}

idmef_criterion_value_non_linear_time_set(year)
idmef_criterion_value_non_linear_time_set(month)
idmef_criterion_value_non_linear_time_set(hour)
idmef_criterion_value_non_linear_time_set(min)
idmef_criterion_value_non_linear_time_set(sec)



void idmef_criterion_value_non_linear_time_set_yday(idmef_criterion_value_non_linear_time_t *time, int yday)
{
	time->yday = yday;
	time->mday = -1;
	time->wday = -1;
}



void idmef_criterion_value_non_linear_time_set_mday(idmef_criterion_value_non_linear_time_t *time, int mday)
{
	time->mday = mday;
	time->yday = -1;
	time->wday = -1;
}



void idmef_criterion_value_non_linear_time_set_wday(idmef_criterion_value_non_linear_time_t *time, int wday)
{
	time->wday = wday;
	time->yday = -1;
	time->mday = -1;
}



#define idmef_criterion_value_non_linear_time_get(name)							\
int idmef_criterion_value_non_linear_time_get_ ## name (idmef_criterion_value_non_linear_time_t *time)	\
{													\
	return time->name;										\
}

idmef_criterion_value_non_linear_time_get(year)
idmef_criterion_value_non_linear_time_get(month)
idmef_criterion_value_non_linear_time_get(yday)
idmef_criterion_value_non_linear_time_get(mday)
idmef_criterion_value_non_linear_time_get(wday)
idmef_criterion_value_non_linear_time_get(hour)
idmef_criterion_value_non_linear_time_get(min)
idmef_criterion_value_non_linear_time_get(sec)



int idmef_criterion_value_non_linear_time_to_string(idmef_criterion_value_non_linear_time_t *time, prelude_string_t *out)
{
        int ret = 0;
	int have_print = 0;

	if ( time->year != -1 ) {
                ret = prelude_string_sprintf(out, "year:%d", time->year);
		have_print = 1;
	}

	if ( time->month != -1 ) {
		ret = prelude_string_sprintf(out, "%smonth:%d", have_print ? " " : "", time->month);
		have_print = 1;
	}

	if ( time->yday != -1 ) {
		ret = prelude_string_sprintf(out, "%syday:%d", have_print ? " " : "", time->yday);
		have_print = 1;
	}

	if ( time->mday != -1 ) {
		ret = prelude_string_sprintf(out, "%smday:%d", have_print ? " " : "", time->mday);
		have_print = 1;
	}

	if ( time->wday != -1 ) {
		ret = prelude_string_sprintf(out, "%swday:%d", have_print ? " " : "", time->wday);
		have_print = 1;
	}

	if ( time->hour != -1 ) {
		ret = prelude_string_sprintf(out, "%shour:%d", have_print ? " " : "", time->hour);
		have_print = 1;
	}

	if ( time->min != -1 ) {
		ret = prelude_string_sprintf(out, "%smin:%d", have_print ? " " : "", time->min);
		have_print = 1;
	}

	if ( time->sec != -1 ) {
		ret = prelude_string_sprintf(out, "%ssec:%d", have_print ? " " : "", time->sec);
		have_print = 1;
	}

	return ret;
}



int idmef_criterion_value_new_fixed(idmef_criterion_value_t **cv, idmef_value_t *fixed)
{
	*cv = malloc(sizeof(**cv));
	if ( ! *cv )
		return prelude_error_from_errno(errno);

	(*cv)->val.fixed = fixed;
	(*cv)->type = IDMEF_CRITERION_VALUE_TYPE_FIXED;
        
	return 0;
}



int idmef_criterion_value_new_non_linear_time(idmef_criterion_value_t **cv, idmef_criterion_value_non_linear_time_t *time)
{
	*cv = malloc(sizeof(**cv));
	if ( ! *cv )
		return prelude_error_from_errno(errno);

        (*cv)->val.non_linear_time = time;
	(*cv)->type = IDMEF_CRITERION_VALUE_TYPE_NON_LINEAR_TIME;

	return 0;
}



static int build_fixed_value(idmef_criterion_value_t **cv, idmef_path_t *path, const char *buf)
{
        int ret;
	idmef_value_t *fixed;

	ret = idmef_value_new_from_path(&fixed, path, buf);
	if ( ret < 0 )
		return ret;

	ret = idmef_criterion_value_new_fixed(cv, fixed);
	if ( ret < 0 ) {
		idmef_value_destroy(fixed);
		return ret;
	}

	return 0;
}



static int build_non_linear_time_value(idmef_criterion_value_t **cv, const char *buf)
{
        int ret;
	idmef_criterion_value_non_linear_time_t *time;

	ret = idmef_criterion_value_non_linear_time_new_string(&time, buf);
	if ( ret < 0 )
		return ret;

	ret = idmef_criterion_value_new_non_linear_time(cv, time);
	if ( ret < 0 ) {
		idmef_criterion_value_non_linear_time_destroy(time);
		return ret;
	}

	return 0;
}



int idmef_criterion_value_new_generic(idmef_criterion_value_t **cv, idmef_path_t *path, const char *buf)
{
	if ( idmef_path_get_value_type(path) == IDMEF_VALUE_TYPE_TIME && isalpha((int) *buf) )
		return build_non_linear_time_value(cv, buf);

	return build_fixed_value(cv, path, buf);
}



void idmef_criterion_value_destroy(idmef_criterion_value_t *value)
{
	switch ( value->type ) {
                
	case IDMEF_CRITERION_VALUE_TYPE_FIXED:
		idmef_value_destroy(value->val.fixed);
		break;

	case IDMEF_CRITERION_VALUE_TYPE_NON_LINEAR_TIME:
		idmef_criterion_value_non_linear_time_destroy(value->val.non_linear_time);
		break;
	}

	free(value);
}



int idmef_criterion_value_clone(const idmef_criterion_value_t *src, idmef_criterion_value_t **dst)
{
        int ret;
        idmef_value_t *fixed;
        idmef_criterion_value_non_linear_time_t *time;
        
	switch ( src->type ) {

        case IDMEF_CRITERION_VALUE_TYPE_FIXED:
		ret = idmef_value_clone(src->val.fixed, &fixed);
		if ( ret < 0 )
			return ret;

		ret = idmef_criterion_value_new_fixed(dst, fixed);
		if ( ret < 0 ) {
			idmef_value_destroy(fixed);
			return ret;
		}

		break;

	case IDMEF_CRITERION_VALUE_TYPE_NON_LINEAR_TIME:
		ret = idmef_criterion_value_non_linear_time_clone(src->val.non_linear_time, &time);
		if ( ret < 0 )
			return ret;

		ret = idmef_criterion_value_new_non_linear_time(dst, time);
		if ( ret < 0 ) {
			idmef_criterion_value_non_linear_time_destroy(time);
			return ret;
		}
                
		break;

        default:
                return -1;
	}
        
	return ret;
}



idmef_criterion_value_type_t idmef_criterion_value_get_type(const idmef_criterion_value_t *value)
{
	return value->type;
}



idmef_value_t *idmef_criterion_value_get_fixed(idmef_criterion_value_t *value)
{
	return (value->type == IDMEF_CRITERION_VALUE_TYPE_FIXED) ? value->val.fixed : NULL;
}



idmef_criterion_value_non_linear_time_t *idmef_criterion_value_get_non_linear_time(idmef_criterion_value_t *value)
{
	return (value->type == IDMEF_CRITERION_VALUE_TYPE_NON_LINEAR_TIME) ? value->val.non_linear_time : NULL;
}



int idmef_criterion_value_print(idmef_criterion_value_t *value, prelude_io_t *fd)
{
        int ret = -1;
        prelude_string_t *out;
        
        ret = prelude_string_new(&out);
        if ( ret < 0 )
                return ret;
        
	switch ( value->type ) {

        case IDMEF_CRITERION_VALUE_TYPE_FIXED:
                ret = idmef_value_to_string(value->val.fixed, out);
                break;
                
        case IDMEF_CRITERION_VALUE_TYPE_NON_LINEAR_TIME:
		ret = idmef_criterion_value_non_linear_time_to_string(value->val.non_linear_time, out);
		break;
	}

        if ( ret < 0 )
                return ret;
        
        ret = prelude_io_write(fd, prelude_string_get_string(out), prelude_string_get_len(out));
        prelude_string_destroy(out);

        return ret;
}



int idmef_criterion_value_to_string(idmef_criterion_value_t *value, prelude_string_t *out)
{
        int ret = -1;
        
        switch ( value->type ) {

        case IDMEF_CRITERION_VALUE_TYPE_FIXED:
                ret = idmef_value_to_string(value->val.fixed, out);
		break;

	case IDMEF_CRITERION_VALUE_TYPE_NON_LINEAR_TIME:
		ret = idmef_criterion_value_non_linear_time_to_string(value->val.non_linear_time, out);
		break;
	}

	return ret;
}
