/*****
*
* Copyright (C) 1998-2004 Yoann Vandoorselaere <yoann@prelude-ids.org>
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

#include "config.h"
#include "prelude-log.h"
#include "variable.h"
#include "inttypes.h"
#include "prelude-io.h"
#include "prelude-message.h"
#include "prelude-getopt.h"
#include "prelude-linked-object.h"
#include "prelude-plugin.h"
#include "config-engine.h"


#define DEFAULT_INSTANCE_NAME "default"


typedef struct {
        int count;
        int (*subscribe)(prelude_plugin_instance_t *pc);
        void (*unsubscribe)(prelude_plugin_instance_t *pc);
} libltdl_data_t;



typedef struct {
        void *handle;
        prelude_list_t list;        
        prelude_list_t instance_list;

        char *tmp_instance_name;
        prelude_plugin_generic_t *plugin;

        int (*subscribe)(prelude_plugin_instance_t *pc);
        void (*unsubscribe)(prelude_plugin_instance_t *pc);
        
        int (*init_instance)(prelude_plugin_instance_t *pi);
        int (*create_instance)(prelude_plugin_instance_t *pi, prelude_option_t *opt, const char *optarg);
} plugin_entry_t;



typedef struct {
        plugin_entry_t *entry;

        int is_activation;
        
        union {
                int (*plugin_init_cb)(prelude_plugin_instance_t *pi);
                int (*plugin_option_set_cb)(prelude_plugin_instance_t *pi, prelude_option_t *opt, const char *optarg);        
        } func;

        int (*plugin_option_get_cb)(prelude_plugin_instance_t *pi, prelude_option_t *opt, const char *optarg);        
        
} plugin_option_intercept_t;



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
        char *name;
        const char *infos;
        
        /*
         * Instance running time and count.
         */
        double time;
        uint32_t count;

        int already_used;
        int already_subscribed;
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

static PRELUDE_LIST_HEAD(all_plugins);



