/*****
*
* Copyright (C) 2003, 2004, 2005 PreludeIDS Technologies. All Rights Reserved.
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
#include <stdlib.h>
#include <string.h>

#include "idmef.h"
#include "common.h"
#include "idmef-criteria-string.yac.h"

#define YY_NO_UNPUT
#define YY_NO_TOP_STATE

#ifndef MIN
# define MIN(x, y) ((x) < (y) ? (x) : (y))
#endif

int yylex(void);
int yyget_lineno(void);
FILE *yyget_in(void);
FILE *yyget_out(void);
int yyget_leng(void);
char *yyget_text(void);
void yyset_lineno(int line_number);
void yyset_in(FILE *in_str);
void yyset_out(FILE *out_str);
int yyget_debug(void);
void yyset_debug(int bdebug);
int yylex_destroy(void);


static prelude_bool_t in_parenthesis = FALSE;

static void do_push(void);
static void do_pop(void);

static char *escape_str(char *str)
{
        size_t w = 0, i = 0;
        int escape_next = FALSE;

        for ( i = 0; str[i]; i++ ) {

                if ( ! escape_next && str[i] == '\\' ) {
                        escape_next = TRUE;
                        continue;
                }

                str[w++] = str[i];
                escape_next = FALSE;
        }

        str[w] = 0;
        return str;
}

%}

IDMEF_PATH      ([a-zA-Z0-9_\-]+(\(\-?[0-9\*]+\))?\.?)+
SQSTRING        \'([^\\\']|\\.)*\'
DQSTRING        \"([^\\\"]|\\.)*\"
BLANK           [ \t\n]+

%option noyywrap
%option stack

%x HAS_PATH

%%

{IDMEF_PATH}            {
                                yylval.str = strdup(yytext);
                                yy_push_state(HAS_PATH);
                                return TOK_IDMEF_PATH;
                        }

\&\&                    { return TOK_OPERATOR_AND; }
\&                      { return TOK_OPERATOR_AND; }
\|\|                    { return TOK_OPERATOR_OR; }
\|                      { return TOK_OPERATOR_OR; }

\(                      { return '('; }
\)                      { return ')'; }

{BLANK}                 {
                                /* nop */;
                        }

.                       {
                                /* invalid token */
                                yylval.str = strdup(yytext);
                                return TOK_ERROR;
                        }


<HAS_PATH>\<\>                    { do_push(); return TOK_RELATION_SUBSTRING; }
<HAS_PATH>\<\>\*                  { do_push(); return TOK_RELATION_SUBSTRING_NOCASE; }
<HAS_PATH>\!\<\>                  { do_push(); return TOK_RELATION_NOT_SUBSTRING; }
<HAS_PATH>\!\<\>\*                { do_push(); return TOK_RELATION_NOT_SUBSTRING_NOCASE; }

<HAS_PATH>\>                      { do_push(); return TOK_RELATION_GREATER; }
<HAS_PATH>\>\=                    { do_push(); return TOK_RELATION_GREATER_OR_EQUAL; }
<HAS_PATH>\<                      { do_push(); return TOK_RELATION_LESS; }
<HAS_PATH>\<\=                    { do_push(); return TOK_RELATION_LESS_OR_EQUAL; }

<HAS_PATH>\~                      { do_push(); return TOK_RELATION_REGEXP; }
<HAS_PATH>\~\*                    { do_push(); return TOK_RELATION_REGEXP_NOCASE; }
<HAS_PATH>\!\~                    { do_push(); return TOK_RELATION_NOT_REGEXP; }
<HAS_PATH>\!\~\*                  { do_push(); return TOK_RELATION_NOT_REGEXP_NOCASE; }

<HAS_PATH>\=\=                    { do_push(); return TOK_RELATION_EQUAL; }
<HAS_PATH>\=                      { do_push(); return TOK_RELATION_EQUAL; }
<HAS_PATH>\=\*                    { do_push(); return TOK_RELATION_EQUAL_NOCASE; }
<HAS_PATH>\!\=                    { do_push(); return TOK_RELATION_NOT_EQUAL; }
<HAS_PATH>\!\=\*                  { do_push(); return TOK_RELATION_NOT_EQUAL_NOCASE; }
<HAS_PATH>\!                      { return TOK_RELATION_IS_NULL; }

<HAS_PATH>\&\&                    { return TOK_OPERATOR_AND; }
<HAS_PATH>\&                      { return TOK_OPERATOR_AND; }
<HAS_PATH>\|\|                    { return TOK_OPERATOR_OR; }
<HAS_PATH>\|                      { return TOK_OPERATOR_OR; }


<HAS_PATH>\(            {
                                in_parenthesis = TRUE;
                                return '(';
                        }

<HAS_PATH>\)            {
                                in_parenthesis = FALSE;
                                do_pop();
                                return ')';
                        }

<HAS_PATH>{SQSTRING}    {
                                yylval.str = escape_str(strndup(yytext + 1, yyleng - 2));

                                if ( ! in_parenthesis )
                                        yy_pop_state();

                                return TOK_IDMEF_VALUE;
                        }

<HAS_PATH>{DQSTRING}    {
                                yylval.str = escape_str(strndup(yytext + 1, yyleng - 2));
                                do_pop();
                                return TOK_IDMEF_VALUE;
                        }


<HAS_PATH>[^\&\|\(\) ]+ {
                                yylval.str = escape_str(strdup(yytext));
                                do_pop();
                                return TOK_IDMEF_VALUE;
                        }

<HAS_PATH>{BLANK}       {
                                /* nop */;
                        }

%%

void _idmef_criteria_string_init_lexer(void);

void _idmef_criteria_string_init_lexer(void)
{
        BEGIN INITIAL;
}




static void do_push(void)
{
}


static void do_pop(void)
{
        if ( ! in_parenthesis ) {
                yy_pop_state();
        }
}

