/*****
*
* Copyright (C) 2003 Nicolas Delon <delon.nicolas@wanadoo.fr>
* Copyright (C) 2003 Krzysztof Zaraska  <kzaraska@student.uci.agh.edu.pl>
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

#ifndef HAVE_STRNDUP

/* 
 * KZ's owns implementation of strndup(), based on man page from 
 * a GNU/Linux system.
 */
static char *strndup(const char *s, size_t n)
{
	char *ptr;
	size_t len, newlen;

	len = strlen(s);
	newlen = ( n < len ) ? n : len;

	ptr = malloc(newlen+1);
	if ( ! ptr ) {
		errno = ENOMEM;
		return NULL;
	}

	strncpy(ptr, s, newlen);
	ptr[newlen] = '\0';

	return ptr;
}

#endif /* HAVE_STRNDUP */

#include "idmef.h"

#include "idmef-criteria-string.yac.h"

%}

STRING		[a-zA-Z0-9_\.\-]+
SQSTRING	\'([^\\\']|\\.)*\'
DQSTRING	\"([^\\\"]|\\.)*\"
BLANK		[ \t\n]+

%option reentrant bison-bridge noyywrap

%%

substr			{ return TOK_RELATION_SUBSTRING; }
SUBSTR			{ return TOK_RELATION_SUBSTRING; }
\=\~			{ return TOK_RELATION_REGEXP; }
\>			{ return TOK_RELATION_GREATER; }
\>\=			{ return TOK_RELATION_GREATER_OR_EQUAL; }
\<			{ return TOK_RELATION_LESS; }
\<\=			{ return TOK_RELATION_LESS_OR_EQUAL; }
\=\=			{ return TOK_RELATION_EQUAL; }
\=			{ return TOK_RELATION_EQUAL; }
\!\=			{ return TOK_RELATION_NOT_EQUAL; }
\!			{ return TOK_RELATION_IS_NULL; }

and|AND			{ return TOK_OPERATOR_AND; }
\&\&			{ return TOK_OPERATOR_AND; }
\&			{ return TOK_OPERATOR_AND; }
or|OR			{ return TOK_OPERATOR_OR; }
\|\|			{ return TOK_OPERATOR_OR; }
\|			{ return TOK_OPERATOR_OR; }

\(			{ return '('; }
\)			{ return ')'; }

{STRING}		{
				yylval->str = strndup(yytext, yyleng);
				return TOK_STRING;
			}
{SQSTRING}		{
				yylval->str = strndup(yytext + 1, yyleng - 2);
				return TOK_STRING;
			}
{DQSTRING}		{
				yylval->str = strndup(yytext + 1, yyleng - 2);
				return TOK_STRING;
			}

{BLANK}			{
				/* nop */;
			}
