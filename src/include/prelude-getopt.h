/*****
*
* Copyright (C) 2001 Yoann Vandoorselaere <yoann@mandrakesoft.com>
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

typedef struct prelude_optlist prelude_optlist_t;
typedef struct prelude_option prelude_option_t;

typedef enum {
        required_argument,
        optionnal_argument,
        no_argument,
} prelude_option_argument_t;


prelude_optlist_t *prelude_option_new(void);

prelude_option_t *prelude_option_add(prelude_optlist_t *optlist,
                                     char shortopt, const char *longopt, const char *desc,
                                     prelude_option_argument_t has_arg, void (*set)(const char *optarg));


int prelude_option_parse_arguments(prelude_optlist_t *optlist,
                                   const char *filename, int argc, const char **argv);


void prelude_option_print(prelude_optlist_t *optlist);


void prelude_option_destroy(prelude_optlist_t *optlist);


/*
 * TODO
 */

#if 0

typedef enum {
        local_option,
        wide_option
} prelude_option_type_t;

#endif
