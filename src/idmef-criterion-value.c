/*****
*
* Copyright (C) 2004, 2005 PreludeIDS Technologies. All Rights Reserved.
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


#ifdef HAVE_PCREPOSIX
 #include <pcreposix.h>
#else
 #include "regex.h"
#endif

struct match_cb {
        unsigned int match;
        idmef_criterion_value_t *cv;
        idmef_criterion_operator_t operator;
};


struct idmef_criterion_value {
        
        void *value;
        idmef_criterion_value_type_t type;
        
        int (*clone)(const idmef_criterion_value_t *cv, idmef_criterion_value_t *dst);
        int (*print)(const idmef_criterion_value_t *cv, prelude_io_t *fd);
        int (*to_string)(const idmef_criterion_value_t *cv, prelude_string_t *out);
        int (*match)(const idmef_criterion_value_t *cv, idmef_criterion_operator_t operator, idmef_value_t *value);
        void (*destroy)(idmef_criterion_value_t *cv);
};



/*
 * regex stuff
 */
static int regex_match(const idmef_criterion_value_t *cv, idmef_criterion_operator_t operator, idmef_value_t *value)
{
        const char *str;
        idmef_class_id_t class;
        
        if ( idmef_value_get_type(value) == IDMEF_VALUE_TYPE_STRING )
                str = prelude_string_get_string(idmef_value_get_string(value));

        else if ( idmef_value_get_type(value) == IDMEF_VALUE_TYPE_ENUM ) {
                class = idmef_value_get_class(value);
                str = idmef_class_enum_to_string(class, idmef_value_get_enum(value));
        }

        else return -1;
        
        return ( regexec(cv->value, str, 0, NULL, 0) == 0 ) ? 1 : 0;
}



static int regex_print(const idmef_criterion_value_t *cv, prelude_io_t *fd)
{
        return 0;
}



static int regex_to_string(const idmef_criterion_value_t *cv, prelude_string_t *out)
{
        return 0;
}



static int regex_clone(const idmef_criterion_value_t *src, idmef_criterion_value_t *dst)
{
        dst->value = malloc(sizeof(regex_t));
        if ( ! dst->value )
                return prelude_error_from_errno(errno);

        memcpy(dst->value, src->value, sizeof(regex_t));

        return 0;
}



static void regex_destroy(idmef_criterion_value_t *cv)
{
        regfree(cv->value);
        free(cv->value);
}




/*
 * value stuff
 */
static int value_match(const idmef_criterion_value_t *cv, idmef_criterion_operator_t operator, idmef_value_t *value)
{        
        return idmef_value_match(cv->value, value, operator);
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
        return idmef_value_clone(cv->value, (idmef_value_t **) &dst->value);
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

        /*
         * In case we are matching against a list of value,
         * a single mach is considered as a match. If the match fail
         * we keep trying.
         */
        ret = cv->match(cv, operator, value);
        if ( ret < 0 )
                return ret;

        if ( ret > 0 )
                mcb->match++;
        
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
        mcb.match = 0;
        mcb.operator = op;
        
        ret = idmef_value_iterate(value, do_match_cb, &mcb);        
        if ( ret < 0 )
                return ret;
        
        return mcb.match;
}



int idmef_criterion_value_new(idmef_criterion_value_t **cv)
{
        *cv = calloc(1, sizeof(**cv));
        if ( ! *cv )
                return prelude_error_from_errno(errno);

        return 0;
}



int idmef_criterion_value_new_regex(idmef_criterion_value_t **cv, const char *regex)
{
        int ret;

        ret = idmef_criterion_value_new(cv);
        if ( ret < 0 )
                return ret;

        (*cv)->value = malloc(sizeof(regex_t));
        if ( ! (*cv)->value ) {
                free(*cv);
                return prelude_error_from_errno(errno);
        }

        ret = regcomp((*cv)->value, regex, REG_EXTENDED);
        if ( ret < 0 ) {
                free((*cv)->value);
                free(*cv);
                return prelude_error_from_errno(errno);
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

        if ( operator == IDMEF_CRITERION_OPERATOR_REGEX )
                return idmef_criterion_value_new_regex(cv, value);

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



const void *idmef_criterion_value_get_value(idmef_criterion_value_t *cv)
{
        return cv->value;
}



idmef_criterion_value_type_t idmef_criterion_value_get_type(idmef_criterion_value_t *cv)
{
        return cv->type;
}
