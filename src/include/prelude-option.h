/*****
*
* Copyright (C) 2001, 2002, 2003, 2004, 2005 PreludeIDS Technologies. All Rights Reserved.
* Author: Yoann Vandoorselaere <yoann.v@prelude-ids.com>
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

#ifndef _LIBPRELUDE_PRELUDE_GETOPT_H
#define _LIBPRELUDE_PRELUDE_GETOPT_H

#include "prelude-msgbuf.h"


#ifdef __cplusplus
 extern "C" {
#endif

typedef enum {
        PRELUDE_OPTION_TYPE_CLI  = 0x01,
        PRELUDE_OPTION_TYPE_CFG  = 0x02,
        PRELUDE_OPTION_TYPE_WIDE = 0x04,
        PRELUDE_OPTION_TYPE_CONTEXT = 0x08,
        PRELUDE_OPTION_TYPE_ROOT    = 0x10,
        PRELUDE_OPTION_TYPE_DESTROY = 0x20
} prelude_option_type_t;


typedef enum {
        PRELUDE_OPTION_INPUT_TYPE_STRING   = 1,
        PRELUDE_OPTION_INPUT_TYPE_INTEGER  = 2,
        PRELUDE_OPTION_INPUT_TYPE_BOOLEAN  = 3
} prelude_option_input_type_t;


typedef struct prelude_option prelude_option_t;
typedef struct prelude_option_context prelude_option_context_t;

typedef int (*prelude_option_destroy_callback_t)(prelude_option_t *opt, prelude_string_t *out, void *context);
typedef int (*prelude_option_commit_callback_t)(prelude_option_t *opt, prelude_string_t *out, void *context);
typedef int (*prelude_option_get_callback_t)(prelude_option_t *opt, prelude_string_t *out, void *context);
typedef int (*prelude_option_set_callback_t)(prelude_option_t *opt, const char *optarg, prelude_string_t *err, void *context);


typedef enum {
        PRELUDE_OPTION_ARGUMENT_REQUIRED = 1,
        PRELUDE_OPTION_ARGUMENT_OPTIONAL = 2,
        PRELUDE_OPTION_ARGUMENT_NONE     = 3
} prelude_option_argument_t;


typedef enum {
        PRELUDE_OPTION_PRIORITY_IMMEDIATE = -2,
        PRELUDE_OPTION_PRIORITY_FIRST     = -1,
        PRELUDE_OPTION_PRIORITY_NONE      =  0,
        PRELUDE_OPTION_PRIORITY_LAST      =  2
} prelude_option_priority_t;


typedef enum {
        PRELUDE_OPTION_WARNING_OPTION    = 0x1,
        PRELUDE_OPTION_WARNING_ARG       = 0x2
} prelude_option_warning_t;


void prelude_option_set_priority(prelude_option_t *option, prelude_option_priority_t priority);


void prelude_option_print(prelude_option_t *opt, prelude_option_type_t type, int descoff, FILE *fd);

int prelude_option_wide_send_msg(prelude_msgbuf_t *msgbuf, void *context);

void prelude_option_destroy(prelude_option_t *option);

int prelude_option_read(prelude_option_t *option, const char **filename,
                        int *argc, char **argv, prelude_string_t **err, void *context);


int prelude_option_add(prelude_option_t *parent, prelude_option_t **retopt, prelude_option_type_t type,
                       char shortopt, const char *longopt, const char *desc, prelude_option_argument_t has_arg,
                       int (*set)(prelude_option_t *opt, const char *optarg, prelude_string_t *err, void *context),
                       int (*get)(prelude_option_t *opt, prelude_string_t *out, void *context));

void prelude_option_set_type(prelude_option_t *opt, prelude_option_type_t type);

prelude_option_type_t prelude_option_get_type(prelude_option_t *opt);

void prelude_option_set_warnings(prelude_option_warning_t new_warnings, prelude_option_warning_t *old_warnings);

char prelude_option_get_shortname(prelude_option_t *opt);

const char *prelude_option_get_longname(prelude_option_t *opt);

void _prelude_option_set_private_data(prelude_option_t *opt, void *data);

void *_prelude_option_get_private_data(prelude_option_t *opt);

void prelude_option_set_data(prelude_option_t *opt, void *data);

void *prelude_option_get_data(prelude_option_t *opt);


int prelude_option_invoke_commit(prelude_option_t *opt, const char *ctname, prelude_string_t *value, void *context);

int prelude_option_invoke_set(prelude_option_t *opt, const char *ctname, prelude_string_t *value, void **context);

int prelude_option_invoke_get(prelude_option_t *opt, const char *ctname, prelude_string_t *value, void *context);

int prelude_option_invoke_destroy(prelude_option_t *opt, const char *ctname, prelude_string_t *value, void *context);


/*
 *
 */
