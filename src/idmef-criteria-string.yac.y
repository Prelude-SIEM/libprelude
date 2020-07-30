/*****
*
* Copyright (C) 2003-2020 CS GROUP - France. All Rights Reserved.
* Author: Krzysztof Zaraska <kzaraska@student.uci.agh.edu.pl>
* Author: Nicolas Delon <nicolas.delon@prelude-ids.com>
*
* This file is part of the Prelude library.
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU Lesser General Public License as published by
* the Free Software Foundation; either version 2.1, or (at your option)
* any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU Lesser General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public License along
* with this program; if not, write to the Free Software Foundation, Inc.,
* 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*
*****/

%define api.prefix {_preludeyy}

%{
#include "libmissing.h"
%}

%{
#define PRELUDE_ERROR_SOURCE_DEFAULT PRELUDE_ERROR_SOURCE_IDMEF_CRITERIA

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <stdarg.h>
#include <ctype.h>

#include "glthread/lock.h"

#include "prelude-log.h"
#include "prelude-error.h"
#include "prelude-inttypes.h"

#include "idmef.h"
#include "idmef-criteria.h"
#include "common.h"

static int path_count = 0;
static int real_ret = 0;
static idmef_path_t *cur_path;
static idmef_criteria_t *processed_criteria;
static idmef_criterion_operator_t cur_operator;

gl_lock_t _criteria_parse_mutex = gl_lock_initializer;


#define operator_or 1
#define operator_and 2

extern int yylex(void);
static void _preludeyyerror(const char *s);
extern void *_preludeyy_scan_string(const char *);
extern void _preludeyy_delete_buffer(void *);
void _idmef_criteria_string_init_lexer(void);

#define YYERROR_VERBOSE


static int escape_str(prelude_string_t **out, char *str)
{
        char c;
        int ret;
        size_t i = 0, len = strlen(str);

        ret = prelude_string_new(out);
        if ( ret < 0 )
                return ret;

        for ( i = 0; i < len; ) {
                c = str[i++];
                if ( ! (c == '\\' && i < len) )
                        ret = prelude_string_ncat(*out, &c, 1);
                else {
                        c = str[i++];
                        if ( c == '0' )
                                ret = prelude_string_ncat(*out, "\0", 1);

                        else if ( c == 'a' )
                                ret = prelude_string_ncat(*out, "\a", 1);

                        else if ( c == 'b' )
                                ret = prelude_string_ncat(*out, "\b", 1);

                        else if ( c == 'f' )
                                ret = prelude_string_ncat(*out, "\f", 1);

                        else if ( c == 'n' )
                                ret = prelude_string_ncat(*out, "\n", 1);

                        else if ( c == 'r' )
                                ret = prelude_string_ncat(*out, "\r", 1);

                        else if ( c == 't' )
                                ret = prelude_string_ncat(*out, "\t", 1);

                        else if ( c == 'v' )
                                ret = prelude_string_ncat(*out, "\v", 1);

                        else if ( !(cur_operator & (IDMEF_CRITERION_OPERATOR_SUBSTR|IDMEF_CRITERION_OPERATOR_REGEX)) && c == '\\' )
                                ret = prelude_string_ncat(*out, "\\", 1);

                        else if ( c == 'u' || c == 'U' ) {
                                ret = prelude_unicode_to_string(*out, str + (i - 2), len - (i - 2));
                                if ( ret < 0 )
                                        return ret;

                                i += ret - 2;
                        }

                        else if ( c == 'x' ) {
                                if ( (len - i) < 2 ) {
                                        ret = prelude_error_verbose(PRELUDE_ERROR_GENERIC, "truncated \\xXX escape");
                                        break;
                                }

                                sscanf(&str[i], "%2hhx", &c);
                                ret = prelude_string_ncat(*out, &c, 1);

                                i += 2;
                        }

                        else {
                                char buf[2] = { '\\', c };
                                ret = prelude_string_ncat(*out, buf, 2);
                        }
                }

                if ( ret < 0 )
                        break;
        }

        if ( ret < 0 )
                prelude_string_destroy(*out);

        return ret;
}


static int create_criteria(idmef_criteria_t **criteria, idmef_path_t *path,
                           idmef_criterion_value_t *value, idmef_criterion_operator_t operator)
{
        if ( path_count++ > 0 )
                idmef_path_ref(path);

        real_ret = idmef_criterion_new(criteria, path, value, operator);
        if ( real_ret < 0 )
                goto err;

        return 0;

err:
        idmef_path_destroy(path);
        cur_path = NULL;

        return real_ret;
}


%}

%union {
        char *str;
        int operator;
        idmef_path_t *path;
        idmef_criteria_t *criteria;
        idmef_criteria_operator_t relation;
}

/* BISON Declarations */

%token <str> TOK_IDMEF_RAW_VALUE "<IDMEF-RValue>"
%token <str> TOK_IDMEF_VALUE "<IDMEF-Value>"
%token <str> TOK_IDMEF_PATH "<IDMEF-Path>"

