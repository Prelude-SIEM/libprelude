/*****
*
* Copyright (C) 1998 - 2000 Yoann Vandoorselaere <yoann@mandrakesoft.com>
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
#include <assert.h>

#include "config.h"
#include "list.h"
#include "common.h"
#include "plugin-common.h"
#include "plugin-common-prv.h"
#include "compat.h"
#include "config-engine.h"

#ifndef RTLD_NOW
#define RTLD_NOW RTLD_LAZY
#endif


typedef struct {
        struct list_head list;
        void *handle;

        struct list_head pclist;
        plugin_generic_t *plugin;
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
static int (*register_for_use_cb)(plugin_container_t *pc);
static void *current_handle;
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
 * return a filename generated from 'dirname' & 'file'.
 */
static char *generate_filename(const char *dirname, const char *file) 
{
        char *filename;
        int len = strlen(dirname) + strlen(file) + 2;

        filename = malloc(len);
        if ( ! filename ) {
                log(LOG_ERR, "memory exhausted.\n");
                return NULL;
        }
        
        snprintf(filename, len, "%s/%s", dirname, file);

        return filename;
}



/*
 *
 */
static plugin_entry_t *add_plugin_entry(void *handle, plugin_generic_t *p) 
{
        plugin_entry_t *pe;
        
        pe = malloc(sizeof(plugin_entry_t));
        if ( ! pe ) {
                log(LOG_ERR, "memory exhausted.\n");
                return NULL;
        }
        
        pe->handle = handle;
        pe->plugin = p;
        
        INIT_LIST_HEAD(&pe->pclist);
        list_add_tail(&pe->list, &all_plugin);
        
        return pe;
}



/*
 *
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
 *
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
static int plugin_load_single(const char *filename) 
{
        void *handle;
        int (*init)(int id);
        
        current_handle = handle = dlopen(filename, RTLD_NOW);
        if ( ! handle ) {
                log(LOG_ERR, "can't open %s : %s.\n", filename, dlerror());
                return -1;
        }

        init = dlsym(handle, "plugin_init");
        if ( ! init ) {
                log(LOG_ERR, "couldn't load %s : %s.\n", filename, dlerror());
                return -1;
        }

        return init(plugins_id_max);
}



/**
 * plugin_load_from_dir:
 * @dirname: The directory to load the plugin from
 * @cb: A callback to be called each time a plugin register
 *
 * Load all plugins in directory 'dirname'.
 * The CB arguments will be called for each plugin that register
 * (using the plugin_register function), then the application will
 * have the ability to use plugin_register_for_use to tell it want
 * to use this plugin.
 *
 * Returns: 0 on success, -1 on error.
 */
int plugin_load_from_dir(const char *dirname, int (*cb)(plugin_container_t *p)) 
{
        int ret;
        DIR *dir;
        char *filename;
        struct dirent *d;
        
        dir = opendir(dirname);
        if ( ! dir ) {
                log(LOG_ERR, "couldn't open directory %s.\n", dirname);
                return -1;
        }

        register_for_use_cb = cb;
        
        while ( ( d = readdir(dir) ) ) {
                filename = generate_filename(dirname, d->d_name);
                
                ret = is_a_plugin(filename);
                if ( ret == 0 )
                        plugin_load_single(filename);

                free(filename);
        }

        closedir(dir);

        return 0;
}



/**
 * plugin_register:
 * @plugin: A plugin to register
 *
 * Used by the plugin at initialisation time to register itself.
 *
 * Returns: 0 on success, -1 on error.
 */
int plugin_register(plugin_generic_t *plugin) 
{
       int ret;
        plugin_entry_t *pe;
        plugin_container_t *pc;

        if ( ! register_for_use_cb )
                return 0;
        
        pe = add_plugin_entry(current_handle, plugin);
        if ( ! pe )
                return -1;
        
        pc = create_container(pe, plugin);
        if ( ! pc )
                return -1;
        
        ret = register_for_use_cb(pc);
        if ( ret < 0 )
                return -1;

        plugins_id_max++;

        return 0;
}



/**
 * plugin_register_for_use:
 * @pc: The container of the plugin to be used
 * @h: A list where the @pc need to be added
 * @infos: Additionnal informations about the plugin
 *
 * To be called by application that wish to use the plugin
 * associated with 'pc'.
 *
 * Returns: 0 on success, -1 on error.
 */
int plugin_register_for_use(plugin_container_t *pc, struct list_head *h, const char *infos)
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
 * plugin_print_stats:
 * @pc: The plugin container to print stats from
 *
 * Print plugin stats for *this* plugin (associated with the container).
 */
void plugin_print_stats(plugin_container_t *pc) 
{
        log(LOG_INFO, "%s ", pc->plugin->name);
        
        if ( pc->infos )
                log(LOG_INFO, "(infos=%s) :\n", pc->infos);
        else
                log(LOG_INFO, ":\n");

        log(LOG_INFO, "\t\t- plugin: called %ld time : %fs average\n",
            pc->p_count, pc->p_time / pc->p_count);
}



/**
 * plugins_print_stats:
 *
 * Print availlable statistics for all plugins.
 */