int prelude_option_new_root(prelude_option_t **retopt);

int prelude_option_new(prelude_option_t *parent, prelude_option_t **retopt);

void prelude_option_set_longopt(prelude_option_t *opt, const char *longopt);

const char *prelude_option_get_longopt(prelude_option_t *opt);

void prelude_option_set_description(prelude_option_t *opt, const char *description);

const char *prelude_option_get_description(prelude_option_t *opt);

void prelude_option_set_has_arg(prelude_option_t *opt, prelude_option_argument_t has_arg);

prelude_option_argument_t prelude_option_get_has_arg(prelude_option_t *opt);

void prelude_option_set_value(prelude_option_t *opt, const char *value);

const char *prelude_option_get_value(prelude_option_t *opt);

void prelude_option_set_help(prelude_option_t *opt, const char *help);

const char *prelude_option_get_help(prelude_option_t *opt);

void prelude_option_set_input_validation_regex(prelude_option_t *opt, const char *regex);

const char *prelude_option_get_input_validation_regex(prelude_option_t *opt);

void prelude_option_set_input_type(prelude_option_t *opt, prelude_option_input_type_t input_type);

prelude_option_input_type_t prelude_option_get_input_type(prelude_option_t *opt);

prelude_list_t *prelude_option_get_optlist(prelude_option_t *opt);

prelude_option_t *prelude_option_get_next(prelude_option_t *start, prelude_option_t *cur);

prelude_bool_t prelude_option_has_optlist(prelude_option_t *opt);

prelude_option_t *prelude_option_get_parent(prelude_option_t *opt);


void prelude_option_set_destroy_callback(prelude_option_t *opt,
                                         prelude_option_destroy_callback_t destroy);

prelude_option_destroy_callback_t prelude_option_get_destroy_callback(prelude_option_t *opt);


void prelude_option_set_set_callback(prelude_option_t *opt,
                                     prelude_option_set_callback_t set);

prelude_option_set_callback_t prelude_option_get_set_callback(prelude_option_t *opt);


void prelude_option_set_get_callback(prelude_option_t *opt,
                                     int (*get)(prelude_option_t *opt, prelude_string_t *out, void *context));

prelude_option_get_callback_t prelude_option_get_get_callback(prelude_option_t *opt);

void prelude_option_set_commit_callback(prelude_option_t *opt, prelude_option_commit_callback_t commit);

prelude_option_commit_callback_t prelude_option_get_commit_callback(prelude_option_t *opt);

void prelude_option_set_default_context(prelude_option_t *opt, void *context);

int prelude_option_new_context(prelude_option_t *opt, prelude_option_context_t **ctx, const char *name, void *data);

void prelude_option_context_destroy(prelude_option_context_t *oc);

void *prelude_option_context_get_data(prelude_option_context_t *oc);

void prelude_option_context_set_data(prelude_option_context_t *oc, void *data);

prelude_option_t *prelude_option_search(prelude_option_t *parent, const char *name,
                                        prelude_option_type_t type, prelude_bool_t walk_children);

prelude_option_context_t *prelude_option_search_context(prelude_option_t *opt, const char *name);

#ifdef __cplusplus
 }
#endif

#endif /* _LIBPRELUDE_PRELUDE_GETOPT_H */
