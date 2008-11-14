/*****
*
* Copyright (C) 1998-2005,2006,2007 PreludeIDS Technologies. All Rights Reserved.
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

#include "config.h"
#include "libmissing.h"

#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>
#include <assert.h>

#if TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# if HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif

#include <ltdl.h>

#include "prelude-log.h"
#include "variable.h"
#include "prelude-inttypes.h"
#include "prelude-io.h"
#include "prelude-option.h"
#include "prelude-linked-object.h"
#include "prelude-plugin.h"
#include "prelude-error.h"
#include "config-engine.h"


#define DEFAULT_INSTANCE_NAME "default"


typedef struct {
        int count;
        void *data;
        const char *symbol;
        prelude_list_t *head;
        int (*subscribe)(prelude_plugin_instance_t *pc);
        void (*unsubscribe)(prelude_plugin_instance_t *pc);
} libltdl_data_t;



struct prelude_plugin_entry {
        prelude_list_t list;

        void *handle;
        prelude_list_t instance_list;

        prelude_option_t *root_opt;
        prelude_plugin_generic_t *plugin;

        int (*subscribe)(prelude_plugin_instance_t *pc);
        void (*unsubscribe)(prelude_plugin_instance_t *pc);

        int (*commit_instance)(prelude_plugin_instance_t *pi, prelude_string_t *out);
        int (*create_instance)(prelude_option_t *opt,
                               const char *optarg, prelude_string_t *err, void *context);
};



struct prelude_plugin_instance {
        /*
         * List members for external list (outside library).
         */
        PRELUDE_LINKED_OBJECT;

        /*
         * List members for internal list (inside plugin_entry).
         */
        prelude_list_t int_list;

        /*
         * pointer to the plugin
         */
        prelude_plugin_entry_t *entry;

        /*
         * information about this instance.
         */
        void *data;
        void *plugin_data;

        char *name;

        /*
         * Instance running time and count.
         */
        double time;
        unsigned count;

        unsigned int already_used;
        prelude_bool_t already_subscribed;
};



/*
 * Some definition :
 *      - Plugin Entry (plugin_entry_t) :
 *        only used here, keep track of all plugin and their container.
 *
 *      - Plugin Instance (plugin_instance_t) :
 *        Contain a pointer on the plugin, and informations about the
 *        plugin that can't be shared. The instances  are what the
 *        structure that external application should use to access the
 *        plugin.
 *
 *      - The Plugin (plugin_generic_t) :
 *        Contain shared plugin data.
 */

static PRELUDE_LIST(all_plugins);
static unsigned int plugin_count = 0;
static prelude_bool_t ltdl_need_init = TRUE;



static prelude_plugin_entry_t *search_plugin_entry_by_name(prelude_list_t *head, const char *name)
{
        prelude_list_t *tmp;
        prelude_plugin_entry_t *pe;

        if ( ! head )
                head = &all_plugins;

        prelude_list_for_each(head, tmp) {
                pe = prelude_list_entry(tmp, prelude_plugin_entry_t, list);

                if ( pe->plugin && strcasecmp(pe->plugin->name, name) == 0 )
                        return pe;
        }

        return NULL;
}




static prelude_plugin_instance_t *search_instance_from_entry(prelude_plugin_entry_t *pe, const char *name)
{
        prelude_list_t *tmp;
        prelude_plugin_instance_t *pi;

        prelude_list_for_each(&pe->instance_list, tmp) {
                pi = prelude_list_entry(tmp, prelude_plugin_instance_t, int_list);

                if ( strcasecmp(pi->name, name) == 0 )
                        return pi;
        }

        return NULL;
}



