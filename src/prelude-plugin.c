/*****
*
* Copyright (C) 1998,2005 PreludeIDS Technologies. All Rights Reserved.
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

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/time.h>
#include <unistd.h>
#include <assert.h>

#include "ltdl.h"

#include "libmissing.h"
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
        int (*subscribe)(prelude_plugin_instance_t *pc);
        void (*unsubscribe)(prelude_plugin_instance_t *pc);
} libltdl_data_t;



typedef struct {
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
} plugin_entry_t;



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
        plugin_entry_t *entry;

        /*
         * information about this instance.
         */
        void *data;
        void *private_data;
        
        char *name;
        const char *infos;
        
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
static prelude_bool_t ltdl_need_init = TRUE;


static plugin_entry_t *search_plugin_entry(prelude_plugin_generic_t *plugin)
{
        plugin_entry_t *pe;
        prelude_list_t *tmp;
        
        prelude_list_for_each_reversed(&all_plugins, tmp) {
                pe = prelude_list_entry(tmp, plugin_entry_t, list);
                
                if ( pe->plugin == NULL || pe->plugin == plugin )
                        return pe;
        }

        return NULL;
}




static plugin_entry_t *search_plugin_entry_by_name(const char *name) 
{
        plugin_entry_t *pe;
        prelude_list_t *tmp;
        
        prelude_list_for_each(&all_plugins, tmp) {
                pe = prelude_list_entry(tmp, plugin_entry_t, list);

                if ( pe->plugin && strcasecmp(pe->plugin->name, name) == 0 ) 
                        return pe;
        }

        return NULL;
}




static prelude_plugin_instance_t *search_instance_from_entry(plugin_entry_t *pe, const char *name)
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



static int create_instance(prelude_plugin_instance_t **npi, plugin_entry_t *pe, const char *name, void *data) 
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




static int subscribe_instance(prelude_plugin_instance_t *pi)
{
        int ret = 0;
        
        if ( pi->entry->subscribe )
                ret = pi->entry->subscribe(pi);

        pi->already_subscribed = TRUE;
        
        return ret;
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

        /*
         * destroy plugin data.
         */
        pi->entry->plugin->destroy(pi, out);

        /*
         * unsubscribe the plugin, only if it is subscribed
         */
        if ( pi->already_subscribed )
                prelude_plugin_unsubscribe(pi);
        else
                destroy_instance(pi);
        
        return 0;
}




static int intercept_plugin_activation_option(prelude_option_t *opt, const char *optarg,
                                              prelude_string_t *err, void *context)
{
        int ret = 0;
        plugin_entry_t *pe;
        prelude_plugin_instance_t *pi;
        prelude_option_context_t *octx;
        
        pe = prelude_option_get_private_data(opt);
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
                        ret = subscribe_instance(pi);
        }
        
        return ret;
}



static int intercept_plugin_commit_option(prelude_option_t *opt, prelude_string_t *out, void *context)
{
        int ret;
        plugin_entry_t *pe;
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
                subscribe_instance(pi);

        return ret;
}




/*
 * Initialize a new plugin entry, and add it to
 * the entry list.
 */
