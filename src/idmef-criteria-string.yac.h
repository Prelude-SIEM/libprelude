/* A Bison parser, made by GNU Bison 3.0.4.  */

/* Bison interface for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2015 Free Software Foundation, Inc.

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

#ifndef YY__PRELUDEYY_IDMEF_CRITERIA_STRING_YAC_H_INCLUDED
# define YY__PRELUDEYY_IDMEF_CRITERIA_STRING_YAC_H_INCLUDED
/* Debug traces.  */
#ifndef _PRELUDEYYDEBUG
# if defined YYDEBUG
#if YYDEBUG
#   define _PRELUDEYYDEBUG 1
#  else
#   define _PRELUDEYYDEBUG 0
#  endif
# else /* ! defined YYDEBUG */
#  define _PRELUDEYYDEBUG 0
# endif /* ! defined YYDEBUG */
#endif  /* ! defined _PRELUDEYYDEBUG */
#if _PRELUDEYYDEBUG
extern int _preludeyydebug;
#endif

/* Token type.  */
#ifndef _PRELUDEYYTOKENTYPE
# define _PRELUDEYYTOKENTYPE
  enum _preludeyytokentype
  {
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
    TOK_OPERATOR_NOT = 276,
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
#define TOK_OPERATOR_NOT 276
#define TOK_OPERATOR_AND 277
#define TOK_OPERATOR_OR 278
#define TOK_ERROR 279

/* Value type.  */
#if ! defined _PRELUDEYYSTYPE && ! defined _PRELUDEYYSTYPE_IS_DECLARED

union _PRELUDEYYSTYPE
{
#line 94 "idmef-criteria-string.yac.y" /* yacc.c:1909  */

        char *str;
        int operator;
        idmef_path_t *path;
        idmef_criteria_t *criteria;
        idmef_criteria_operator_t relation;

#line 118 "idmef-criteria-string.yac.h" /* yacc.c:1909  */
};

typedef union _PRELUDEYYSTYPE _PRELUDEYYSTYPE;
# define _PRELUDEYYSTYPE_IS_TRIVIAL 1
# define _PRELUDEYYSTYPE_IS_DECLARED 1
#endif


extern _PRELUDEYYSTYPE _preludeyylval;

int _preludeyyparse (void);

#endif /* !YY__PRELUDEYY_IDMEF_CRITERIA_STRING_YAC_H_INCLUDED  */
