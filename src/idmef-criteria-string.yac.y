/*****
*
* Copyright (C) 2003 Krzysztof Zaraska <kzaraska@student.uci.agh.edu.pl>
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
     
%{

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <sys/types.h>
#include <stdarg.h>
#include <ctype.h>
#include <pthread.h>

#include "prelude-log.h"

#include "idmef-string.h"
#include "idmef-tree.h"

#include "idmef-value.h"
#include "idmef-object.h"
#include "idmef-message.h"
#include "idmef-value-object.h"
#include "idmef-criterion-value.h"

#include "idmef-criteria.h"

#include "idmef-criteria-string.h"

struct parser_control {
	idmef_criteria_t *criteria;
};

#define operator_or 1
#define operator_and 2
 
#define YYPARSE_PARAM param

extern int yylex();
extern void yylex_init();
extern void yylex_destroy();
static void yyerror (char *s);
 
%}

%union {
	char *str;
        int operator;
	idmef_criterion_t *criterion;
	idmef_criteria_t *criteria;
	idmef_value_relation_t relation;
}     

/* BISON Declarations */

%token <str> TOK_STRING

%token TOK_RELATION_SUBSTRING
%token TOK_RELATION_REGEXP
%token TOK_RELATION_GREATER
%token TOK_RELATION_GREATER_OR_EQUAL
%token TOK_RELATION_LESS
%token TOK_RELATION_LESS_OR_EQUAL
%token TOK_RELATION_EQUAL
%token TOK_RELATION_NOT_EQUAL
%token TOK_RELATION_IS_NULL

%token TOK_OPERATOR_AND
%token TOK_OPERATOR_OR

%type <criteria> criteria
%type <criteria> criteria_base
%type <criterion> criterion
%type <relation> relation
%type <operator> operator
     
/* Grammar follows */
%%

input: criteria					{
							((struct parser_control *) param)->criteria = $1;
						}
;

criteria:	criteria_base			{
							$$ = $1;
						}
	| criteria operator criteria_base	{                                                        
                                                        if ( $2 == operator_or ) {
                                                                idmef_criteria_or_criteria($1, $3);
                                                        } else {
                                                                idmef_criteria_and_criteria($1, $3);
                                                        }
                                                        
                                                        $$ = $1;
						}
;

criteria_base:	criterion			{
							idmef_criteria_t *criteria;

							criteria = idmef_criteria_new();
							if ( ! criteria )
								YYABORT;
							criteria->criterion = $1;
							$$ = criteria;
                                                        
						}
	| '(' criteria ')'			{
							$$ = $2;
						}
;

criterion: TOK_STRING relation TOK_STRING 	{
							idmef_object_t *object = NULL;
							idmef_criterion_value_t *value = NULL;
							idmef_value_relation_t relation;
							idmef_criterion_t *criterion;

							object = idmef_object_new_fast($1);
							if ( ! object ) {
								log(LOG_ERR, "cannot build object '%s'\n", $1);
								free($1);
								free($3);
								YYABORT;
							}

							value = idmef_criterion_value_new_generic(object, $3);
							if ( ! value ) {
								log(LOG_ERR, "cannot build value '%s' for object '%s'\n",
								    $3, $1);
								free($1);
								free($3);
								idmef_object_destroy(object);
								YYABORT;
							}

							relation = $2;

							criterion = idmef_criterion_new(object, value, relation);
							if ( ! criterion ) {
								log(LOG_ERR, "cannot build criterion for "
								    "object: %s and value: %s\n", $1, $3);
								free($1);
								free($3);
								idmef_object_destroy(object);
								idmef_criterion_value_destroy(value);
								YYABORT;
							}

							free($1);
							free($3);
                                                        
							$$ = criterion;
						}
	| TOK_STRING				{
							idmef_object_t *object;
							idmef_criterion_t *criterion;
                                                        
							object = idmef_object_new_fast($1);
							if ( ! object ) {
								log(LOG_ERR, "cannot build object '%s'\n", $1);
								free($1);
								YYABORT;
							}

							criterion = idmef_criterion_new(object, NULL, IDMEF_VALUE_RELATION_IS_NOT_NULL);
							if ( ! criterion ) {
								log(LOG_ERR,
								    "cannot build criterion for object: '%s' and value: NULL\n",
								    $1);
								free($1);
								idmef_object_destroy(object);
								YYABORT;
							}

							free($1);

							$$ = criterion;
						}
	| TOK_RELATION_IS_NULL TOK_STRING	{
							idmef_object_t *object;
							idmef_criterion_t *criterion;

							object = idmef_object_new_fast($2);
							if ( ! object ) {
								log(LOG_ERR, "cannot build object '%s'\n", $2);
								free($2);
								YYABORT;
							}

							criterion = idmef_criterion_new(object, NULL, IDMEF_VALUE_RELATION_IS_NULL);
							if ( ! criterion ) {
								log(LOG_ERR,
								    "cannot build criterion for object: '%s' and value: NULL\n",
								    $2);
								free($2);
								idmef_object_destroy(object);
								YYABORT;
							}

							free($2);

							$$ = criterion;
						}
;

relation:	TOK_RELATION_SUBSTRING		{ $$ = IDMEF_VALUE_RELATION_SUBSTR; }
	|	TOK_RELATION_REGEXP		{ $$ = IDMEF_VALUE_RELATION_REGEX; }
	|	TOK_RELATION_GREATER		{ $$ = IDMEF_VALUE_RELATION_GREATER; }
	|	TOK_RELATION_GREATER_OR_EQUAL	{ $$ = IDMEF_VALUE_RELATION_GREATER|IDMEF_VALUE_RELATION_EQUAL; }
	|	TOK_RELATION_LESS		{ $$ = IDMEF_VALUE_RELATION_LESSER; }
	|	TOK_RELATION_LESS_OR_EQUAL	{ $$ = IDMEF_VALUE_RELATION_LESSER|IDMEF_VALUE_RELATION_EQUAL; }
	|	TOK_RELATION_EQUAL		{ $$ = IDMEF_VALUE_RELATION_EQUAL; }
	|	TOK_RELATION_NOT_EQUAL		{ $$ = IDMEF_VALUE_RELATION_NOT_EQUAL; }
	|	TOK_RELATION_IS_NULL		{ $$ = IDMEF_VALUE_RELATION_IS_NULL; }
;

operator:	TOK_OPERATOR_AND		{ $$ = operator_and; }
       |	TOK_OPERATOR_OR		        { $$ = operator_or; }
;

%%


static void yyerror(char *s)  /* Called by yyparse on error */
{
	/* nop */
}



idmef_criteria_t *idmef_criteria_new_string(const char *str)
{
        int retval;
	size_t len;
	char *buffer;
	struct parser_control parser_control;
	pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

	/* 
	 * yy_scan_buffer wants buffer terminated by a DOUBLE null character
	 */

	len = strlen(str);

	buffer = malloc(len + 2);
	if ( ! buffer )
		return NULL;

	memcpy(buffer, str, len);
	buffer[len] = 0;
	buffer[len + 1] = 0;

	parser_control.criteria = NULL;

	pthread_mutex_lock(&mutex);
	yy_scan_buffer(buffer, len + 2);
	retval = yyparse(&parser_control);
	pthread_mutex_unlock(&mutex);
        
	free(buffer);

	if ( retval != 0 ) {
                if ( parser_control.criteria )
                        idmef_criteria_destroy(parser_control.criteria);

                return NULL;
	}

	return parser_control.criteria;
}