%destructor { free($$); } TOK_IDMEF_RAW_VALUE TOK_IDMEF_VALUE TOK_IDMEF_PATH
%destructor { idmef_criteria_destroy($$); } criteria

%token TOK_RELATION_SUBSTRING "<>"
%token TOK_RELATION_SUBSTRING_NOCASE "<>*"
%token TOK_RELATION_NOT_SUBSTRING "!<>"
%token TOK_RELATION_NOT_SUBSTRING_NOCASE "!<>*"

%token TOK_RELATION_REGEXP "~"
%token TOK_RELATION_REGEXP_NOCASE "~*"
%token TOK_RELATION_NOT_REGEXP "!~"
%token TOK_RELATION_NOT_REGEXP_NOCASE "!~*"

%token TOK_RELATION_GREATER ">"
%token TOK_RELATION_GREATER_OR_EQUAL ">="
%token TOK_RELATION_LESS "<"
%token TOK_RELATION_LESS_OR_EQUAL "<="
%token TOK_RELATION_EQUAL "="
%token TOK_RELATION_EQUAL_NOCASE "=*"
%token TOK_RELATION_NOT_EQUAL "!="
%token TOK_RELATION_NOT_EQUAL_NOCASE "!=*"

%token TOK_OPERATOR_NOT "!"
%token TOK_OPERATOR_AND "&&"
%token TOK_OPERATOR_OR "||"

%token TOK_ERROR

%type <criteria> criteria
%type <criteria> criteria_base
%type <criteria> criteria_and
%type <criteria> criteria_not
%type <criteria> value
%type <criteria> multiple_value
%type <criteria> criterion
%type <path> path
%type <relation> relation
%type <operator> operator


/* Grammar follows */
%%

input:
        criteria {
                processed_criteria = $1;
        }
;


criteria:
        criteria_and {
                $$ = $1;
        }

        | criteria TOK_OPERATOR_OR criteria_and {
               idmef_criteria_or_criteria($1, $3);
               $$ = $1;
        }
;


criteria_and:
        criteria_not {
            $$ = $1;
        }

        | criteria_and TOK_OPERATOR_AND criteria_not {
               idmef_criteria_and_criteria($1, $3);
               $$ = $1;
        }
;


criteria_not:
        criteria_base {
            $$ = $1;
        }

        | TOK_OPERATOR_NOT criteria_not {
                idmef_criteria_t *criteria;

                real_ret = idmef_criteria_join(&criteria, NULL, IDMEF_CRITERIA_OPERATOR_NOT, $2);
                if ( real_ret < 0 )
                        YYABORT;

                $$ = criteria;
        }
;


criteria_base:
        criterion {
                $$ = $1;
        }

        | '(' criteria ')' {
                $$ = $2;
        }
;


criterion:
        path relation '(' multiple_value ')' {
                $$ = $4;
        }

        | path relation value {
                $$ = $3;
        }

        | path {
                idmef_criteria_t *criteria;

                real_ret = create_criteria(&criteria, $1, NULL, IDMEF_CRITERION_OPERATOR_NOT|IDMEF_CRITERION_OPERATOR_NULL);
                if ( real_ret < 0 )
                        YYABORT;

                $$ = criteria;
        }
;


path:
        TOK_IDMEF_PATH {
                real_ret = idmef_path_new_fast(&cur_path, $1);
                free($1);

                if ( real_ret < 0 )
                        YYABORT;

                path_count = 0;
                $$ = cur_path;
        }
;


value:
        TOK_IDMEF_RAW_VALUE {
                idmef_criteria_t *criteria;
                idmef_criterion_value_t *value = NULL;
                prelude_string_t *out;

                real_ret = idmef_criterion_value_new_from_string(&value, cur_path, $1, cur_operator);
                free($1);

                if ( real_ret < 0 )
                        YYABORT;

                real_ret = create_criteria(&criteria, cur_path, value, cur_operator);
                if ( real_ret < 0 )
                        YYABORT;

                $$ = criteria;

        } |

        TOK_IDMEF_VALUE {
                idmef_criteria_t *criteria;
                idmef_criterion_value_t *value = NULL;
                prelude_string_t *out;

                real_ret = escape_str(&out, $1);
                free($1);
                if ( real_ret < 0 )
                        YYABORT;

                real_ret = idmef_criterion_value_new_from_string(&value, cur_path, prelude_string_get_string(out), cur_operator);
                prelude_string_destroy(out);

                if ( real_ret < 0 )
                        YYABORT;

                real_ret = create_criteria(&criteria, cur_path, value, cur_operator);
                if ( real_ret < 0 )
                        YYABORT;

                $$ = criteria;
        }
;



multiple_value:
        multiple_value operator multiple_value {
                if ( $2 == operator_or )
                        idmef_criteria_or_criteria($1, $3);
                else
                        idmef_criteria_and_criteria($1, $3);

                $$ = $1;
        }

        | '(' multiple_value ')' {
                $$ = $2;
        }

        | value {
                $$ = $1;
        }
