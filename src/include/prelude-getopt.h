/*****
*
* Copyright (C) 2001, 2002 Yoann Vandoorselaere <yoann@mandrakesoft.com>
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

#ifndef _LIBPRELUDE_PRELUDE_GETOPT_H
#define _LIBPRELUDE_PRELUDE_GETOPT_H

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


typedef struct prelude_option prelude_option_t;


typedef enum {
        required_argument,
        optionnal_argument,
        no_argument
} prelude_option_argument_t;



/*
 * option callback order
 */
#define option_run_last      -1
#define option_run_first     -2
#define option_run_no_order   0


void prelude_option_set_priority(prelude_option_t *option, int priority);

void prelude_option_print(prelude_option_t *opt, int flags, int descoff);

prelude_msg_t *prelude_option_wide_get_msg(void);

void prelude_option_destroy(prelude_option_t *option);

int prelude_option_parse_arguments(prelude_option_t *option,
                                   const char *filename, int argc, char **argv);

prelude_option_t *prelude_option_add(prelude_option_t *parent, int flags,
                                     char shortopt, const char *longopt, const char *desc,
                                     prelude_option_argument_t has_arg,
                                     int (*set)(prelude_option_t *opt, const char *optarg),
                                     int (*get)(char *buf, size_t size));



#define OPT_INVAL     0x1
#define OPT_INVAL_ARG 0x2

void prelude_option_set_warnings(int flags, int *old_flags);

char prelude_option_get_shortname(prelude_option_t *opt);

const char *prelude_option_get_longname(prelude_option_t *opt);

void prelude_option_set_private_data(prelude_option_t *opt, void *data);

void *prelude_option_get_private_data(prelude_option_t *opt);

int prelude_option_invoke_set(const char *option, const char *value);

int prelude_option_invoke_get(const char *option, char *buf, size_t len);


/*
 *
 */
prelude_option_t *prelude_option_new(prelude_option_t *parent);

void prelude_option_set_longopt(prelude_option_t *opt, const char *longopt);

const char *prelude_option_get_longopt(prelude_option_t *opt);

void prelude_option_set_description(prelude_option_t *opt, const char *description);

const char *prelude_option_get_description(prelude_option_t *opt);

void prelude_option_set_has_arg(prelude_option_t *opt, prelude_option_argument_t has_arg);

prelude_option_argument_t prelude_option_get_has_arg(prelude_option_t *opt);

void prelude_option_set_help(prelude_option_t *opt, const char *help);

const char *prelude_option_get_help(prelude_option_t *opt);

void prelude_option_set_input_validation_regex(prelude_option_t *opt, const char *regex);

const char *prelude_option_get_input_validation_regex(prelude_option_t *opt);

void prelude_option_set_input_type(prelude_option_t *opt, uint8_t input_type);

uint8_t prelude_option_get_input_type(prelude_option_t *opt);

prelude_option_t *prelude_option_get_root(void);

#endif /* _LIBPRELUDE_PRELUDE_GETOPT_H */
