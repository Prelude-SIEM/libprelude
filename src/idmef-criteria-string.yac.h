/* A Bison parser, made by GNU Bison 2.7.  */

/* Bison interface for Yacc-like parsers in C
   
      Copyright (C) 1984, 1989-1990, 2000-2012 Free Software Foundation, Inc.
   
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.
   
   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

#ifndef YY_YY_IDMEF_CRITERIA_STRING_YAC_H_INCLUDED
# define YY_YY_IDMEF_CRITERIA_STRING_YAC_H_INCLUDED
/* Enabling traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif
#if YYDEBUG
extern int yydebug;
#endif

/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     TOK_IDMEF_VALUE = 258,
     TOK_IDMEF_PATH = 259,
     TOK_RELATION_SUBSTRING = 260,
     TOK_RELATION_SUBSTRING_NOCASE = 261,
     TOK_RELATION_NOT_SUBSTRING = 262,
     TOK_RELATION_NOT_SUBSTRING_NOCASE = 263,
     TOK_RELATION_REGEXP = 264,
     TOK_RELATION_REGEXP_NOCASE = 265,
     TOK_RELATION_NOT_REGEXP = 266,
     TOK_RELATION_NOT_REGEXP_NOCASE = 267,
     TOK_RELATION_GREATER = 268,
     TOK_RELATION_GREATER_OR_EQUAL = 269,
     TOK_RELATION_LESS = 270,
     TOK_RELATION_LESS_OR_EQUAL = 271,
     TOK_RELATION_EQUAL = 272,
     TOK_RELATION_EQUAL_NOCASE = 273,
     TOK_RELATION_NOT_EQUAL = 274,
     TOK_RELATION_NOT_EQUAL_NOCASE = 275,
     TOK_NOT = 276,
     TOK_OPERATOR_AND = 277,
     TOK_OPERATOR_OR = 278,
     TOK_ERROR = 279
   };
#endif
/* Tokens.  */
#define TOK_IDMEF_VALUE 258
#define TOK_IDMEF_PATH 259
#define TOK_RELATION_SUBSTRING 260
#define TOK_RELATION_SUBSTRING_NOCASE 261
#define TOK_RELATION_NOT_SUBSTRING 262
#define TOK_RELATION_NOT_SUBSTRING_NOCASE 263
#define TOK_RELATION_REGEXP 264
#define TOK_RELATION_REGEXP_NOCASE 265
#define TOK_RELATION_NOT_REGEXP 266
#define TOK_RELATION_NOT_REGEXP_NOCASE 267
#define TOK_RELATION_GREATER 268
#define TOK_RELATION_GREATER_OR_EQUAL 269
#define TOK_RELATION_LESS 270
#define TOK_RELATION_LESS_OR_EQUAL 271
#define TOK_RELATION_EQUAL 272
#define TOK_RELATION_EQUAL_NOCASE 273
#define TOK_RELATION_NOT_EQUAL 274
#define TOK_RELATION_NOT_EQUAL_NOCASE 275
#define TOK_NOT 276
#define TOK_OPERATOR_AND 277
#define TOK_OPERATOR_OR 278
#define TOK_ERROR 279



#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
{
/* Line 2058 of yacc.c  */
#line 103 "idmef-criteria-string.yac.y"

        char *str;
        int operator;
        idmef_path_t *path;
        idmef_criteria_t *criteria;
        idmef_criterion_operator_t relation;


/* Line 2058 of yacc.c  */
#line 114 "idmef-criteria-string.yac.h"
} YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
#endif

extern YYSTYPE yylval;

#ifdef YYPARSE_PARAM
#if defined __STDC__ || defined __cplusplus
int yyparse (void *YYPARSE_PARAM);
#else
int yyparse ();
#endif
#else /* ! YYPARSE_PARAM */
#if defined __STDC__ || defined __cplusplus
int yyparse (void);
#else
int yyparse ();
#endif
#endif /* ! YYPARSE_PARAM */

#endif /* !YY_YY_IDMEF_CRITERIA_STRING_YAC_H_INCLUDED  */
