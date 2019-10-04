/*****
*
* Copyright (C) 2003-2019 CS-SI. All Rights Reserved.
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
* You should have received a copy of the GNU General Public License along
* with this program; if not, write to the Free Software Foundation, Inc.,
* 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*
*****/

%top{
#include "libmissing.h"
}

%{
#include <stdlib.h>
#include <string.h>

#include "idmef.h"
#include "common.h"
#include "idmef-criteria-string.yac.h"

#define YY_NO_INPUT
#define YY_NO_TOP_STATE

#ifndef MIN
# define MIN(x, y) ((x) < (y) ? (x) : (y))
#endif

static void do_pop(void);

static unsigned int inp = 0;

%}

IDMEF_PATH      ([a-zA-Z0-9_\-]+(\(((\-?[0-9\*]+)|(\"[^"]+\")|(\'[^']+\'))\))?\.?)+
SQSTRING        \'([^\\\']|\\.)*\'
DQSTRING        \"([^\\\"]|\\.)*\"
RSQSTRING        r\'([^\\\']|\\.)*\'
RDQSTRING        r\"([^\\\"]|\\.)*\"
BLANK           [ \t\n]+

%option nounput
%option noyywrap
%option stack
%option prefix="_preludeyy"

%x IDMEF_VALUE

%%

substr                  { yy_push_state(IDMEF_VALUE); return TOK_RELATION_SUBSTRING; }
\!substr                { yy_push_state(IDMEF_VALUE); return TOK_RELATION_NOT_SUBSTRING; }
substr\*                { yy_push_state(IDMEF_VALUE); return TOK_RELATION_SUBSTRING_NOCASE; }
\!substr\*              { yy_push_state(IDMEF_VALUE); return TOK_RELATION_NOT_SUBSTRING_NOCASE; }

{IDMEF_PATH}            {
                                _preludeyylval.str = strdup(yytext);
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

\!                      { return TOK_OPERATOR_NOT; }
\&\&                    { return TOK_OPERATOR_AND; }
\&                      { return TOK_OPERATOR_AND; }
\|\|                    { return TOK_OPERATOR_OR; }
\|                      { return TOK_OPERATOR_OR; }

\(                      { return '('; }
\)                      { return ')'; }

<IDMEF_VALUE>{RSQSTRING} {
                                _preludeyylval.str = strndup(yytext + 2, yyleng - 3);

                                if ( ! inp )
                                        yy_pop_state();

                                return TOK_IDMEF_RAW_VALUE;
                        }

<IDMEF_VALUE>{RDQSTRING} {
                                _preludeyylval.str = strndup(yytext + 2, yyleng - 3);

                                if ( ! inp )
                                        yy_pop_state();

                                return TOK_IDMEF_RAW_VALUE;
                        }

<IDMEF_VALUE>{SQSTRING} {
                                _preludeyylval.str = strndup(yytext + 1, yyleng - 2);

                                if ( ! inp )
                                        yy_pop_state();

                                return TOK_IDMEF_VALUE;
                        }

<IDMEF_VALUE>{DQSTRING} {
                                _preludeyylval.str = strndup(yytext + 1, yyleng - 2);

                                if ( ! inp )
                                        yy_pop_state();

                                return TOK_IDMEF_VALUE;
                        }

<IDMEF_VALUE>[^ \t\(\)\&\|]+ {
                                _preludeyylval.str = strdup(yytext);

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
                                _preludeyylval.str = strdup(yytext);
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