static int create_instance(prelude_plugin_instance_t **npi,
                           prelude_plugin_entry_t *pe, const char *name, void *data)
{
        prelude_plugin_instance_t *pi;

        *npi = pi = calloc(1, sizeof(*pi));
        if ( ! pi )
                return prelude_error_from_errno(errno);

        if ( ! name || *name == 0 )
                name = DEFAULT_INSTANCE_NAME;

        pi->name = strdup(name);
        if ( ! pi->name ) {
                free(pi);
                return prelude_error_from_errno(errno);
        }

        pi->entry = pe;
        pi->data = data;

        prelude_list_add_tail(&pe->instance_list, &pi->int_list);

        return 0;
}




static void destroy_instance(prelude_plugin_instance_t *instance)
{
        free(instance->name);

        prelude_list_del(&instance->int_list);

        free(instance);
}




static int plugin_desactivate(prelude_option_t *opt, prelude_string_t *out, void *context)
{
        prelude_plugin_instance_t *pi = context;

        if ( ! pi ) {
                prelude_string_sprintf(out, "referenced instance not available");
                return -1;
        }

        if ( pi->entry->plugin->destroy ) {
                int ret;
                prelude_bool_t need_free = FALSE;

                if ( ! out ) {
                        ret = prelude_string_new(&out);
                        if ( ret < 0 )
                                return ret;

                        need_free = TRUE;
                }

                pi->entry->plugin->destroy(pi, out);
                if ( need_free )
                        prelude_string_destroy(out);


                /*
                 * prevent unsubscribe from destroying it again
                 */
                pi->entry->plugin->destroy = NULL;
        }

        return prelude_plugin_instance_unsubscribe(pi);
}




static int intercept_plugin_activation_option(prelude_option_t *opt, const char *optarg,
                                              prelude_string_t *err, void *context)
{
        int ret = 0;
        prelude_plugin_entry_t *pe;
        prelude_plugin_instance_t *pi;
        prelude_option_context_t *octx;

        pe = _prelude_option_get_private_data(opt);
        assert(pe);

        if ( ! optarg || ! *optarg )
                optarg = DEFAULT_INSTANCE_NAME;

        pi = search_instance_from_entry(pe, optarg);
        if ( ! pi ) {
                ret = create_instance(&pi, pe, optarg, NULL);
                if ( ret < 0 )
                        return ret;

                ret = pi->entry->create_instance(opt, optarg, err, pi);
                if ( ret < 0 )
                        return ret;

                ret = prelude_option_new_context(opt, &octx, optarg, pi);
                if ( ret < 0 ) {
                        destroy_instance(pi);
                        return ret;
                }

                if ( ! pe->commit_instance )
                        ret = prelude_plugin_instance_subscribe(pi);
        }

        return ret;
}



static int intercept_plugin_commit_option(prelude_option_t *opt, prelude_string_t *out, void *context)
{
        int ret;
        prelude_plugin_entry_t *pe;
        prelude_plugin_instance_t *pi = context;

        if ( ! pi ) {
                prelude_string_sprintf(out, "referenced instance not available");
                return -1;
        }

        pe = pi->entry;

        ret = pe->commit_instance(pi, out);
        if ( pi->already_subscribed )
                return ret;

        if ( ret == 0 )
                prelude_plugin_instance_subscribe(pi);

        return ret;
}




/*
 * Initialize a new plugin entry, and add it to
 * the entry list.
 */
static int add_plugin_entry(prelude_list_t *head, prelude_plugin_entry_t **pe)
{
        *pe = calloc(1, sizeof(**pe));
        if ( ! *pe )
                return prelude_error_from_errno(errno);

        (*pe)->plugin = NULL;

        prelude_list_init(&(*pe)->instance_list);
        prelude_list_add_tail(head, &(*pe)->list);

        return 0;
}




/*
 * Copy an existing container (because the plugin will probably
 * linked in several part of the program, and that the container
 * contain not shared information).
 */
