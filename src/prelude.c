/*****
*
* Copyright (C) 2004,2005 PreludeIDS Technologies. All Rights Reserved.
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
#include <pthread.h>

#include "prelude.h"
#include "idmef-path.h"
#include "prelude-option.h"


int _prelude_internal_argc = 0;
char *_prelude_internal_argv[1024];
extern pthread_mutex_t _criteria_parse_mutex;
extern prelude_option_t *_prelude_generic_optlist;



static void prepare_fork_cb(void)
{
        _idmef_path_cache_lock();
        pthread_mutex_lock(&_criteria_parse_mutex);
}



static void in_fork_cb(void)
{
        _idmef_path_cache_unlock();
        pthread_mutex_unlock(&_criteria_parse_mutex);
}



static void remove_argv(int *argc, char **argv, int removed)
{
        int i;
        
        for ( i = removed; (i + 1) < *argc; i++ )                
                argv[i] = argv[i + 1];
        
        (*argc)--;
}




static void slice_arguments(int *argc, char **argv)
{
        int i;
        char *ptr;
        prelude_option_t *opt;
        
        _prelude_client_register_options();

        if ( ! argc || ! argv )
                return;
        
        _prelude_internal_argv[_prelude_internal_argc++] = argv[0];

        for ( i = 0; i < *argc && _prelude_internal_argc + 1 < sizeof(_prelude_internal_argv) / sizeof(char *); i++ ) {
                                
                ptr = argv[i];
                if ( *ptr != '-' )
                        continue;
                
                while ( *ptr == '-' ) ptr++;
                
                opt = prelude_option_search(_prelude_generic_optlist, ptr, PRELUDE_OPTION_TYPE_CLI, TRUE); 
                if ( ! opt )
                        continue;
                
                _prelude_internal_argv[_prelude_internal_argc++] = argv[i];
                remove_argv(argc, argv, i--);
                
                if ( (i + 1) == *argc )
                        break;
                
                if ( prelude_option_get_has_arg(opt) == PRELUDE_OPTION_ARGUMENT_NONE )
                        continue;
                
                if ( *argv[i + 1] == '-' )
                        continue;
                
                _prelude_internal_argv[_prelude_internal_argc++] = argv[i + 1];
                remove_argv(argc, argv, i + 1);
        }
}



/**
 * prelude_init:
 * @argc: Address of the argc parameter of your main() function.
 * @argv: Address of the argv parameter of your main() function.
 * 
 * Call this function before using any other Prelude functions in your applications.
 * It will initialize everything needed to operate the library and parses some standard
 * command line options. @argc and @argv are adjusted accordingly so your own code will
 * never see those standard arguments.
 *
 * Returns: 0 on success, a negative value if an error occured.
 */
int prelude_init(int *argc, char **argv)
{
        int ret;
        
        ret = pthread_atfork(prepare_fork_cb, in_fork_cb, in_fork_cb);
        if ( ret != 0 )
                return prelude_error_from_errno(ret);
        
        slice_arguments(argc, argv);

        return 0;
}



/**
 * prelude_deinit:
 *
 * Call this function if you're done using the library and want to free global
 * shared ressource allocated by libprelude.
 */
void prelude_deinit(void)
{
        prelude_list_t *iter = NULL;
        prelude_plugin_generic_t *plugin;
        
        while ( (plugin = prelude_plugin_get_next(&iter)) )
                prelude_plugin_unload(plugin);
        
        _idmef_path_cache_destroy();
        prelude_option_destroy(NULL);
        gnutls_global_deinit();
}



/**
 * prelude_check_version:
 * @req_version: The minimum acceptable version number.
 *
 * If @req_version is NULL this function will return the version of the library.
 * Otherwise, @req_version will be compared to the current library version. If
 * the library version is higher or equal, this function will return the current
 * library version. Otherwise, NULL is returned.
 *
 * Returns: The current library version, or NULL if @req_version doesn't match.
 */
const char *prelude_check_version(const char *req_version)
{
        int ret;
        int major, minor, micro;
        int rq_major, rq_minor, rq_micro;

        if ( ! req_version )
                return VERSION;
        
        ret = sscanf(VERSION, "%d.%d.%d", &major, &minor, &micro);
        if ( ret != 3 )
                return NULL;
        
        ret = sscanf(req_version, "%d.%d.%d", &rq_major, &rq_minor, &rq_micro);
        if ( ret != 3 )
                return NULL;
        
        if ( major > rq_major
             || (major == rq_major && minor > rq_minor)
             || (major == rq_major && minor == rq_minor && micro > rq_micro)
             || (major == rq_major && minor == rq_minor && micro == rq_micro) ) {
                return VERSION;
        }

        return NULL;
}
