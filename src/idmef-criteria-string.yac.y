/*****
*
* Copyright (C) 2003-2005,2006,2007 PreludeIDS Technologies. All Rights Reserved.
* Author: Krzysztof Zaraska <kzaraska@student.uci.agh.edu.pl>
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
extern void yylex_init(void);
extern void yylex_destroy(void);
static void yyerror(char *s);
extern void *yy_scan_string(const char *);
extern void yy_delete_buffer(void *);
void _idmef_criteria_string_init_lexer(void);

#define YYERROR_VERBOSE


static int create_criteria(idmef_criteria_t **criteria, idmef_path_t *path,
                           idmef_criterion_value_t *value, idmef_criterion_operator_t operator)
{
        idmef_criterion_t *criterion;

        real_ret = idmef_criteria_new(criteria);
        if ( real_ret < 0 )
                goto err;

        if ( path_count++ > 0 )
                idmef_path_ref(path);

        real_ret = idmef_criterion_new(&criterion, path, value, operator);
        if ( real_ret < 0 ) {
                idmef_criteria_destroy(*criteria);
                goto err;
        }

        idmef_criteria_set_criterion(*criteria, criterion);
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
        idmef_criterion_operator_t relation;
}

/* BISON Declarations */

%token <str> TOK_IDMEF_VALUE "<IDMEF-Value>"
%token <str> TOK_IDMEF_PATH "<IDMEF-Path>"

%destructor { free($$); } TOK_IDMEF_VALUE TOK_IDMEF_PATH
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

%token TOK_NOT "!"
%token TOK_OPERATOR_AND "&&"
%token TOK_OPERATOR_OR "||"

%token TOK_ERROR

%type <criteria> criteria
%type <criteria> criteria_base
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
        criteria_base {
                $$ = $1;
        }

        | criteria operator criteria_base {
                if ( $2 == operator_or )
                        idmef_criteria_or_criteria($1, $3);
                else
                        idmef_criteria_and_criteria($1, $3);

                $$ = $1;
        }
;

criteria_base:
        criterion {
                $$ = $1;
        }

        | '(' criteria ')' {
                $$ = $2;
        }

        | TOK_NOT '(' criteria ')' {
                idmef_criteria_set_negation($3, TRUE);
                $$ = $3;
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

        | TOK_NOT path {
                idmef_criteria_t *criteria;

                real_ret = create_criteria(&criteria, $2, NULL, IDMEF_CRITERION_OPERATOR_NULL);
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
        TOK_IDMEF_VALUE {
                idmef_criteria_t *criteria;
                idmef_criterion_value_t *value = NULL;

                real_ret = idmef_criterion_value_new_from_string(&value, cur_path, $1, cur_operator);
                free($1);

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
| TOK_NOT                           { cur_operator = $$ = IDMEF_CRITERION_OPERATOR_NULL; }
| TOK_ERROR                         { real_ret = prelude_error_verbose(PRELUDE_ERROR_IDMEF_CRITERIA_PARSE,
                                                                       "Criteria parser reported: Invalid operator found"); YYERROR; }
;

operator:       TOK_OPERATOR_AND        { $$ = operator_and; }
                | TOK_OPERATOR_OR       { $$ = operator_or; }
;

%%

static void yyerror(char *s)  /* Called by yyparse on error */
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

        state = yy_scan_string(str);
        ret = yyparse();
        yy_delete_buffer(state);

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