static int copy_instance(prelude_plugin_instance_t **dst, prelude_plugin_instance_t *src)
{
        *dst = malloc(sizeof(**dst));
        if ( ! *dst )
                return prelude_error_from_errno(errno);

        memcpy(*dst, src, sizeof(**dst));

        (*dst)->name = strdup(src->name);
        if ( ! (*dst)->name ) {
                free(*dst);
                return prelude_error_from_errno(errno);
        }

        prelude_list_add_tail(&src->entry->instance_list, &(*dst)->int_list);

        return 0;
}



/*
 * lt_dlopenext will fail looking up the symbol in case the libtool
 * archive is missing.
 */
static const char *libtool_is_buggy(const char *pname, const char *sym, char *out, size_t size)
{
        size_t i;

        for ( i = 0; i < size && pname[i]; i++ ) {

                if ( isalnum((int) pname[i]) )
                        out[i] = pname[i];
                else
                        out[i] = '_';
        }

        snprintf(out + i, size - i, "_LTX_%s", sym);

        return out;
}



/*
 * Load a single plugin pointed to by 'filename'.
 */
static int plugin_load_single(prelude_list_t *head,
                              const char *filename, const char *symbol, void *data,
                              int (*subscribe)(prelude_plugin_instance_t *pc),
                              void (*unsubscribe)(prelude_plugin_instance_t *pc))
{
        int ret;
        void *handle;
        const char *pname;
        prelude_plugin_entry_t *pe;
        char buf[1024];
        prelude_bool_t buggy_libtool = FALSE;
        int (*plugin_version)(void);
        int (*plugin_init)(prelude_plugin_entry_t *pe, void *data);

        handle = lt_dlopenext(filename);
        if ( ! handle ) {
                prelude_log(PRELUDE_LOG_WARN, "%s: %s.\n", filename, lt_dlerror());
                return -1;
        }

        pname = strrchr(filename, '/');
        pname = (pname) ? pname + 1 : filename;

        plugin_version = lt_dlsym(handle, "prelude_plugin_version");
        if ( ! plugin_version ) {
                buggy_libtool = TRUE;
                plugin_version = lt_dlsym(handle, libtool_is_buggy(pname, "prelude_plugin_version", buf, sizeof(buf)));

        }

        if ( ! plugin_version ) {
                prelude_log(PRELUDE_LOG_WARN, "%s: %s.\n", pname, lt_dlerror());
                lt_dlclose(handle);
                return -1;
        }

        ret = plugin_version();
        if ( ret != PRELUDE_PLUGIN_API_VERSION ) {
                prelude_log(PRELUDE_LOG_WARN, "%s: API version %d does not match plugin API version %d.\n",
                            pname, ret, PRELUDE_PLUGIN_API_VERSION);
                lt_dlclose(handle);
                return -1;
        }

        plugin_init = lt_dlsym(handle, buggy_libtool ? libtool_is_buggy(pname, symbol, buf, sizeof(buf)) : symbol);
        if ( ! plugin_init ) {
                prelude_log(PRELUDE_LOG_WARN, "%s: plugin initialization failed: '%s'.\n", pname, lt_dlerror());
                lt_dlclose(handle);
                return -1;
        }

        ret = add_plugin_entry(head, &pe);
        if ( ret < 0 ) {
                lt_dlclose(handle);
                return ret;
        }

        pe->handle = handle;
        pe->subscribe = subscribe;
        pe->unsubscribe = unsubscribe;

        ret = plugin_init(pe, data);
        if ( ret < 0 || ! pe->plugin ) {
                prelude_log(PRELUDE_LOG_WARN, "%s initialization failure.\n", filename);
                prelude_list_del(&pe->list);
                lt_dlclose(handle);
                free(pe);
                return -1;
        }

        return 0;
}



static int libltdl_load_cb(const char *filename, lt_ptr ptr)
{
        int ret;
        libltdl_data_t *data = ptr;

        ret = plugin_load_single(data->head, filename, data->symbol,
                                 data->data, data->subscribe, data->unsubscribe);
        if ( ret == 0 )
                data->count++;

        return 0;
}



