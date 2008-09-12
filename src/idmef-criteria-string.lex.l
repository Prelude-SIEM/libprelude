/*****
*
* Copyright (C) 2003, 2004, 2005 PreludeIDS Technologies. All Rights Reserved.
* Author: Krzysztof Zaraska
* Author: Nicolas Delon
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
static void do_pop(void);


static unsigned int inp = 0;


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

%x IDMEF_VALUE

%%

substr                  { yy_push_state(IDMEF_VALUE); return TOK_RELATION_SUBSTRING; }
\!substr                { yy_push_state(IDMEF_VALUE); return TOK_RELATION_NOT_SUBSTRING; }
substr\*                { yy_push_state(IDMEF_VALUE); return TOK_RELATION_SUBSTRING_NOCASE; }
\!substr\*              { yy_push_state(IDMEF_VALUE); return TOK_RELATION_NOT_SUBSTRING_NOCASE; }

{IDMEF_PATH}            {
                                yylval.str = strdup(yytext);
                                return TOK_IDMEF_PATH;
                        }

\<\>                    { yy_push_state(IDMEF_VALUE); return TOK_RELATION_SUBSTRING; }
\<\>\*                  { yy_push_state(IDMEF_VALUE); return TOK_RELATION_SUBSTRING_NOCASE; }
\!\<\>                  { yy_push_state(IDMEF_VALUE); return TOK_RELATION_NOT_SUBSTRING; }
\!\<\>\*                { yy_push_state(IDMEF_VALUE); return TOK_RELATION_NOT_SUBSTRING_NOCASE; }

\>                      { yy_push_state(IDMEF_VALUE); return TOK_RELATION_GREATER; }
\>\=                    { yy_push_state(IDMEF_VALUE); return TOK_RELATION_GREATER_OR_EQUAL; }
\<                      { yy_push_state(IDMEF_VALUE); return TOK_RELATION_LESS; }
\<\=                    { yy_push_state(IDMEF_VALUE); return TOK_RELATION_LESS_OR_EQUAL; }

\~                      { yy_push_state(IDMEF_VALUE); return TOK_RELATION_REGEXP; }
\~\*                    { yy_push_state(IDMEF_VALUE); return TOK_RELATION_REGEXP_NOCASE; }
\!\~                    { yy_push_state(IDMEF_VALUE); return TOK_RELATION_NOT_REGEXP; }
\!\~\*                  { yy_push_state(IDMEF_VALUE); return TOK_RELATION_NOT_REGEXP_NOCASE; }

\=\=                    { yy_push_state(IDMEF_VALUE); return TOK_RELATION_EQUAL; }
\=                      { yy_push_state(IDMEF_VALUE); return TOK_RELATION_EQUAL; }
\=\*                    { yy_push_state(IDMEF_VALUE); return TOK_RELATION_EQUAL_NOCASE; }
\!\=                    { yy_push_state(IDMEF_VALUE); return TOK_RELATION_NOT_EQUAL; }
\!\=\*                  { yy_push_state(IDMEF_VALUE); return TOK_RELATION_NOT_EQUAL_NOCASE; }
\!                      { return TOK_NOT; }

\&\&                    { return TOK_OPERATOR_AND; }
\&                      { return TOK_OPERATOR_AND; }
\|\|                    { return TOK_OPERATOR_OR; }
\|                      { return TOK_OPERATOR_OR; }

\(                      { return '('; }
\)                      { return ')'; }

<IDMEF_VALUE>{SQSTRING} {
                                yylval.str = escape_str(strndup(yytext + 1, yyleng - 2));

                                if ( ! inp )
                                        yy_pop_state();

                                return TOK_IDMEF_VALUE;
                        }

<IDMEF_VALUE>{DQSTRING} {
                                yylval.str = escape_str(strndup(yytext + 1, yyleng - 2));

                                if ( ! inp )
                                        yy_pop_state();

                                return TOK_IDMEF_VALUE;
                        }

<IDMEF_VALUE>[^ \t\(\)\&\|]+ {
                                yylval.str = escape_str(strdup(yytext));

                                if ( ! inp )
                                        yy_pop_state();

                                return TOK_IDMEF_VALUE;
                        }

<IDMEF_VALUE>{BLANK}    {
                                /* nop */;
                        }

<IDMEF_VALUE>\(         {
                                inp++;
                                return '(';
                        }

<IDMEF_VALUE>\)         {
                                do_pop();
                                return ')';
                        }

<IDMEF_VALUE>\&\&       { return TOK_OPERATOR_AND; }
<IDMEF_VALUE>\&         { return TOK_OPERATOR_AND; }
<IDMEF_VALUE>\|\|       { return TOK_OPERATOR_OR; }
<IDMEF_VALUE>\|         { return TOK_OPERATOR_OR; }

<IDMEF_VALUE>.          {
                                /* invalid token */
                                return TOK_ERROR;
                        }

{BLANK}                 {
                                /* nop */;
                        }

.                       {
                                /* invalid token */
                                yylval.str = strdup(yytext);
                                return TOK_ERROR;
                        }



%%

static void do_pop(void)
{
        if ( --inp == 0 )
                yy_pop_state();
}

void _idmef_criteria_string_init_lexer(void);

void _idmef_criteria_string_init_lexer(void)
{
        inp = 0;
        BEGIN INITIAL;
}
