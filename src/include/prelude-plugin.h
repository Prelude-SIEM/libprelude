/*****
*
* Copyright (C) 1998 - 2004 Yoann Vandoorselaere <yoann@prelude-ids.org>
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

#ifndef _LIBPRELUDE_PLUGIN_H
#define _LIBPRELUDE_PLUGIN_H


#include "prelude-list.h"
#include "prelude-getopt.h"


typedef struct prelude_plugin_instance prelude_plugin_instance_t;



#define PRELUDE_PLUGIN_OPTION_DECLARE_STRING_CB(prefix, type, name)	 		        \
static int prefix ## _set_ ## name(prelude_plugin_instance_t *pi, prelude_option_t *opt, const char *arg)	\
{                                                                                               \
        char *dup;                                                                              \
        type *ptr = prelude_plugin_instance_get_data(pi);                                       \
                                                                                                \
 	dup = strdup(arg);					                                \
	if ( ! dup ) {   						                        \
		log(LOG_ERR, "memory exhausted.\n");			                        \
		return prelude_option_error;				                        \
	}		                                                                        \
                                                                                                \
        if ( ptr->name )						                        \
		free(ptr->name);			   		                        \
									                        \
        ptr->name = dup;                                                                        \
									                        \
	return prelude_option_success;					                        \
}                                                                                               \
                                                                                                \
                                                                                                \
static int prefix ## _get_ ## name(prelude_plugin_instance_t *pi, char *buf, size_t size)       \
{                                                                                               \
        type *ptr = prelude_plugin_instance_get_data(pi);                                       \
        snprintf(buf, size, "%s", ptr->name);                                                   \
        return prelude_option_success;                                                          \
}





#define PRELUDE_PLUGIN_GENERIC               \
        char *name; int namelen;             \
        char *author; int authorlen;         \
        char *contact; int contactlen;       \
        char *desc; int desclen;             \
        void (*destroy)(prelude_plugin_instance_t *pi)


typedef struct {
        PRELUDE_PLUGIN_GENERIC;
} prelude_plugin_generic_t;


#define prelude_plugin_name(p) (p)->name
#define prelude_plugin_name_len(p) (p)->namelen

#define prelude_plugin_author(p) (p)->author
#define prelude_plugin_author_len(p) (p)->authorlen

#define prelude_plugin_contact(p) (p)->contact
#define prelude_plugin_contact_len(p) (p)->contactlen

#define prelude_plugin_desc(p) (p)->desc
#define prelude_plugin_desc_len(p) (p)->desclen


/*
 *
 */
#define prelude_plugin_set_name(p, str) prelude_plugin_name(p) = (str); \
                                prelude_plugin_name_len(p) = sizeof((str))

#define prelude_plugin_set_author(p, str) prelude_plugin_author(p) = (str); \
                                  prelude_plugin_author_len(p) = sizeof((str))

#define prelude_plugin_set_contact(p, str) prelude_plugin_contact(p) = (str); \
                                   prelude_plugin_contact_len(p) = sizeof((str))

#define prelude_plugin_set_desc(p, str) prelude_plugin_desc(p) = (str); \
                                prelude_plugin_desc_len(p) = sizeof((str))

#define prelude_plugin_set_destroy_func(p, func) (p)->destroy = func



/*
 * Plugin need to call this function in order to get registered.
 */

int prelude_plugin_set_activation_option(prelude_plugin_generic_t *plugin,
                                         prelude_option_t *opt, int (*init)(prelude_plugin_instance_t *pi));

prelude_plugin_instance_t *prelude_plugin_subscribe(prelude_plugin_generic_t *plugin, const char *name, void *data);


int prelude_plugin_unsubscribe(prelude_plugin_instance_t *plugin);


/*
 *
 */
prelude_plugin_generic_t *prelude_plugin_search_by_name(const char *name);

prelude_plugin_instance_t *prelude_plugin_search_instance_by_name(const char *pname, const char *iname);


void prelude_plugin_instance_set_data(prelude_plugin_instance_t *pi, void *data);

void *prelude_plugin_instance_get_data(prelude_plugin_instance_t *pi);

const char *prelude_plugin_instance_get_name(prelude_plugin_instance_t *pi);

prelude_plugin_generic_t *prelude_plugin_instance_get_plugin(prelude_plugin_instance_t *pi);


/*
 * Load all plugins in directory 'dirname'.
 * The CB arguments will be called for each plugin that register
 * (using the plugin_register function), then the application will
 * have the ability to use plugin_register_for_use to tell it want
 * to use this plugin.
 */
int prelude_plugin_load_from_dir(const char *dirname,
                                 int (*subscribe)(prelude_plugin_instance_t *p),
                                 void (*unsubscribe)(prelude_plugin_instance_t *pi));


/*
 * Call this if you want to use this plugin.
 */ 
int prelude_plugin_add(prelude_plugin_instance_t *pc, prelude_list_t *h, const char *infos);

void prelude_plugin_del(prelude_plugin_instance_t *pc);

void prelude_plugin_instance_compute_time(prelude_plugin_instance_t *pi, struct timeval *start, struct timeval *end);


prelude_option_t *prelude_plugin_option_add(prelude_option_t *parent, int flags,
                                            char shortopt, const char *longopt, const char *desc,
                                            prelude_option_argument_t has_arg,
                                            int (*set)(prelude_plugin_instance_t *pi, prelude_option_t *opt, const char *optarg),
                                            int (*get)(prelude_plugin_instance_t *pi, char *buf, size_t size));


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
#define prelude_plugin_run(pi, type, member, arg...) \
        (((type *)prelude_plugin_instance_get_plugin(pi))->member(arg))


/*
 * hook function for a plugin.
 */
prelude_plugin_generic_t *prelude_plugin_init(void);


#endif /* _LIBPRELUDE_PLUGIN_H */

