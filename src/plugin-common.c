/*****
*
* Copyright (C) 1998 - 2000, 2002 Yoann Vandoorselaere <yoann@mandrakesoft.com>
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
#include <dlfcn.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/time.h>
#include <unistd.h>

#include "config.h"
#include "list.h"
#include "prelude-log.h"
#include "variable.h"
#include "plugin-common.h"
#include "plugin-common-prv.h"
#include "config-engine.h"

#ifndef RTLD_NOW
#define RTLD_NOW RTLD_LAZY
#endif


typedef struct {
        void *handle;
        struct list_head list;
        
        struct list_head pclist;
        plugin_generic_t *plugin;

        int (*subscribe)(plugin_container_t *pc);
        void (*unsubscribe)(plugin_container_t *pc);
} plugin_entry_t;


/*
 * Some definition :
 *      - Plugin Entry (plugin_entry_t) :
 *        only used here, keep track of all plugin and their container.
 *
 *      - Plugin Container (plugin_container_t) :
 *        Contain a pointer on the plugin, and informations about the
 *        plugin that can't be shared. The container are what the
 *        structure that external application should use to access the
 *        plugin.
 *
 *      - The Plugin (plugin_generic_t) :
 *        Contain shared plugin data.
 */

/*
 * Internal list
 */
static LIST_HEAD(all_plugin);
static int plugins_id_max = 0;



/*
 * Check if 'filename' look like a plugin.
 * FIXME : do real check.
 */
static int is_a_plugin(const char *filename) 
{
        return ( strstr(filename, ".so") ) ? 0 : -1;
}




/*
 * Initialize a new plugin entry, and add it to
 * the entry list.
 */
static plugin_entry_t *add_plugin_entry(void) 
{
        plugin_entry_t *pe;
        
        pe = malloc(sizeof(plugin_entry_t));
        if ( ! pe ) {
                log(LOG_ERR, "memory exhausted.\n");
                return NULL;
        }

        pe->plugin = NULL;
        INIT_LIST_HEAD(&pe->pclist);
        list_add_tail(&pe->list, &all_plugin);
        
        return pe;
}



/*
 * Destroy all container in the plugin entry.
 */
static void delete_container(plugin_entry_t *entry) 
{
        plugin_container_t *pc;
        struct list_head *tmp, *bkp;

        list_for_each_safe(tmp, bkp, &entry->pclist) {
                
                pc = list_entry(tmp, plugin_container_t, int_list);

                entry->unsubscribe(pc);

                list_del(&pc->int_list);
                free(pc);
        }
}




/*
 * Initialize a new container for a plugin,
 * and add the container to the plugin entry.
 */
static plugin_container_t *create_container(plugin_entry_t *pe, plugin_generic_t *p) 
{
        plugin_container_t *pc;
        
        pc = calloc(1, sizeof(plugin_container_t));
        if ( ! pc ) {
                log(LOG_ERR, "memory exhausted.\n");
                return NULL;
        }

        pc->plugin = p;
        pc->parent = pe;
        
        list_add_tail(&pc->int_list, &pe->pclist);
        
        return pc;
}



/*
 * Copy an existing container (because the plugin will probably
 * linked in several part of the program, and that the container
 * contain not shared information).
 */
static plugin_container_t *copy_container(plugin_container_t *pc) 
{
        plugin_entry_t *pe = (plugin_entry_t *) pc->parent;
        plugin_container_t *new;
        
        new = malloc(sizeof(plugin_container_t));
        if ( ! new ) {
                log(LOG_ERR, "memory exhausted.\n");
                return NULL;
        }

        memcpy(new, pc, sizeof(plugin_container_t));
        new->already_used = 0;

        list_add_tail(&new->int_list, &pe->pclist);
        
        return new;
}




/*
 * Load a single plugin pointed to by 'filename'.
 */
static int plugin_load_single(const char *filename, int argc, char **argv,
                              int (*subscribe)(plugin_container_t *pc),
                              void (*unsubscribe)(plugin_container_t *pc)) 
{
        void *handle;
        plugin_entry_t *pe;
        plugin_generic_t *plugin;
        plugin_generic_t *(*init)(int argc, char **argv);
        
        handle = dlopen(filename, RTLD_NOW);
        if ( ! handle ) {
                log(LOG_INFO, "can't open %s : %s.\n", filename, dlerror());
                return -1;
        }

        init = dlsym(handle, "plugin_init");
        if ( ! init ) {
                log(LOG_INFO, "couldn't load %s : %s.\n", filename, dlerror());
                dlclose(handle);
                return -1;
        }
        
        pe = add_plugin_entry();
        if ( ! pe ) {
                dlclose(handle);
                return -1;
        }

        pe->handle = handle;
        pe->subscribe = subscribe;
        pe->unsubscribe = unsubscribe;
        
        plugin = init(argc, argv);
        if ( ! plugin ) {
                log(LOG_ERR, "plugin returned an error.\n");
                dlclose(handle);
                free(pe);
                return -1;
        }
        
        pe->plugin = plugin;

        return 0;
}




