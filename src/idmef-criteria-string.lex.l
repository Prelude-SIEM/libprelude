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

#include "libmissing.h"
#include "idmef.h"
#include "common.h"
#include "idmef-criteria-string.yac.h"

int yylex(void);

#define YY_NO_UNPUT

%}

IDMEF_PATH      ([a-zA-Z0-9_\-]+(\(\-?[0-9]+\))?\.?)+
SQSTRING	\'([^\\\']|\\.)*\'
DQSTRING	\"([^\\\"]|\\.)*\"
BLANK		[ \t\n]+

%option noyywrap
%option stack

%x IDMEF_VALUE

%%

substr			{ yy_push_state(IDMEF_VALUE); return TOK_RELATION_SUBSTRING; }
SUBSTR			{ yy_push_state(IDMEF_VALUE); return TOK_RELATION_SUBSTRING; }

{IDMEF_PATH}		{ 
				yylval.str = strdup(yytext);
				return TOK_IDMEF_PATH; 
			}

\=\~			{ yy_push_state(IDMEF_VALUE); return TOK_RELATION_REGEXP; }
\>			{ yy_push_state(IDMEF_VALUE); return TOK_RELATION_GREATER; }
\>\=			{ yy_push_state(IDMEF_VALUE); return TOK_RELATION_GREATER_OR_EQUAL; }
\<			{ yy_push_state(IDMEF_VALUE); return TOK_RELATION_LESS; }
\<\=			{ yy_push_state(IDMEF_VALUE); return TOK_RELATION_LESS_OR_EQUAL; }
\=\=			{ yy_push_state(IDMEF_VALUE); return TOK_RELATION_EQUAL; }
\=			{ yy_push_state(IDMEF_VALUE); return TOK_RELATION_EQUAL; }
\!\=			{ yy_push_state(IDMEF_VALUE); return TOK_RELATION_NOT_EQUAL; }
\!			{ return TOK_RELATION_IS_NULL; }

\&\&			{ return TOK_OPERATOR_AND; }
\&			{ return TOK_OPERATOR_AND; }
\|\|			{ return TOK_OPERATOR_OR; }
\|			{ return TOK_OPERATOR_OR; }

\(			{ return '('; }
\)			{ return ')'; }

<IDMEF_VALUE>{SQSTRING}	{
				yylval.str = strndup(yytext + 1, yyleng - 2);
				yy_pop_state();
				return TOK_IDMEF_VALUE;
			}

<IDMEF_VALUE>{DQSTRING}	{
				yylval.str = strndup(yytext + 1, yyleng - 2);
				yy_pop_state();
				return TOK_IDMEF_VALUE;
			}

<IDMEF_VALUE>[^ \t\(\)\&\|]+ {
				yylval.str = strdup(yytext);
				yy_pop_state();
				return TOK_IDMEF_VALUE;
			}

<IDMEF_VALUE>{BLANK}	{
				/* nop */;
			}

<IDMEF_VALUE>.		{
				/* invalid token */
				return TOK_ERROR;
			}

{BLANK}			{
				/* nop */;
			}

.			{
				/* invalid token */
				return TOK_ERROR;
			}