static plugin_entry_t *search_plugin_entry(prelude_plugin_generic_t *plugin)
{
        plugin_entry_t *pe;
        prelude_list_t *tmp;
        
        prelude_list_for_each_reversed(tmp, &all_plugins) {
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
        
        prelude_list_for_each(tmp, &all_plugins) {
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
        
        prelude_list_for_each(tmp, &pe->instance_list) {
                pi = prelude_list_entry(tmp, prelude_plugin_instance_t, int_list);
                
                if ( strcasecmp(pi->name, name) == 0 )
                        return pi;
        }

        return NULL;
}



static prelude_plugin_instance_t *create_instance(plugin_entry_t *pe, const char *name, void *data) 
{
        prelude_plugin_instance_t *pi;
                        
        pi = calloc(1, sizeof(*pi));
        if ( ! pi ) {
                log(LOG_ERR, "memory exhausted.\n");
                return NULL;
        }

        if ( ! name || *name == 0 )
                name = DEFAULT_INSTANCE_NAME;
        
        pi->name = strdup(name);
        if ( ! pi->name ) {
                log(LOG_ERR, "memory exhausted.\n");
                free(pi);
                return NULL;
        }

        pi->entry = pe;
        pi->data = data;
        
        prelude_list_add_tail(&pi->int_list, &pe->instance_list);
        
        return pi;
}




static int subscribe_instance(prelude_plugin_instance_t *pi)
{
        int ret = 0;
        
        if ( pi->entry->subscribe )
                ret = pi->entry->subscribe(pi);

        pi->already_subscribed = 1;
        
        return ret;
}




static void destroy_instance(prelude_plugin_instance_t *instance)
{
        free(instance->name);
        
        prelude_list_del(&instance->int_list);

        free(instance);
}




#if 0
/*
 * FIXME: reactivate once the prelude-getopt get() option callback
 * is fixed: pass the option as an argument + fix the way we are
 * writing the output.
 */
static int intercept_plugin_option_get(void **context, char *buf, size_t size)
{
        plugin_option_intercept_t *intercept;
        prelude_plugin_instance_t *pi = *context;
        
        intercept = prelude_option_get_private_data(opt);
        assert(intercept);

        return intercept->plugin_option_get_cb(pi, buf, size);
}
#endif



static int intercept_plugin_option_set(void **context, prelude_option_t *opt, const char *name)
{
        plugin_option_intercept_t *intercept;
        prelude_plugin_instance_t *pi = *context;

        if ( ! *context ) {
                log(LOG_ERR, "referenced instance not available.\n");
                return -1;
        }
        
        intercept = prelude_option_get_private_data(opt);
        assert(intercept);

        return intercept->func.plugin_option_set_cb(pi, opt, name);
}



static int intercept_plugin_activation_option(void **context, prelude_option_t *opt, const char *name)
{
        int ret = 0;
        plugin_entry_t *pe;
        prelude_plugin_instance_t *pi;
        
        pe = prelude_option_get_private_data(opt);
        assert(pe);
                
        if ( ! name || *name == 0 )
                name = DEFAULT_INSTANCE_NAME;

        pi = search_instance_from_entry(pe, name);
        if ( ! pi ) {
                pi = create_instance(pe, name, NULL);
                if ( ! pi )
                        return -1;

                *context = pi;

                ret = pi->entry->create_instance(pi, opt, name);
                if ( ret < 0 )
                        return -1;

                if ( ! pe->init_instance )
                        ret = subscribe_instance(pi);
        }
        
        *context = pi;

        return ret;
}



static int intercept_plugin_init_option(void **context, prelude_option_t *opt, const char *name)
{
        int ret;
        plugin_entry_t *pe;
        prelude_plugin_instance_t *pi;

        if ( ! *context ) {
                log(LOG_ERR, "referenced instance not available.\n");
                return -1;
        }
        
        pi = *context;
        pe = pi->entry;
                
        ret = pe->init_instance(pi);
        if ( pi->already_subscribed )
                return ret;
        
        if ( ret < 0 ) {
                pe->plugin->destroy(pi);
                destroy_instance(pi);
        } else
                subscribe_instance(pi);

        return ret;
}




static int setup_plugin_option_intercept(plugin_entry_t *pe, prelude_option_t *opt,
                                         int (*intercept)(void **context, prelude_option_t *opt, const char *arg))
{
        plugin_option_intercept_t *new;

        if ( ! prelude_option_get_set_callback(opt) )
                return 0;
        
        new = malloc(sizeof(*new));
        if ( ! new ) {
                log(LOG_ERR, "memory exhausted.\n");
                return -1;
        }

        new->entry = pe;
        new->func.plugin_option_set_cb = prelude_option_get_set_callback(opt);
        
        prelude_option_set_private_data(opt, new);
        prelude_option_set_set_callback(opt, intercept);

        return 0;
}





/*
 * Initialize a new plugin entry, and add it to
 * the entry list.
 */
static plugin_entry_t *add_plugin_entry(void) 
{
        plugin_entry_t *pe;
        
        pe = calloc(1, sizeof(*pe));
        if ( ! pe ) {
                log(LOG_ERR, "memory exhausted.\n");
                return NULL;
        }

        pe->plugin = NULL;
        PRELUDE_INIT_LIST_HEAD(&pe->instance_list);
        
        prelude_list_add_tail(&pe->list, &all_plugins);
        
        return pe;
}




/*
 * Copy an existing container (because the plugin will probably
 * linked in several part of the program, and that the container
 * contain not shared information).
 */
static prelude_plugin_instance_t *copy_instance(prelude_plugin_instance_t *pi) 
{
        prelude_plugin_instance_t *new;
        
        new = malloc(sizeof(*new));
        if ( ! new ) {
                log(LOG_ERR, "memory exhausted.\n");
                return NULL;
        }

        memcpy(new, pi, sizeof(*new));

        new->name = strdup(pi->name);
        if ( ! new->name ) {
                log(LOG_ERR, "memory exhausted.\n");
                free(new);
                return NULL;
        }
        
        prelude_list_add_tail(&new->int_list, &pi->entry->instance_list);
        
        return new;
}





/*
 * Load a single plugin pointed to by 'filename'.
 */
static int plugin_load_single(const char *filename,
                              int (*subscribe)(prelude_plugin_instance_t *pc),
                              void (*unsubscribe)(prelude_plugin_instance_t *pc)) 
{
        void *handle;
        plugin_entry_t *pe;
        prelude_plugin_generic_t *plugin;
        prelude_plugin_generic_t *(*plugin_init)(void);
        
        handle = lt_dlopenext(filename);
        if ( ! handle ) {
                log(LOG_INFO, "couldn't open %s : %s.\n", filename, lt_dlerror());
                return -1;
        }
        
        plugin_init = lt_dlsym(handle, "prelude_plugin_init");
        if ( ! plugin_init ) {
                log(LOG_INFO, "couldn't load %s : %s.\n", filename, lt_dlerror());
                lt_dlclose(handle);
                return -1;
        }
        
        pe = add_plugin_entry();
        if ( ! pe ) {
                lt_dlclose(handle);
                return -1;
        }
        
        pe->handle = handle;
        pe->subscribe = subscribe;
        pe->unsubscribe = unsubscribe;
        
        plugin = plugin_init();
        if ( ! plugin ) {
                log(LOG_ERR, "plugin returned an error.\n");
                prelude_list_del(&pe->list);
                lt_dlclose(handle);
                free(pe);
                return -1;
        }

        pe->plugin = plugin;
        
        return 0;
}



static int libltdl_load_cb(const char *filename, lt_ptr ptr)
{
        int ret;
        libltdl_data_t *data = ptr;

        ret = plugin_load_single(filename, data->subscribe, data->unsubscribe);
        if ( ret == 0 )
                data->count++;

        return 0;
}



/**
 * prelude_plugin_load_from_dir:
 * @dirname: The directory to load the plugin from.
 * @argc: Argument count for the plugin.
 * @argv: Argument vector for the plugin.
 * @subscribe: Pointer to a callback function for plugin subscribtion.
 * @unsubscribe: Pointer to a callback function for plugin un-subscribtion.
 *  
 * Load all plugins in directory 'dirname'.
 * Each plugin have a @subscribe and @unsubscribe callback associated with it.
 *
 * The plugins are loaded, but not active, until someone call
 * plugin_subscribe() on one of the plugin. Which'll call @subscribe in order to
 * register the it.
 *
 * @argc and @argv are passed to the plugin at initialization time for
 * option handling.
 *
 * Returns: The number of loaded plugins on success, -1 on error.
 */
int prelude_plugin_load_from_dir(const char *dirname,
                                 int (*subscribe)(prelude_plugin_instance_t *p),
                                 void (*unsubscribe)(prelude_plugin_instance_t *pc)) 
{
        int ret;
        libltdl_data_t data;

        ret = lt_dlinit();
        if ( ret < 0 ) {
                log(LOG_ERR, "error initializing libltdl.\n");
                return -1;
        }

        data.count = 0;
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
prelude_plugin_instance_t *prelude_plugin_new_instance(prelude_plugin_generic_t *plugin, const char *name, void *data)
{
        plugin_entry_t *pe;
        prelude_plugin_instance_t *pi;

        if ( ! name || ! *name )
                name = DEFAULT_INSTANCE_NAME;
                
        pe = search_plugin_entry(plugin);        
        if ( ! pe )
                return NULL;

        /*
         * might be NULL in case the plugin subscribe from the init function.
         */
        pe->plugin = plugin;
        
        pi = search_instance_from_entry(pe, name);
        if ( ! pi ) {
                        
                pi = create_instance(pe, name, NULL);
                if ( ! pi )
                        return NULL;
                
                if ( pe->create_instance && pe->create_instance(pi, NULL, name) < 0 )
                        return NULL;
        }
        
        return pi;
}




int prelude_plugin_subscribe(prelude_plugin_instance_t *pi)
{
        return subscribe_instance(pi);
}




static int plugin_desactivate(void **context, prelude_option_t *opt, const char *arg)
{
        prelude_plugin_instance_t *pi = *context;
        
        if ( ! pi ) {
                log(LOG_ERR, "referenced instance not available.\n");
                return -1;
        }
        
        pi->entry->plugin->destroy(pi);
        prelude_plugin_unsubscribe(pi);

        *context = NULL;
        
        /*
         * so that the plugin init function is not called after unsubscribtion.
         */
        return prelude_option_end;
}




int prelude_plugin_set_activation_option(prelude_plugin_generic_t *plugin,
                                         prelude_option_t *opt, int (*init)(prelude_plugin_instance_t *pi))
{
        int ret = 0;
        plugin_entry_t *pe;
        prelude_option_t *new;
        plugin_option_intercept_t *intercept;
        
        pe = search_plugin_entry(plugin);        
        if ( ! pe )
                return -1;
        
        new = prelude_option_add(opt, CLI_HOOK|WIDE_HOOK, 0, "unsubscribe",
                                 "Unsubscribe this plugin", no_argument,
                                 plugin_desactivate, NULL);
        if ( ! new )
                return -1;
        
        /*
         * We assume that the provided option was created with
         * prelude_plugin_option_add() and thus contain an interceptor.
         */
        intercept = prelude_option_get_private_data(opt);
        assert(intercept);
        pe->create_instance = intercept->func.plugin_option_set_cb;
        free(intercept);
        
        prelude_option_set_set_callback(opt, intercept_plugin_activation_option);
        prelude_option_set_private_data(opt, pe);
        
        /*
         * if an initialisation option is given, set it up.
         */
        if ( init ) {
                new = prelude_option_add_init_func(opt, intercept_plugin_init_option);
                if ( ! new )
                        return -1;

                pe->init_instance = init;
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
        if ( pi->already_used++ && ! (pi = copy_instance(pi)) ) {
                log(LOG_ERR, "couldn't duplicate plugin container.\n");
                return -1;
        }
        
        pi->infos = infos;
        prelude_linked_object_add_tail((prelude_linked_object_t *) pi, h);
        
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
        prelude_linked_object_del((prelude_linked_object_t *) pi);
}



void prelude_plugin_instance_set_data(prelude_plugin_instance_t *pi, void *data)
{
        pi->data = data;
}



void *prelude_plugin_instance_get_data(prelude_plugin_instance_t *pi)
{
        return pi->data;
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





prelude_option_t *prelude_plugin_option_add(prelude_option_t *parent, int flags,
                                            char shortopt, const char *longopt, const char *desc,
                                            prelude_option_argument_t has_arg,
                                            int (*set)(prelude_plugin_instance_t *pi, prelude_option_t *opt, const char *optarg),
                                            int (*get)(prelude_plugin_instance_t *pi, char *buf, size_t size))
{
        int ret;
        prelude_option_t *opt;
        plugin_option_intercept_t *new;

        new = malloc(sizeof(*new));
        if ( ! new ) {
                log(LOG_ERR, "memory exhausted.\n");
                return NULL;
        }
        
        opt = prelude_option_add(parent, flags, shortopt, longopt, desc, has_arg, (void *)set, (void *)get);
        if ( ! opt ) {
                free(new);
                return NULL;
        }

        new->func.plugin_option_set_cb = set;
        prelude_option_set_private_data(opt, new);
        
        ret = setup_plugin_option_intercept(NULL, opt, intercept_plugin_option_set);
        if ( ret < 0 ) {
                free(new);
                free(opt);
                return NULL;
        }
        
        return opt;
}