void plugins_print_stats(void) 
{
        plugin_container_t *pc;
        plugin_entry_t *pe;
        struct list_head *tmp, *tmp2;
        
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



/*
 *
 */

static char **argv = NULL;
static int argc;
static int s_optind;
static int help_flag = 0;


/*
 * Search plugin argument.
 * If found, set argv to start at first plugin argument,
 * and argc to count the exact number of args for this plugins.
 */  
static void get_plugin_opts(char *pname, int *pargc, char ***pargv) 
{
        int i, j;
        
        for ( i = s_optind; (i + 1) < argc; i++ ) {
                /*
                 * Start of a plugin option...
                 */
                if ( ! strstr(argv[i], "-m") && ! strstr(argv[i], "-plugin") )
                        continue;

                /*
                 * Is it our plugin (pname) ?
                 */
                if ( strcmp(argv[i + 1], pname) != 0  )
                        continue;
                
                *pargc = 0;

                /*
                 * Count argc for theses plugin option.
                 */
                for (j = (i + 1); j < argc; j++ ) {
                        if ( strstr(argv[j], "-m") || strstr(argv[j], "-plugin") )
                                break;
                        else
                                *pargc += 1;
                }
                        
                *pargv = &argv[i + 1];
                break;
        }
}



/*
 * Get plugin related options (option behind -m pname).
 */
static void plugin_get_opts(char *pname, int *pargc, char ***pargv) 
{
        static char *null_argv[3] = { 0 };
        
        optind = 0;
        *pargc = 1;
        *pargv = null_argv;
        null_argv[0] = pname;
        
        if ( ! help_flag )
                get_plugin_opts(pname, pargc, pargv);
        else {
                *pargc = 2;
                null_argv[0] = pname;
                null_argv[1] = "-h";
                *pargv = null_argv;
                optind = 0;
        }
}



/*
 * Try to get all option that were not set from the command line in the config file.
 */
static void get_missing_options(const char *filename, plugin_generic_t *plugin, plugin_option_t *opts) 
{
        int i;
        const char *str;
        config_t *cfg = NULL;
        
        for ( i = 0; opts[i].name != NULL; i++ ) {
            
                if ( opts[i].called )
                        continue;
                
                if ( ! cfg && ! (cfg = config_open(filename)) ) {
                        log(LOG_ERR, "couldn't open %s.\n", filename);
                        return;
                }
                
                str = config_get(cfg, plugin_name(plugin), opts[i].name);
                if ( str ) 
                        opts[i].cb(str);
        }

        if ( cfg )
                config_close(cfg);
}



/*
 * Generate a getopt compatible option string from
 * a plugin_option_t array.
 */
static const char *generate_options_string(plugin_option_t *opts) 
{
        int i, ok = 0;
        static char buf[1024];

        for ( i = 0; opts[i].name != NULL; i++ ) {
            
                if ( ok == (sizeof(buf) - 2) ) {
                        log(LOG_ERR, "Buffer too short.\n");
                        return NULL;
                }
                
                buf[ok++] = opts[i].val;
                if ( opts[i].has_arg == optional_argument || opts[i].has_arg == required_argument )
                        buf[ok++] = ':';
                
        }

        buf[ok] = '\0';

        return buf;
}



/**
 * plugin_config_get:
 * @plugin: The plugin that want to read it's configuration.
 * @cfg: List of options supported by the plugin.
 * @conffile: Config file to read for value not specified on the command line.
 *
 * Request the configuration for this plugin to be read.
 * For each option found, the corresponding callback int he #cfg structure is
 * called back.
 *
 * Returns: -1 on error, 0 on success.
 */
void plugin_config_get(plugin_generic_t *plugin, plugin_option_t *cfg, const char *conffile) 
{
        char **argv;
        int c, i, argc;
        const char *optstring;
        
        plugin_get_opts(plugin_name(plugin), &argc, &argv);

        optstring = generate_options_string(cfg);
        if ( ! optstring )
                return;
        
        while ( (c = getopt_long(argc, argv, optstring, (struct option *)cfg, NULL)) != -1 ) {
                
                for ( i = 0; cfg[i].name != NULL; i++ )
                        if ( cfg[i].val == c ) {

                                cfg[i].cb(optarg);
                                cfg[i].called = 1;
                                break;
                        }
        }
        
        get_missing_options(conffile, plugin, cfg);
}




/**
 * plugins_print_opts:
 * @dirname: The directory to load plugin from.
 *
 * Print availlable options for all plugins.
 */
void plugins_print_opts(const char *dirname) 
{
        plugin_load_from_dir(dirname, NULL);
}



/**
 * plugin_set_args:
 * @ac: argc where plugin configuration start.
 * @av: argv where plugin configuration start.
 *
 * Should be called by the configuration module (the one that parse
 * argc and argv in order to set the beginning of the plugin
 * configuration string).
 */
void plugin_set_args(int ac, char **av) 
{        
        if ( (ac == 0 && av == NULL) || strstr(av[optind - 1], "-h") )
                help_flag = 1;        
        
        argc = ac;
        argv = av;
        s_optind = optind - 2;
}