/**
 * plugin_load_from_dir:
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
int plugin_load_from_dir(const char *dirname, int argc, char **argv,
                         int (*subscribe)(plugin_container_t *p),
                         void (*unsubscribe)(plugin_container_t *pc)) 
{
        DIR *dir;
        struct dirent *d;
        int ret, count = 0;
        char filename[1024];
        
        dir = opendir(dirname);
        if ( ! dir ) {
                log(LOG_ERR, "couldn't open directory %s.\n", dirname);
                return -1;
        }
        
        while ( ( d = readdir(dir) ) ) {
                
                ret = is_a_plugin(d->d_name);
                if ( ret < 0 )
                        continue;
                
                snprintf(filename, sizeof(filename), "%s/%s", dirname, d->d_name);
                
                ret = plugin_load_single(filename, argc, argv, subscribe, unsubscribe);                
                if ( ret == 0 )
                        count++;
        }

        closedir(dir);

        return count;
}





/**
 * plugin_unsubscribe:
 * @plugin: Pointer to a plugin.
 *
 * Set @plugin to be inactive.
 *
 * The unsubscribe function specified in plugin_load_from_dir()
 * is called for plugin un-registration.
 *
 * Returns: 0 on success, -1 if an error occured.
 */
int plugin_unsubscribe(plugin_generic_t *plugin) 
{
        plugin_entry_t *pe;
        struct list_head *tmp;
        
        list_for_each(tmp, &all_plugin) {
                pe = list_entry(tmp, plugin_entry_t, list);

                if ( pe->plugin != plugin )
                        continue;

                delete_container(pe);                
                return 0;
        }

        log(LOG_ERR, "couldn't find plugin %p in plugin list.\n");
        return -1;
}




/**
 * plugin_subscribe
 * @plugin: Pointer to a plugin.
 *
 * Set @plugin to be active.
 *
 * The subscribe function specified in plugin_load_from_dir()
 * is called for plugin registration.
 *
 * Returns: 0 on success or -1 if an error occured.
 */
int plugin_subscribe(plugin_generic_t *plugin) 
{
        plugin_entry_t *pe;
        struct list_head *tmp;
        plugin_container_t *pc;
        
        list_for_each(tmp, &all_plugin) {
                pe = list_entry(tmp, plugin_entry_t, list);

                /*
                 * The plugin is subscribing itself
                 * before it's init() function returned.
                 */
                if ( pe->plugin == NULL ) 
                        pe->plugin = plugin;
                
                else if ( pe->plugin != plugin )
                        continue;
                
                pc = create_container(pe, plugin);
                if ( ! pc )
                        return -1;
                
                pe->subscribe(pc);

                return 0;
        }

        log(LOG_ERR, "couldn't find plugin %p in plugin list.\n");
        return -1;
}




/**
 * plugin_add:
 * @pc: Pointer to a plugin container
 * @h: Pointer to a linked list
 * @infos: Specific informations to associate with the container.
 *
 * This function add the plugin associated with @pc to the linked list
 * specified by @h. If this container is already used somewhere else,
 * a copy is made, because container doesn't share information).
 *
 * Returns: 0 on success or -1 if an error occured.
 */
int plugin_add(plugin_container_t *pc, struct list_head *h, const char *infos) 
{
        if ( pc->already_used++ && ! (pc = copy_container(pc)) ) {
                log(LOG_ERR, "couldn't duplicate plugin container.\n");
                return -1;
        }

        pc->infos = infos;
        list_add_tail(&pc->ext_list, h);

        return 0;
}




/**
 * plugin_del:
 * @pc: Pointer to a plugin container.
 *
 * Destroy @pc.
 */
void plugin_del(plugin_container_t *pc) 
{
        list_del(&pc->ext_list);
}




/**
 * plugin_print_stats:
 * @pc: The plugin container to print stats from
 *
 * Print plugin stats for *this* plugin (associated with the container).
 */
void plugin_print_stats(plugin_container_t *pc) 
{
        log(LOG_INFO, "%s (\"%s\") : plugin called %u time: %fs average\n",
            pc->plugin->name, pc->infos ? pc->infos : "no infos",
            pc->p_count, pc->p_time / pc->p_count);
}




/**
 * plugins_print_stats:
 *
 * Print available statistics for all plugins.
 */
void plugins_print_stats(void) 
{
        plugin_entry_t *pe;
        plugin_container_t *pc;
        struct list_head *tmp, *tmp2;

        log(LOG_INFO, "*** Plugin stats (not accurate if used > 2e32-1 times) ***\n\n");
        
        list_for_each(tmp, &all_plugin) {
                pe = list_entry(tmp, plugin_entry_t, list);
                
                list_for_each(tmp2, &pe->pclist) {
                        pc = list_entry(tmp2, plugin_container_t, int_list);
                        plugin_print_stats(pc);
                }
        }
}



int plugin_get_highest_id(void) 
{
        return plugins_id_max;
}



int plugin_request_new_id(void) 
{
        return plugins_id_max++;
}
