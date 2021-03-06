/* A Bison parser, made by GNU Bison 3.5.  */

/* Bison interface for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2015, 2018-2019 Free Software Foundation,
   Inc.

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

/* Undocumented macros, especially those whose name start with YY_,
   are private implementation details.  Do not rely on them.  */

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
    TOK_IDMEF_RAW_VALUE = 258,
    TOK_IDMEF_VALUE = 259,
    TOK_IDMEF_PATH = 260,
    TOK_RELATION_SUBSTRING = 261,
    TOK_RELATION_SUBSTRING_NOCASE = 262,
    TOK_RELATION_NOT_SUBSTRING = 263,
    TOK_RELATION_NOT_SUBSTRING_NOCASE = 264,
    TOK_RELATION_REGEXP = 265,
    TOK_RELATION_REGEXP_NOCASE = 266,
    TOK_RELATION_NOT_REGEXP = 267,
    TOK_RELATION_NOT_REGEXP_NOCASE = 268,
    TOK_RELATION_GREATER = 269,
    TOK_RELATION_GREATER_OR_EQUAL = 270,
    TOK_RELATION_LESS = 271,
    TOK_RELATION_LESS_OR_EQUAL = 272,
    TOK_RELATION_EQUAL = 273,
    TOK_RELATION_EQUAL_NOCASE = 274,
    TOK_RELATION_NOT_EQUAL = 275,
    TOK_RELATION_NOT_EQUAL_NOCASE = 276,
    TOK_OPERATOR_NOT = 277,
    TOK_OPERATOR_AND = 278,
    TOK_OPERATOR_OR = 279,
    TOK_ERROR = 280
  };
#endif
/* Tokens.  */
#define TOK_IDMEF_RAW_VALUE 258
#define TOK_IDMEF_VALUE 259
#define TOK_IDMEF_PATH 260
#define TOK_RELATION_SUBSTRING 261
#define TOK_RELATION_SUBSTRING_NOCASE 262
#define TOK_RELATION_NOT_SUBSTRING 263
#define TOK_RELATION_NOT_SUBSTRING_NOCASE 264
#define TOK_RELATION_REGEXP 265
#define TOK_RELATION_REGEXP_NOCASE 266
#define TOK_RELATION_NOT_REGEXP 267
#define TOK_RELATION_NOT_REGEXP_NOCASE 268
#define TOK_RELATION_GREATER 269
#define TOK_RELATION_GREATER_OR_EQUAL 270
#define TOK_RELATION_LESS 271
#define TOK_RELATION_LESS_OR_EQUAL 272
#define TOK_RELATION_EQUAL 273
#define TOK_RELATION_EQUAL_NOCASE 274
#define TOK_RELATION_NOT_EQUAL 275
#define TOK_RELATION_NOT_EQUAL_NOCASE 276
#define TOK_OPERATOR_NOT 277
#define TOK_OPERATOR_AND 278
#define TOK_OPERATOR_OR 279
#define TOK_ERROR 280

/* Value type.  */
#if ! defined _PRELUDEYYSTYPE && ! defined _PRELUDEYYSTYPE_IS_DECLARED
union _PRELUDEYYSTYPE
{
#line 174 "idmef-criteria-string.yac.y"

        char *str;
        int operator;
        idmef_path_t *path;
        idmef_criteria_t *criteria;
        idmef_criteria_operator_t relation;

#line 123 "idmef-criteria-string.yac.h"

};
typedef union _PRELUDEYYSTYPE _PRELUDEYYSTYPE;
# define _PRELUDEYYSTYPE_IS_TRIVIAL 1
# define _PRELUDEYYSTYPE_IS_DECLARED 1
#endif


extern _PRELUDEYYSTYPE _preludeyylval;

int _preludeyyparse (void);

#endif /* !YY__PRELUDEYY_IDMEF_CRITERIA_STRING_YAC_H_INCLUDED  */