/**
 * prelude_plugin_load_from_dir:
 * @head: List where the loaded plugin should be added.
 * @dirname: The directory to load the plugin from.
 * @symbol: Symbol to lookup within loaded plugin.
 * @ptr: Extra pointer to provide to the plugin initialization function.
 * @subscribe: Pointer to a callback function for plugin subscribtion.
 * @unsubscribe: Pointer to a callback function for plugin un-subscribtion.
 *
 * Load all plugins in directory 'dirname', using @symbol entry point.
 * Each plugin have a @subscribe and @unsubscribe callback associated with it.
 *
 * The plugins are loaded, but not active, until someone call prelude_plugin_subscribe()
 * on one of the plugin. Which'll call @subscribe in order to register it.
 *
 * @ptr is an extra argument provided to the plugin at initialization time.
 *
 * Returns: The number of loaded plugins on success, -1 on error.
 */
int prelude_plugin_load_from_dir(prelude_list_t *head,
                                 const char *dirname, const char *symbol, void *ptr,
                                 int (*subscribe)(prelude_plugin_instance_t *p),
                                 void (*unsubscribe)(prelude_plugin_instance_t *pc))
{
        int ret;
        libltdl_data_t data;

        if ( plugin_count == 0 && ltdl_need_init ) {

                ret = lt_dlinit();
                if ( ret < 0 )
                        return prelude_error(PRELUDE_ERROR_PLUGIN_LTDL_INIT);

                ltdl_need_init = FALSE;
        }

        data.count = 0;
        data.data = ptr;
        data.symbol = symbol;
        data.subscribe = subscribe;
        data.unsubscribe = unsubscribe;
        data.head = head ? head : &all_plugins;

        lt_dlforeachfile(dirname, libltdl_load_cb, &data);
        plugin_count += data.count;

        return data.count;
}



int prelude_plugin_new_instance(prelude_plugin_instance_t **pi,
                                prelude_plugin_generic_t *plugin, const char *name, void *data)
{
        int ret = 0;
        prelude_plugin_entry_t *pe;
        prelude_option_context_t *octx;

        if ( ! name || ! *name )
                name = DEFAULT_INSTANCE_NAME;

        pe = plugin->_pe;

        /*
         * might be NULL in case the plugin subscribe from the commit function.
         */
        pe->plugin = plugin;

        *pi = search_instance_from_entry(pe, name);
        if ( ! *pi ) {
                ret = create_instance(pi, pe, name, data);
                if ( ret < 0 )
                        return ret;

                if ( pe->create_instance ) {
                        ret = pe->create_instance(pe->root_opt, name, NULL, *pi);
                        if ( ret < 0 )
                                return ret;
                }

                if ( pe->root_opt ) {
                        ret = prelude_option_new_context(pe->root_opt, &octx, name, *pi);
                        if ( ret < 0 ) {
                                destroy_instance(*pi);
                                return ret;
                        }
                }

                if ( ! pe->commit_instance )
                        ret = prelude_plugin_instance_subscribe(*pi);
        }

        return ret;
}




int prelude_plugin_instance_subscribe(prelude_plugin_instance_t *pi)
{
        int ret = 0;

        if ( pi->entry->subscribe )
                ret = pi->entry->subscribe(pi);

        pi->already_subscribed = TRUE;

        return ret;
}



/**
 * prelude_plugin_instance_unsubscribe:
 * @pi: Pointer to a plugin instance.
 *
 * Set @pi to be inactive.
 *
 * The unsubscribe function specified in plugin_load_from_dir()
 * is called for plugin un-registration and the instance for this
 * plugin is freed.
 *
 * Returns: 0 on success, -1 if an error occured.
 */
int prelude_plugin_instance_unsubscribe(prelude_plugin_instance_t *pi)

