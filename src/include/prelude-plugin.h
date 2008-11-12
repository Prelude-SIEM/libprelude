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

#ifndef _LIBPRELUDE_PLUGIN_H
#define _LIBPRELUDE_PLUGIN_H

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "prelude-list.h"
#include "prelude-option.h"

#ifdef __cplusplus
extern "C" {
#endif


#define PRELUDE_PLUGIN_API_VERSION 1


typedef struct prelude_plugin_entry prelude_plugin_entry_t;
typedef struct prelude_plugin_instance prelude_plugin_instance_t;


#define PRELUDE_PLUGIN_GENERIC               \
        prelude_plugin_entry_t *_pe;         \
        char *name;                          \
        void (*destroy)(prelude_plugin_instance_t *pi, prelude_string_t *err)


typedef struct {
        PRELUDE_PLUGIN_GENERIC;
} prelude_plugin_generic_t;



/*
 * Hack for plugin preloading,
 * without having the end program depend on ltdl.
 */
#ifdef PRELUDE_APPLICATION_USE_LIBTOOL2
# define lt_preloaded_symbols lt__PROGRAM__LTX_preloaded_symbols
#endif

extern const void *lt_preloaded_symbols[];

#define PRELUDE_PLUGIN_SET_PRELOADED_SYMBOLS()         \
        prelude_plugin_set_preloaded_symbols(lt_preloaded_symbols)


#define PRELUDE_PLUGIN_OPTION_DECLARE_STRING_CB(prefix, type, name)                              \
static int prefix ## _set_ ## name(prelude_option_t *opt, const char *optarg, prelude_string_t *err, void *context)  \
{                                                                                                \
        char *dup = NULL;                                                                        \
        type *ptr = prelude_plugin_instance_get_plugin_data(context);                            \
                                                                                                 \
        if ( optarg ) {                                                                          \
                dup = strdup(optarg);                                                            \
                if ( ! dup )                                                                     \
                        return prelude_error_from_errno(errno);                                  \
        }                                                                                        \
                                                                                                 \
        if ( ptr->name )                                                                         \
                free(ptr->name);                                                                 \
                                                                                                 \
        ptr->name = dup;                                                                         \
                                                                                                 \
        return 0;                                                                                \
}                                                                                                \
                                                                                                 \
                                                                                                 \
static int prefix ## _get_ ## name(prelude_option_t *opt, prelude_string_t *out, void *context)  \
{                                                                                                \
        type *ptr = prelude_plugin_instance_get_plugin_data(context);                            \
        if ( ptr->name )                                                                         \
                prelude_string_cat(out, ptr->name);                                              \
                                                                                                 \
        return 0;                                                                                \
}


/*
 *
 */
#define prelude_plugin_get_name(p) (p)->name

#define prelude_plugin_set_name(p, str) (p)->name = (str)

#define prelude_plugin_set_destroy_func(p, func) (p)->destroy = func




/*
 * Plugin need to call this function in order to get registered.
 */
void prelude_plugin_entry_set_plugin(prelude_plugin_entry_t *pe, prelude_plugin_generic_t *pl);

int prelude_plugin_set_activation_option(prelude_plugin_entry_t *pe, prelude_option_t *opt,
                                         int (*commit)(prelude_plugin_instance_t *pi, prelude_string_t *err));

int prelude_plugin_instance_subscribe(prelude_plugin_instance_t *pi);

int prelude_plugin_instance_unsubscribe(prelude_plugin_instance_t *pi);


int prelude_plugin_new_instance(prelude_plugin_instance_t **pi,
                                prelude_plugin_generic_t *plugin, const char *name, void *data);


/*
 *
 */
prelude_plugin_generic_t *prelude_plugin_search_by_name(prelude_list_t *head, const char *name);

prelude_plugin_instance_t *prelude_plugin_search_instance_by_name(prelude_list_t *head,
                                                                  const char *pname, const char *iname);


void prelude_plugin_instance_set_data(prelude_plugin_instance_t *pi, void *data);

void *prelude_plugin_instance_get_data(prelude_plugin_instance_t *pi);

void prelude_plugin_instance_set_plugin_data(prelude_plugin_instance_t *pi, void *data);

void *prelude_plugin_instance_get_plugin_data(prelude_plugin_instance_t *pi);

const char *prelude_plugin_instance_get_name(prelude_plugin_instance_t *pi);

prelude_plugin_generic_t *prelude_plugin_instance_get_plugin(prelude_plugin_instance_t *pi);


/*
 * Load all plugins in directory 'dirname'.
 * The CB arguments will be called for each plugin that register
 * (using the plugin_register function), then the application will
 * have the ability to use plugin_register_for_use to tell it want
 * to use this plugin.
 */
int prelude_plugin_load_from_dir(prelude_list_t *head,
                                 const char *dirname, const char *symbol, void *ptr,
                                 int (*subscribe)(prelude_plugin_instance_t *p),
                                 void (*unsubscribe)(prelude_plugin_instance_t *pi));


/*
 * Call this if you want to use this plugin.
 */
int prelude_plugin_instance_add(prelude_plugin_instance_t *pi, prelude_list_t *h);

void prelude_plugin_instance_del(prelude_plugin_instance_t *pi);

void prelude_plugin_instance_compute_time(prelude_plugin_instance_t *pi,
                                          struct timeval *start, struct timeval *end);


int prelude_plugin_instance_call_commit_func(prelude_plugin_instance_t *pi, prelude_string_t *err);

prelude_bool_t prelude_plugin_instance_has_commit_func(prelude_plugin_instance_t *pi);

void prelude_plugin_set_preloaded_symbols(void *symlist);

prelude_plugin_generic_t *prelude_plugin_get_next(prelude_list_t *head, prelude_list_t **iter);

void prelude_plugin_unload(prelude_plugin_generic_t *plugin);


/*
 *
 */
#define prelude_plugin_compute_stats(pi, func) do {                            \
        struct timeval start, end;                                             \
        gettimeofday(&start, NULL);                                            \
        (func);                                                                \
        gettimeofday(&end, NULL);                                              \
        prelude_plugin_instance_compute_time(&start, &end);                    \
} while(0)


/*
 * Macro used to start a plugin.
 */
#define prelude_plugin_run(pi, type, member, ...) \
        (((type *)prelude_plugin_instance_get_plugin(pi))->member(__VA_ARGS__))

#ifdef __cplusplus
 }
#endif

#endif /* _LIBPRELUDE_PLUGIN_H */