;


relation:
  TOK_RELATION_SUBSTRING            { cur_operator = $$ = IDMEF_CRITERION_OPERATOR_SUBSTR; }
| TOK_RELATION_SUBSTRING_NOCASE     { cur_operator = $$ = IDMEF_CRITERION_OPERATOR_SUBSTR|IDMEF_CRITERION_OPERATOR_NOCASE; }
| TOK_RELATION_NOT_SUBSTRING        { cur_operator = $$ = IDMEF_CRITERION_OPERATOR_SUBSTR|IDMEF_CRITERION_OPERATOR_NOT; }
| TOK_RELATION_NOT_SUBSTRING_NOCASE { cur_operator = $$ = IDMEF_CRITERION_OPERATOR_SUBSTR|IDMEF_CRITERION_OPERATOR_NOT|IDMEF_CRITERION_OPERATOR_NOCASE; }
| TOK_RELATION_REGEXP               { cur_operator = $$ = IDMEF_CRITERION_OPERATOR_REGEX; }
| TOK_RELATION_REGEXP_NOCASE        { cur_operator = $$ = IDMEF_CRITERION_OPERATOR_REGEX|IDMEF_CRITERION_OPERATOR_NOCASE; }
| TOK_RELATION_NOT_REGEXP           { cur_operator = $$ = IDMEF_CRITERION_OPERATOR_REGEX|IDMEF_CRITERION_OPERATOR_NOT; }
| TOK_RELATION_NOT_REGEXP_NOCASE    { cur_operator = $$ = IDMEF_CRITERION_OPERATOR_REGEX|IDMEF_CRITERION_OPERATOR_NOT|IDMEF_CRITERION_OPERATOR_NOCASE; }
| TOK_RELATION_GREATER              { cur_operator = $$ = IDMEF_CRITERION_OPERATOR_GREATER; }
| TOK_RELATION_GREATER_OR_EQUAL     { cur_operator = $$ = IDMEF_CRITERION_OPERATOR_GREATER|IDMEF_CRITERION_OPERATOR_EQUAL; }
| TOK_RELATION_LESS                 { cur_operator = $$ = IDMEF_CRITERION_OPERATOR_LESSER; }
| TOK_RELATION_LESS_OR_EQUAL        { cur_operator = $$ = IDMEF_CRITERION_OPERATOR_LESSER|IDMEF_CRITERION_OPERATOR_EQUAL; }
| TOK_RELATION_EQUAL                { cur_operator = $$ = IDMEF_CRITERION_OPERATOR_EQUAL; }
| TOK_RELATION_EQUAL_NOCASE         { cur_operator = $$ = IDMEF_CRITERION_OPERATOR_EQUAL|IDMEF_CRITERION_OPERATOR_NOCASE; }
| TOK_RELATION_NOT_EQUAL            { cur_operator = $$ = IDMEF_CRITERION_OPERATOR_EQUAL|IDMEF_CRITERION_OPERATOR_NOT; }
| TOK_RELATION_NOT_EQUAL_NOCASE     { cur_operator = $$ = IDMEF_CRITERION_OPERATOR_EQUAL|IDMEF_CRITERION_OPERATOR_NOCASE|IDMEF_CRITERION_OPERATOR_NOT; }
| TOK_ERROR                         { real_ret = prelude_error_verbose(PRELUDE_ERROR_IDMEF_CRITERIA_PARSE,
                                                                       "Criteria parser reported: Invalid operator found"); YYERROR; }
;

operator:       TOK_OPERATOR_AND        { $$ = operator_and; }
                | TOK_OPERATOR_OR       { $$ = operator_or; }
;

%%

static void _preludeyyerror(const char *s)  /* Called by yyparse on error */
{
        real_ret = prelude_error_verbose_make(PRELUDE_ERROR_SOURCE_IDMEF_CRITERIA,
                                              PRELUDE_ERROR_IDMEF_CRITERIA_PARSE,
                                              "IDMEF-Criteria parser: %s", s);
}


int idmef_criteria_new_from_string(idmef_criteria_t **new_criteria, const char *str)
{
        int ret;
        void *state;

        prelude_return_val_if_fail(str, -1);

        gl_lock_lock(_criteria_parse_mutex);

        real_ret = 0;
        processed_criteria = NULL;

        state = _preludeyy_scan_string(str);
        ret = yyparse();
        _preludeyy_delete_buffer(state);

        if ( ret != 0 ) {
                _idmef_criteria_string_init_lexer();

                if ( real_ret )
                        ret = real_ret;
                else
                        ret = prelude_error_make(PRELUDE_ERROR_SOURCE_IDMEF_CRITERIA, PRELUDE_ERROR_IDMEF_CRITERIA_PARSE);

                if ( processed_criteria )
                        idmef_criteria_destroy(processed_criteria);
        }

        else *new_criteria = processed_criteria;

        gl_lock_unlock(_criteria_parse_mutex);

        return ret;
}