{
        int ret;

        if ( pi->entry->plugin->destroy ) {
                prelude_string_t *tmp;

                ret = prelude_string_new(&tmp);
                if ( ret < 0 )
                        return ret;

                pi->entry->plugin->destroy(pi, tmp);
                prelude_string_destroy(tmp);
        }

        if ( pi->already_subscribed && pi->entry->unsubscribe )
                pi->entry->unsubscribe(pi);

        destroy_instance(pi);

        return 0;
}



int prelude_plugin_set_activation_option(prelude_plugin_entry_t *pe,
                                         prelude_option_t *opt, int (*commit)(prelude_plugin_instance_t *pi,
                                                                              prelude_string_t *out))
{
        pe->root_opt = opt;

        prelude_option_set_destroy_callback(opt, plugin_desactivate);
        prelude_option_set_type(opt, prelude_option_get_type(opt) | PRELUDE_OPTION_TYPE_CONTEXT);

        pe->create_instance = prelude_option_get_set_callback(opt);

        prelude_option_set_get_callback(opt, NULL);
        prelude_option_set_set_callback(opt, intercept_plugin_activation_option);
        _prelude_option_set_private_data(opt, pe);

        /*
         * if a commit function is provided, set it up.
         */
        if ( commit ) {
                prelude_option_set_commit_callback(opt, intercept_plugin_commit_option);
                pe->commit_instance = commit;
        }

        return 0;
}




/**
 * prelude_plugin_instance_add:
 * @pi: Pointer to a plugin instance
 * @h: Pointer to a linked list
 *
 * This function add the plugin instance associated with @pi to the linked list
 * specified by @h. If this instance is already used somewhere else, a copy is
 * made, since instance does not share information).
 *
 * Returns: 0 on success or -1 if an error occured.
 */
int prelude_plugin_instance_add(prelude_plugin_instance_t *pi, prelude_list_t *h)
{
        int ret;

        if ( pi->already_used++ ) {
                ret = copy_instance(&pi, pi);
                if ( ret < 0 )
                        return ret;
        }

        prelude_linked_object_add_tail(h, (prelude_linked_object_t *) pi);

        return 0;
}




/**
 * prelude_plugin_instance_del:
 * @pi: Pointer to a plugin instance.
 *
 * Delete @pi from the list specified at prelude_plugin_instance_add() time.
 */
void prelude_plugin_instance_del(prelude_plugin_instance_t *pi)
{
        assert(pi->already_used);

        pi->already_used--;
        prelude_linked_object_del((prelude_linked_object_t *) pi);
}




/**
 * prelude_plugin_search_by_name:
 * @head: List where to search the plugin from.
 * @name: Name of the plugin to search.
 *
 * Search @head list of plugin for a plugin with name @name.
 *
 * Returns: the a #prelude_plugin_t on success, or NULL if the plugin does not exist.
 */
prelude_plugin_generic_t *prelude_plugin_search_by_name(prelude_list_t *head, const char *name)
{
        prelude_plugin_entry_t *pe;

        pe = search_plugin_entry_by_name(head, name);
        if ( ! pe )
                return NULL;

        return pe->plugin;
}



/**
 * prelude_plugin_instance_search_by_name:
 * @head: List where to search the plugin from.
 * @pname: Name of the plugin to search.
 * @iname: Name of the instance for this plugin.
 *
 * Search @head list of plugin for a plugin @pname with instance @iname.
 *
 * Returns: A #prelude_plugin_instance_t on success, or NULL if the instance does not exit.
 */
prelude_plugin_instance_t *prelude_plugin_search_instance_by_name(prelude_list_t *head,
                                                                  const char *pname, const char *iname)
{
        prelude_plugin_entry_t *pe;

        if ( ! iname )
                iname = DEFAULT_INSTANCE_NAME;

        pe = search_plugin_entry_by_name(head, pname);
        if ( ! pe )
                return NULL;

        return search_instance_from_entry(pe, iname);
}



void prelude_plugin_instance_set_plugin_data(prelude_plugin_instance_t *pi, void *data)
{
        pi->plugin_data = data;
}



