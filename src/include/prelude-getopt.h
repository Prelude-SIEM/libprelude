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


/*
 * prelude_option_add flags
 */
#define CLI_HOOK 0x1 /* Option to be hooked to command line */
#define CFG_HOOK 0x2 /* Option to be hooked to config file  */
#define WIDE_HOOK 0x4

/*
 * Possible return value for set() callback.
 * Theses return value are only usefull when the set function
 * is called from prelude_option_parse_argument().
 */
#define prelude_option_success 0

/*
 * tell prelude_option_parse_argument that next option should be
 * matched against parent of current option list
 */
#define prelude_option_end 1

/*
 * Any parsing will stop.
 */
#define prelude_option_error -1


typedef struct prelude_optlist prelude_optlist_t;
typedef struct prelude_option prelude_option_t;
typedef struct prelude_option_wide prelude_option_wide_t;


typedef enum {
        required_argument,
        optionnal_argument,
        no_argument,
} prelude_option_argument_t;



prelude_optlist_t *prelude_option_new(void);

prelude_option_t *prelude_option_add(prelude_optlist_t *optlist, int flags,
                                     char shortopt, const char *longopt, const char *desc,
                                     prelude_option_argument_t has_arg, int (*set)(const char *optarg));

prelude_option_t *prelude_option_wide_add(prelude_optlist_t *optlist, int flags,
                                          char shortopt, const char *longopt, const char *desc,
                                          prelude_option_argument_t has_arg, int (*set)(const char *optarg),
                                          int (*get)(char *buf, size_t size));


int prelude_option_parse_arguments(prelude_optlist_t *optlist,
                                   const char *filename, int argc, char **argv);


void prelude_option_print(prelude_optlist_t *optlist, int flags);

prelude_msg_t *prelude_option_wide_get_msg(prelude_optlist_t *optlist);


void prelude_option_destroy(prelude_optlist_t *optlist);



#define OPT_INVAL     0x1
#define OPT_INVAL_ARG 0x2

void prelude_option_set_warnings(int flags, int *old_flags);
