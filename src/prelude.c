/*****
*
* Copyright (C) 2004-2017 CS-SI. All Rights Reserved.
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
* You should have received a copy of the GNU General Public License along
* with this program; if not, write to the Free Software Foundation, Inc.,
* 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*
*****/

#include "config.h"
#include "libmissing.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <gnutls/gnutls.h>

#include "glthread/thread.h"
#include "glthread/lock.h"

#include "prelude.h"
#include "idmef-path.h"
#include "prelude-option.h"
#include "prelude-log.h"
#include "prelude-timer.h"
#include "variable.h"
#include "tls-auth.h"


int _prelude_internal_argc = 0;
char *_prelude_prefix = NULL;
char *_prelude_internal_argv[1024];

char _prelude_init_cwd[PATH_MAX];
static int libprelude_refcount = 0;
extern prelude_option_t *_prelude_generic_optlist;
extern gl_lock_t _criteria_parse_mutex;



static void tls_log_func(int level, const char *data)
{
        prelude_log(PRELUDE_LOG_INFO, "%s", data);
}


static void slice_arguments(int *argc, char **argv)
{
        int i;
        char *ptr;
        prelude_option_t *rootopt, *opt, *bkp = NULL;

        _prelude_client_register_options();

        _prelude_internal_argc = 0;
        _prelude_internal_argv[0] = NULL;

        if ( ! argc || ! argv || *argc < 1 )
                return;

        rootopt = _prelude_generic_optlist;
        _prelude_internal_argv[_prelude_internal_argc++] = argv[0];

        for ( i = 0; i < *argc && (size_t) _prelude_internal_argc + 1 < sizeof(_prelude_internal_argv) / sizeof(char *); i++ ) {

                ptr = argv[i];
                if ( *ptr != '-' )
                        continue;

                while ( *ptr == '-' ) ptr++;

                opt = prelude_option_search(rootopt, ptr, PRELUDE_OPTION_TYPE_CLI, FALSE);
                if ( ! opt ) {
                        if ( bkp )
                                rootopt = bkp;
                        continue;
                }

                if ( prelude_option_has_optlist(opt) ) {
                        rootopt = opt;
                        bkp = _prelude_generic_optlist;
                }

                _prelude_internal_argv[_prelude_internal_argc++] = argv[i];

                if ( (i + 1) == *argc )
                        break;

                if ( prelude_option_get_has_arg(opt) == PRELUDE_OPTION_ARGUMENT_NONE )
                        continue;

                if ( *argv[i + 1] == '-' )
                        continue;

                _prelude_internal_argv[_prelude_internal_argc++] = argv[i + 1];
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
        const char *env;

        if ( libprelude_refcount++ > 0 )
                return 0;

        env = getenv("LIBPRELUDE_DEBUG");
        if ( env )
                prelude_log_set_debug_level(atoi(env));

        env = getenv("LIBPRELUDE_TLS_DEBUG");
        if ( env ) {
                gnutls_global_set_log_level(atoi(env));
                gnutls_global_set_log_function(tls_log_func);
        }

        env = getenv("LIBPRELUDE_LOGFILE");
        if ( env )
                prelude_log_set_logfile(env);

        env = getenv("LIBPRELUDE_PREFIX");
        if ( env )
                _prelude_prefix = strdup(env);

        env = getenv("LIBPRELUDE_ABORT");
        if ( env ) {
                if ( *env )
                        _prelude_log_set_abort_level_from_string(env);
                else
                        _prelude_log_set_abort_level(PRELUDE_LOG_CRIT);
        }

        prelude_thread_init(NULL);

        if ( ! getcwd(_prelude_init_cwd, sizeof(_prelude_init_cwd)) )
                _prelude_init_cwd[0] = 0;

        ret = _prelude_timer_init();
        if ( ret < 0 )
                return ret;

        ret = glthread_atfork(prelude_fork_prepare, prelude_fork_parent, prelude_fork_child);
        if ( ret != 0 )
                return prelude_error_from_errno(ret);

        slice_arguments(argc, argv);

        ret = gnutls_global_init();
        if ( ret < 0 )
                return prelude_error_verbose(PRELUDE_ERROR_TLS,
                                             "TLS initialization failed: %s", gnutls_strerror(ret));

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

        if ( --libprelude_refcount != 0 )
                return;

        while ( (plugin = prelude_plugin_get_next(NULL, &iter)) )
                prelude_plugin_unload(plugin);

        _idmef_path_cache_destroy();
        prelude_option_destroy(NULL);
        variable_unset_all();

        tls_auth_deinit();
        gnutls_global_deinit();

        _prelude_thread_deinit();
}



static int levelstr_to_int(const char *wanted)
{
        int i;
        struct {
                int level;
                const char *level_str;
        } tbl[] = {
                { LIBPRELUDE_RELEASE_LEVEL_ALPHA, "alpha" },
                { LIBPRELUDE_RELEASE_LEVEL_BETA, "beta"   },
                { LIBPRELUDE_RELEASE_LEVEL_RC, "rc"       }
        };

        for ( i = 0; i < sizeof(tbl) / sizeof(*tbl); i++ ) {
                if ( strcmp(wanted, tbl[i].level_str) == 0 )
                        return tbl[i].level;
        }

        return -1;
}



/**
 * prelude_parse_version:
 * @version: A version string.
 * @out: Where to store the parsed version
 *
 * Parse version to an integer, and return it in @out.
 * Accepted format are:
 *
 * major.minor.micro.patchlevel
 * the following special level string are supported : alpha, beta, rc
 *
 * For example: 1.1.1rc1
 *
 * Returns: The 0 on success, a negative value in case of error.
 */
int prelude_parse_version(const char *version, unsigned int *out)
{
        int ret;
        char levels[6] = { 0 };
        int major = 0, minor = 0, micro = 0, level = 0, patch = 0;

        ret = sscanf(version, "%d.%d.%d%5[^0-9]%d", &major, &minor, &micro, levels, &patch);
        if ( ret <= 0 )
                return prelude_error_verbose(PRELUDE_ERROR_GENERIC, "version formatting error with '%s'", version);

        if ( *levels == 0 || *levels == '.' )
                level = LIBPRELUDE_RELEASE_LEVEL_FINAL;
        else {
                level = levelstr_to_int(levels);
                if ( level < 0 )
                        return level;
        }

        *out = (major << 24) | (minor << 16) | (micro << 8) | (level << 4) | (patch << 0);
        return 0;
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
        unsigned int hexversion;

        if ( ! req_version )
                return VERSION;

        ret = prelude_parse_version(req_version, &hexversion);
        if ( ret < 0 )
                return NULL;

        return (hexversion <= LIBPRELUDE_HEXVERSION) ? VERSION : NULL;
}



void prelude_fork_prepare(void)
{
#ifdef HAVE_PTHREAD_ATFORK
        return;
#endif

        _prelude_async_fork_prepare();
        _prelude_timer_fork_prepare();

        _idmef_path_cache_lock();
        gl_lock_lock(_criteria_parse_mutex);
}


void prelude_fork_parent(void)
{
#ifdef HAVE_PTHREAD_ATFORK
        return;
#endif

        _prelude_async_fork_parent();
        _prelude_timer_fork_parent();

        _idmef_path_cache_unlock();
        gl_lock_unlock(_criteria_parse_mutex);
}


void prelude_fork_child(void)
{
#ifdef HAVE_PTHREAD_ATFORK
        return;
#endif

        _prelude_async_fork_child();
        _prelude_timer_fork_child();

        _idmef_path_cache_reinit();
        gl_lock_init(_criteria_parse_mutex);
}