void prelude_plugin_instance_set_data(prelude_plugin_instance_t *pi, void *data)
{
        pi->data = data;
}




void *prelude_plugin_instance_get_data(prelude_plugin_instance_t *pi)
{
        return pi->data;
}




void *prelude_plugin_instance_get_plugin_data(prelude_plugin_instance_t *pi)
{
        return pi->plugin_data;
}



const char *prelude_plugin_instance_get_name(prelude_plugin_instance_t *pi)
{
        return pi->name;
}



prelude_plugin_generic_t *prelude_plugin_instance_get_plugin(prelude_plugin_instance_t *pi)
{
        return pi->entry->plugin;
}



void prelude_plugin_instance_compute_time(prelude_plugin_instance_t *pi,
                                          struct timeval *start, struct timeval *end)
{
        pi->time += (double) end->tv_sec + (double) (end->tv_usec * 1e-6);
        pi->time -= (double) start->tv_sec + (double) (start->tv_usec * 1e-6);
        pi->count++;
}



int prelude_plugin_instance_call_commit_func(prelude_plugin_instance_t *pi, prelude_string_t *err)
{
        return pi->entry->commit_instance(pi, err);
}



prelude_bool_t prelude_plugin_instance_has_commit_func(prelude_plugin_instance_t *pi)
{
        return (pi->entry->commit_instance) ? TRUE : FALSE;
}



void prelude_plugin_entry_set_plugin(prelude_plugin_entry_t *pe, prelude_plugin_generic_t *pl)
{
        pl->_pe = pe;
        pe->plugin = pl;
}



void prelude_plugin_set_preloaded_symbols(void *symlist)
{
        unsigned long len;
        lt_dlsymlist *s = symlist;
        static lt_dlsymlist rpl_sym[65535] = {
                { "@PROGNAME@", NULL },
                { NULL, NULL         }
        };

        if ( s[0].name == NULL || strcmp(s[0].name, "@PROGNAME@") != 0 ) {
                /*
                 * Check size of the input symlist.
                 */
                for ( len = 0; s[len].name != NULL; len++ );

                if ( len + 1 >= sizeof(rpl_sym) / sizeof(*rpl_sym) ) {
                        prelude_log(PRELUDE_LOG_CRIT, "replacement symlist is not large enough (%lu entry).\n", len);
                        len = (sizeof(rpl_sym) / sizeof(*rpl_sym)) - 2;
                }

                /*
                 * Copy as many symbols as possible, and set the last entry to NULL.
                 */
                memcpy(&rpl_sym[1], s, len * sizeof(*rpl_sym));
                rpl_sym[len + 1].name = NULL;

                s = rpl_sym;
        }

        lt_dlpreload_default(s);
}



prelude_plugin_generic_t *prelude_plugin_get_next(prelude_list_t *head, prelude_list_t **iter)
{
        prelude_list_t *tmp;
        prelude_plugin_entry_t *pe;

        if ( ! head )
                head = &all_plugins;

        prelude_list_for_each_continue_safe(head, tmp, *iter) {
                pe = prelude_list_entry(tmp, prelude_plugin_entry_t, list);
                return pe->plugin;
        }

        return NULL;
}



void prelude_plugin_unload(prelude_plugin_generic_t *plugin)
{
        prelude_list_t *tmp, *bkp;
        prelude_plugin_entry_t *pe;
        prelude_plugin_instance_t *pi;

        prelude_list_for_each_safe(&plugin->_pe->instance_list, tmp, bkp) {
                pi = prelude_list_entry(tmp, prelude_plugin_instance_t, int_list);
                plugin_desactivate(NULL, NULL, pi);
        }

        pe = plugin->_pe;
        prelude_list_del(&pe->list);

        lt_dlclose(pe->handle);
        free(pe);

        if ( --plugin_count == 0 && ! ltdl_need_init ) {
                lt_dlexit();
                ltdl_need_init = TRUE;
        }
}