static int add_plugin_entry(plugin_entry_t **pe) 
{        
        *pe = calloc(1, sizeof(**pe));
        if ( ! *pe )
                return prelude_error_from_errno(errno);

        (*pe)->plugin = NULL;
        
        prelude_list_init(&(*pe)->instance_list);
        prelude_list_add_tail(&all_plugins, &(*pe)->list);
        
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
 * Load a single plugin pointed to by 'filename'.
 */
static int plugin_load_single(const char *filename, const char *symbol, void *data,
                              int (*subscribe)(prelude_plugin_instance_t *pc),
                              void (*unsubscribe)(prelude_plugin_instance_t *pc)) 
{
        int ret;
        void *handle;
        plugin_entry_t *pe;
        prelude_plugin_generic_t *plugin;
        int (*plugin_init)(prelude_plugin_generic_t **pl, void *data);
        
        handle = lt_dlopenext(filename);
        if ( ! handle ) {
                prelude_log(PRELUDE_LOG_WARN, "couldn't open %s : %s.\n", filename, lt_dlerror());
                return -1;
        }
        
        plugin_init = lt_dlsym(handle, symbol);
        if ( ! plugin_init ) {
                prelude_log(PRELUDE_LOG_WARN, "couldn't find symbol %s in %s: %s.\n", symbol, filename, lt_dlerror());
                lt_dlclose(handle);
                return -1;
        }

        ret = add_plugin_entry(&pe);
        if ( ret < 0 ) {
                lt_dlclose(handle);
                return ret;
        }

        pe->handle = handle;
        pe->subscribe = subscribe;
        pe->unsubscribe = unsubscribe;
        
        ret = plugin_init(&plugin, data);
        if ( ret < 0 ) {
                prelude_log(PRELUDE_LOG_WARN, "%s initialization failure.\n", filename);
                prelude_list_del(&pe->list);
                lt_dlclose(handle);
                free(pe);
                return ret;
        }
                
        pe->plugin = plugin;
        
        return 0;
}



static int libltdl_load_cb(const char *filename, lt_ptr ptr)
{
        int ret;
        libltdl_data_t *data = ptr;

        ret = plugin_load_single(filename, data->symbol, data->data, data->subscribe, data->unsubscribe);
        if ( ret == 0 )
                data->count++;

        return 0;
}



/**
 * prelude_plugin_load_from_dir:
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
int prelude_plugin_load_from_dir(const char *dirname, const char *symbol, void *ptr,
                                 int (*subscribe)(prelude_plugin_instance_t *p),
                                 void (*unsubscribe)(prelude_plugin_instance_t *pc)) 
{
        int ret;
        libltdl_data_t data;

        if ( ltdl_need_init ) {

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
        
        lt_dlforeachfile(dirname, libltdl_load_cb, &data);
        
        return data.count;
}





/**
 * prelude_plugin_unsubscribe:
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
int prelude_plugin_unsubscribe(prelude_plugin_instance_t *pi) 
{
        if ( pi->entry->unsubscribe )
                pi->entry->unsubscribe(pi);
        
        destroy_instance(pi);
        
        return 0;
}




/**
 * prelude_plugin_subscribe
 * @pi: Pointer to a plugin instance.
 *
 * Set @pi to be active.
 *
 * The subscribe function specified in plugin_load_from_dir()
 * is called for plugin registration. A new plugin instance is
 * created.
 *
 * Returns: 0 on success or -1 if an error occured.
 */
int prelude_plugin_new_instance(prelude_plugin_instance_t **pi,
                                prelude_plugin_generic_t *plugin, const char *name, void *data)
{
        int ret = 0;
        plugin_entry_t *pe;
        prelude_option_context_t *octx;
        
        if ( ! name || ! *name )
                name = DEFAULT_INSTANCE_NAME;
                
        pe = search_plugin_entry(plugin);        
        if ( ! pe )
                return -1;

        /*
         * might be NULL in case the plugin subscribe from the commit function.
         */
        pe->plugin = plugin;
        
        *pi = search_instance_from_entry(pe, name);
        if ( ! *pi ) {
                ret = create_instance(pi, pe, name, data);
                if ( ret < 0 )
                        return ret;
                
                ret = pe->create_instance(pe->root_opt, name, NULL, *pi);
                if ( ret < 0 )
                        return ret;
                
                ret = prelude_option_new_context(pe->root_opt, &octx, name, *pi);
                if ( ret < 0 ) {
                        destroy_instance(*pi);
                        return ret;
                }
                
                if ( ! pe->commit_instance )
                        ret = subscribe_instance(*pi);
        }
        
        return ret;
}




int prelude_plugin_subscribe(prelude_plugin_instance_t *pi)
{
        return subscribe_instance(pi);
}




int prelude_plugin_set_activation_option(prelude_plugin_generic_t *plugin,
                                         prelude_option_t *opt, int (*commit)(prelude_plugin_instance_t *pi,
                                                                              prelude_string_t *out))
{
        int ret = 0;
        plugin_entry_t *pe;
        
        pe = search_plugin_entry(plugin);        
        if ( ! pe )
                return -1;

        pe->root_opt = opt;
        
        prelude_option_set_destroy_callback(opt, plugin_desactivate);
        prelude_option_set_type(opt, prelude_option_get_type(opt) | PRELUDE_OPTION_TYPE_CONTEXT);
        
        pe->create_instance = prelude_option_get_set_callback(opt);

        prelude_option_set_get_callback(opt, NULL);
        prelude_option_set_set_callback(opt, intercept_plugin_activation_option);
        prelude_option_set_private_data(opt, pe);
        
        /*
         * if a commit function is provided, set it up.
         */
        if ( commit ) {
                prelude_option_set_commit_callback(opt, intercept_plugin_commit_option);
                pe->commit_instance = commit;
        }

        return ret;
}




/**
 * prelude_plugin_add:
 * @pi: Pointer to a plugin instance
 * @h: Pointer to a linked list
 * @infos: Specific informations to associate with the container.
 *
 * This function add the plugin associated with @pi to the linked list
 * specified by @h. If this container is already used somewhere else,
 * a copy is made, because container doesn't share information).
 *
 * Returns: 0 on success or -1 if an error occured.
 */
int prelude_plugin_add(prelude_plugin_instance_t *pi, prelude_list_t *h, const char *infos) 
{
        int ret;
        
        if ( pi->already_used++ ) {
                ret = copy_instance(&pi, pi);
                if ( ret < 0 )
                        return ret;
        }
        
        pi->infos = infos;
        prelude_linked_object_add_tail(h, (prelude_linked_object_t *) pi);
        
        return 0;
}




/**
 * prelude_plugin_search_by_name:
 * @name: Name of the plugin to search
 *
 * Search the whole plugin list (subscribed and unsubscribed),
 * for a plugin with name @name.
 *
 * Returns: the plugin on success, or NULL if the plugin doesn't exist.
 */
prelude_plugin_generic_t *prelude_plugin_search_by_name(const char *name) 
{
        plugin_entry_t *pe;

        pe = search_plugin_entry_by_name(name);
        if ( ! pe )
                return NULL;

        return pe->plugin;
}



/**
 * prelude_plugin_instance_search_by_name:
 *
 */
prelude_plugin_instance_t *prelude_plugin_search_instance_by_name(const char *pname, const char *iname)
{
        plugin_entry_t *pe;

        if ( ! iname )
                iname = DEFAULT_INSTANCE_NAME;
        
        pe = search_plugin_entry_by_name(pname); 
        if ( ! pe )
                return NULL;

        return search_instance_from_entry(pe, iname);
}





/**
 * prelude_plugin_del:
 * @pi: Pointer to a plugin instance.
 *
 * Delete @pi from the list specified at plugin_add() time.
 */
void prelude_plugin_del(prelude_plugin_instance_t *pi) 
{
        assert(pi->already_used);
        
        pi->already_used--;
        prelude_linked_object_del((prelude_linked_object_t *) pi);
}



void prelude_plugin_instance_set_data(prelude_plugin_instance_t *pi, void *data)
{
        pi->data = data;
}



void prelude_plugin_instance_set_private_data(prelude_plugin_instance_t *pi, void *data)
{
        pi->private_data = data;
}




void *prelude_plugin_instance_get_data(prelude_plugin_instance_t *pi)
{
        return pi->data;
}




void *prelude_plugin_instance_get_private_data(prelude_plugin_instance_t *pi)
{
        return pi->private_data;
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



void prelude_plugin_set_preloaded_symbols(void *symlist)
{
        lt_dlpreload_default(symlist);
}



prelude_plugin_generic_t *prelude_plugin_get_next(prelude_list_t **iter)
{
        plugin_entry_t *pe;
        prelude_list_t *tmp;

        prelude_list_for_each_continue_safe(&all_plugins, tmp, *iter) {
                pe = prelude_list_entry(tmp, plugin_entry_t, list);
                return pe->plugin;
        }
        
        return NULL;
}



void prelude_plugin_unload(prelude_plugin_generic_t *plugin)
{
        plugin_entry_t *pe;
        prelude_list_t *tmp, *bkp;
        
        prelude_list_for_each_safe(&all_plugins, tmp, bkp) {
                pe = prelude_list_entry(tmp, plugin_entry_t, list);

                if ( ! plugin || pe->plugin == plugin ) {
                        lt_dlclose(pe->handle);
                        prelude_list_del(&pe->list);
                        free(pe);
                }
        }

        if ( prelude_list_is_empty(&all_plugins) && ! ltdl_need_init ) {
                lt_dlexit();
                ltdl_need_init = TRUE;
        }
}
