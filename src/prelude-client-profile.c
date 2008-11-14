/*****
*
* Copyright (C) 2004-2005,2006,2007 PreludeIDS Technologies. All Rights Reserved.
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
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>

#include <gnutls/gnutls.h>

#include "relocatable.h"
#include "glthread/lock.h"

#define PRELUDE_ERROR_SOURCE_DEFAULT PRELUDE_ERROR_SOURCE_CLIENT_PROFILE

#include "prelude-error.h"
#include "prelude-client-profile.h"
#include "tls-auth.h"
#include "common.h"

#if (defined _WIN32 || defined __WIN32__) && !defined __CYGWIN__
# define geteuid(x) (0)
# define getegid(x) (0)
#endif


#define PRELUDE_PROFILE_DIR PRELUDE_CONFIG_DIR "/profile"
#define PRELUDE_CONFIG_DEFAULT_DIR PRELUDE_CONFIG_DIR "/default"
#define TLS_CONFIG PRELUDE_CONFIG_DEFAULT_DIR "/tls.conf"


/*
 * directory where TLS private keys file are stored.
 */
#define TLS_KEY_DIR PRELUDE_CONFIG_DIR "/keys"

/*
 * directory where TLS client certificate file are stored.
 */
#define TLS_CLIENT_CERT_DIR PRELUDE_CONFIG_DIR "/tls/client"

/*
 * directory where TLS server certificate file are stored.
 */
#define TLS_SERVER_CERT_DIR PRELUDE_CONFIG_DIR "/tls/server"



struct prelude_client_profile {
        int refcount;
        prelude_uid_t uid;
        prelude_gid_t gid;
        char *name;
        uint64_t analyzerid;
        gnutls_certificate_credentials credentials;
};


extern char *_prelude_prefix;
static char *user_prefix = NULL;
static const char *relocated_prefix;
static const char *relative_spool_dir = NULL;
static const char *relative_config_default_dir = NULL;
static const char *relative_profile_dir = NULL;

static gl_lock_t lock = gl_lock_initializer;
gl_once_define(static, relocate_once);



static const char *get_relpath(const char *path)
{
        return ( strstr(path, INSTALLPREFIX) ) ? path + sizeof(INSTALLPREFIX) : NULL;
}



static void _get_dir_once(void)
{
        relocated_prefix = (_prelude_prefix) ? _prelude_prefix : relocate(INSTALLPREFIX);

        relative_spool_dir = get_relpath(PRELUDE_SPOOL_DIR);
        relative_profile_dir = get_relpath(PRELUDE_PROFILE_DIR);
        relative_config_default_dir = get_relpath(PRELUDE_CONFIG_DEFAULT_DIR);

        prelude_log_debug(2, "install   prefix=%s", INSTALLPREFIX);
        prelude_log_debug(2, "relocated prefix=%s\n", relocated_prefix);
        prelude_log_debug(2, "relative   spool=%s\n", relative_spool_dir ? relative_spool_dir : PRELUDE_SPOOL_DIR);
        prelude_log_debug(2, "relative  config=%s\n", relative_config_default_dir ? relative_config_default_dir : PRELUDE_CONFIG_DEFAULT_DIR);
        prelude_log_debug(2, "relative profile=%s\n", relative_profile_dir ? relative_profile_dir : PRELUDE_PROFILE_DIR);
}


static const char *init_once_and_get_prefix(void)
{
        gl_once(relocate_once, _get_dir_once);
        return (user_prefix) ? user_prefix : relocated_prefix;
}


static int get_profile_analyzerid(prelude_client_profile_t *cp)
{
        int ret;
        FILE *fd;
        char *ptr, filename[256], buf[256];

        prelude_client_profile_get_profile_dirname(cp, filename, sizeof(filename));
        if ( access(filename, R_OK|X_OK) < 0 ) {
                if ( errno == ENOENT )
                        return prelude_error_verbose(PRELUDE_ERROR_PROFILE, "profile '%s' does not exist", cp->name);

                else if ( errno == EACCES )
                        return prelude_error_verbose(PRELUDE_ERROR_PROFILE, "could not open profile '%s': insufficient permission", cp->name);
        }

        prelude_client_profile_get_analyzerid_filename(cp, filename, sizeof(filename));

        fd = fopen(filename, "r");
        if ( ! fd )
                return prelude_error_verbose(PRELUDE_ERROR_PROFILE, "could not open '%s' for reading", filename);

        ptr = fgets(buf, sizeof(buf), fd);
        fclose(fd);

        if ( ! ptr )
                return prelude_error_verbose(PRELUDE_ERROR_PROFILE, "could not read analyzerID from '%s'", filename);

        ret = sscanf(buf, "%" PRELUDE_PRIu64, &cp->analyzerid);
        if ( ret != 1 )
                return prelude_error_verbose(PRELUDE_ERROR_PROFILE, "'%s' is not a valid analyzerID", buf);

        return 0;
}


/**
 * prelude_client_profile_set_prefix:
 * @cp: pointer on a #prelude_client_profile_t object.
 * @prefix: Prefix to use for various libprelude files.
 *
 * This function allow to dynamically change the prefix used to acess
 * libprelude related file. This is particularly usefull in case of
 * application running under certain condition (chroot).
 *
 * Returns: 0 on success, a negative value if an error occured.
 */
int prelude_client_profile_set_prefix(prelude_client_profile_t *cp, const char *prefix)
{
        char *n;

        n = strdup(prefix);

        gl_lock_lock(lock);

        if ( user_prefix )
                free(user_prefix);

        user_prefix = n;

        gl_lock_unlock(lock);

        return (n) ? 0 : prelude_error_from_errno(errno);
}


/**
 * prelude_client_profile_get_prefix:
 * @cp: pointer on a #prelude_client_profile_t object.
 * @buf: buffer to write the returned filename to.
 * @size: size of @buf.
 *
 * Retrieve current prefix used with this profile.
 */
void prelude_client_profile_get_prefix(const prelude_client_profile_t *cp, char *buf, size_t size)
{
        const char *prefix;

        prelude_return_if_fail(buf);

        gl_lock_lock(lock);

        prefix = init_once_and_get_prefix();
        snprintf(buf, size, "%s", prefix);

        gl_lock_unlock(lock);
}



/**
 * prelude_client_profile_get_analyzerid_filename:
 * @cp: pointer on a #prelude_client_profile_t object.
 * @buf: buffer to write the returned filename to.
 * @size: size of @buf.
 *
 * Writes the filename used to store @cp unique and permanent analyzer ident.
 */
void prelude_client_profile_get_default_config_dirname(const prelude_client_profile_t *cp, char *buf, size_t size)
{
        const char *prefix;

        prelude_return_if_fail(buf);

        gl_lock_lock(lock);

        prefix = init_once_and_get_prefix();
        if ( ! relative_config_default_dir )
                snprintf(buf, size, "%s", PRELUDE_CONFIG_DEFAULT_DIR);
        else
                snprintf(buf, size, "%s/%s", prefix, relative_config_default_dir);

        gl_lock_unlock(lock);
}



/**
 * prelude_client_profile_get_analyzerid_filename:
 * @cp: pointer on a #prelude_client_profile_t object.
 * @buf: buffer to write the returned filename to.
 * @size: size of @buf.
 *
 * Writes the filename used to store @cp unique and permanent analyzer ident.
 */
void prelude_client_profile_get_analyzerid_filename(const prelude_client_profile_t *cp, char *buf, size_t size)
{
        const char *prefix;

        prelude_return_if_fail(cp);
        prelude_return_if_fail(buf);

        gl_lock_lock(lock);

        prefix = init_once_and_get_prefix();
        if ( ! relative_profile_dir )
                snprintf(buf, size, "%s/%s/analyzerid", PRELUDE_PROFILE_DIR, cp->name);
        else
                snprintf(buf, size, "%s/%s/%s/analyzerid", prefix, relative_profile_dir, cp->name);

        gl_lock_unlock(lock);
}



/**
 * prelude_client_profile_get_config_filename:
 * @cp: pointer on a #prelude_client_profile_t object.
 * @buf: buffer to write the returned filename to.
 * @size: size of @buf.
 *
 * Writes the filename used to store @cp configuration template.
 */
void prelude_client_profile_get_config_filename(const prelude_client_profile_t *cp, char *buf, size_t size)
{
        const char *prefix;

        prelude_return_if_fail(cp);
        prelude_return_if_fail(buf);

        gl_lock_lock(lock);

        prefix = init_once_and_get_prefix();
        if ( ! relative_profile_dir )
                snprintf(buf, size, "%s/%s/config", PRELUDE_PROFILE_DIR, cp->name);
        else
                snprintf(buf, size, "%s/%s/%s/config", prefix, relative_profile_dir, cp->name);

        gl_lock_unlock(lock);
}


/**
 * prelude_client_profile_get_tls_key_filename:
 * @cp: pointer on a #prelude_client_profile_t object.
 * @buf: buffer to write the returned filename to.
 * @size: size of @buf.
 *
 * Writes the filename used to store @cp private key.
 */
void prelude_client_profile_get_tls_key_filename(const prelude_client_profile_t *cp, char *buf, size_t size)
{
        const char *prefix;

        prelude_return_if_fail(cp);
        prelude_return_if_fail(buf);

        gl_lock_lock(lock);

        prefix = init_once_and_get_prefix();
        if ( ! relative_profile_dir )
                snprintf(buf, size, "%s/%s/key", PRELUDE_PROFILE_DIR, cp->name);
        else
                snprintf(buf, size, "%s/%s/%s/key", prefix, relative_profile_dir, cp->name);

        gl_lock_unlock(lock);
}



/**
 * prelude_client_profile_get_tls_server_ca_cert_filename:
 * @cp: pointer on a #prelude_client_profile_t object.
 * @buf: buffer to write the returned filename to.
 * @size: size of @buf.
 *
 * Writes the filename used to store @cp related CA certificate.
 * This only apply to @cp receiving connection from analyzer (server).
 */
void prelude_client_profile_get_tls_server_ca_cert_filename(const prelude_client_profile_t *cp, char *buf, size_t size)
{
        const char *prefix;

        prelude_return_if_fail(cp);
        prelude_return_if_fail(buf);

        gl_lock_lock(lock);

        prefix = init_once_and_get_prefix();
        if ( ! relative_profile_dir )
                snprintf(buf, size, "%s/%s/server.ca", PRELUDE_PROFILE_DIR, cp->name);
        else
                snprintf(buf, size, "%s/%s/%s/server.ca", prefix, relative_profile_dir, cp->name);

        gl_lock_unlock(lock);
}



/**
 * prelude_client_profile_get_tls_server_keycert_filename:
 * @cp: pointer on a #prelude_client_profile_t object.
 * @buf: buffer to write the returned filename to.
 * @size: size of @buf.
 *
 * Writes the filename used to store certificate for @cp server.
 * This only apply to @cp receiving connection from analyzer (server).
 */
void prelude_client_profile_get_tls_server_keycert_filename(const prelude_client_profile_t *cp, char *buf, size_t size)
{
        const char *prefix;

        prelude_return_if_fail(cp);
        prelude_return_if_fail(buf);

        gl_lock_lock(lock);

        prefix = init_once_and_get_prefix();
        if ( ! relative_profile_dir )
                snprintf(buf, size, "%s/%s/server.keycrt", PRELUDE_PROFILE_DIR, cp->name);
        else
                snprintf(buf, size, "%s/%s/%s/server.keycrt", prefix, relative_profile_dir, cp->name);

        gl_lock_unlock(lock);
}



/**
 * prelude_client_profile_get_tls_server_crl_filename:
 * @cp: pointer on a #prelude_client_profile_t object.
 * @buf: buffer to write the returned filename to.
 * @size: size of @buf.
 *
 * Writes the filename used to store CRL for @cp server.
 * This only apply to @cp receiving connection from analyzer (server).
 */
void prelude_client_profile_get_tls_server_crl_filename(const prelude_client_profile_t *cp, char *buf, size_t size)
{
        const char *prefix;

        prelude_return_if_fail(cp);
        prelude_return_if_fail(buf);

        gl_lock_lock(lock);

        prefix = init_once_and_get_prefix();
        if ( ! relative_profile_dir )
                snprintf(buf, size, "%s/%s/server.crl", PRELUDE_PROFILE_DIR, cp->name);
        else
                snprintf(buf, size, "%s/%s/%s/server.crl", prefix, relative_profile_dir, cp->name);

        gl_lock_unlock(lock);
}



/**
 * prelude_client_profile_get_tls_client_trusted_cert_filename:
 * @cp: pointer on a #prelude_client_profile_t object.
 * @buf: buffer to write the returned filename to.
 * @size: size of @buf.
 *
 * Writes the filename used to store peers public certificates that @cp trust.
 * This only apply to client connecting to a peer.
 */
void prelude_client_profile_get_tls_client_trusted_cert_filename(const prelude_client_profile_t *cp, char *buf, size_t size)
{
        const char *prefix;

        prelude_return_if_fail(cp);
        prelude_return_if_fail(buf);

        gl_lock_lock(lock);

        prefix = init_once_and_get_prefix();
        if ( ! relative_profile_dir )
                snprintf(buf, size, "%s/%s/client.trusted", PRELUDE_PROFILE_DIR, cp->name);
        else
                snprintf(buf, size, "%s/%s/%s/client.trusted", prefix, relative_profile_dir, cp->name);

        gl_lock_unlock(lock);
}




/**
 * prelude_client_profile_get_tls_client_keycert_filename:
 * @cp: pointer on a #prelude_client_profile_t object.
 * @buf: buffer to write the returned filename to.
 * @size: size of @buf.
 *
 * Writes the filename used to store public certificate for @cp private key.
 * This only apply to client connecting to a peer.
 */
void prelude_client_profile_get_tls_client_keycert_filename(const prelude_client_profile_t *cp, char *buf, size_t size)
{
        const char *prefix;

        prelude_return_if_fail(cp);
        prelude_return_if_fail(buf);

        gl_lock_lock(lock);

        prefix = init_once_and_get_prefix();
        if ( ! relative_profile_dir )
                snprintf(buf, size, "%s/%s/client.keycrt", PRELUDE_PROFILE_DIR, cp->name);
        else
                snprintf(buf, size, "%s/%s/%s/client.keycrt", prefix, relative_profile_dir, cp->name);

        gl_lock_unlock(lock);
}



/**
 * prelude_client_profile_get_backup_dirname:
 * @cp: pointer on a #prelude_client_profile_t object.
 * @buf: buffer to write the returned filename to.
 * @size: size of @buf.
 *
 * Writes the directory name where message sent by @cp will be stored,
 * in case writing the message to the peer fail.
 */
void prelude_client_profile_get_backup_dirname(const prelude_client_profile_t *cp, char *buf, size_t size)
{
        const char *prefix;

        prelude_return_if_fail(cp);
        prelude_return_if_fail(buf);

        gl_lock_lock(lock);

        prefix = init_once_and_get_prefix();
        if ( ! relative_spool_dir )
                snprintf(buf, size, "%s/%s", PRELUDE_SPOOL_DIR, cp->name);
        else
                snprintf(buf, size, "%s/%s/%s", prefix, relative_spool_dir, cp->name);

        gl_lock_unlock(lock);
}


/**
 * prelude_client_profile_get_backup_dirname:
 * @cp: pointer on a #prelude_client_profile_t object.
 * @buf: buffer to write the returned filename to.
 * @size: size of @buf.
 *
 * Writes the directory name where the profile for @cp is stored. If
 * @cp is NULL or has no name, then this function will provide the main
 * profile directory.
 */
void prelude_client_profile_get_profile_dirname(const prelude_client_profile_t *cp, char *buf, size_t size)
{
        const char *prefix, *name_sep = "", *name = "";

        prelude_return_if_fail(buf);

        if ( cp && cp->name ) {
                name_sep = "/";
                name = cp->name;
        }

        gl_lock_lock(lock);

        prefix = init_once_and_get_prefix();
        if ( ! relative_profile_dir )
                snprintf(buf, size, "%s/%s%s", PRELUDE_PROFILE_DIR, name_sep, name);
        else
                snprintf(buf, size, "%s/%s%s%s", prefix, relative_profile_dir, name_sep, name);

        gl_lock_unlock(lock);
}


int _prelude_client_profile_new(prelude_client_profile_t **ret)
{
        *ret = calloc(1, sizeof(**ret));
        if ( ! *ret )
                return prelude_error_from_errno(errno);

        (*ret)->refcount = 1;
        (*ret)->uid = geteuid();
        (*ret)->gid = getegid();

        return 0;
}



int _prelude_client_profile_init(prelude_client_profile_t *cp)
{
        int ret;

        prelude_return_val_if_fail(cp, prelude_error(PRELUDE_ERROR_ASSERTION));

        ret = get_profile_analyzerid(cp);
        if ( ret < 0 )
                return ret;

        return 0;
}



/**
 * prelude_client_profile_new:
 * @ret: Pointer where to store the address of the created object.
 * @name: Name for this profile.
 *
 * Creates a new #prelude_client_profile_t object and store its
 * address into @ret.
 *
 * Returns: 0 on success or a negative value if an error occured.
 */
int prelude_client_profile_new(prelude_client_profile_t **ret, const char *name)
{
        int retval;
        prelude_client_profile_t *cp;

        prelude_return_val_if_fail(name, prelude_error(PRELUDE_ERROR_ASSERTION));

        retval = _prelude_client_profile_new(&cp);
        if ( retval < 0 )
                return retval;

        cp->name = strdup(name);
        if ( ! cp->name ) {
                free(cp);
                return prelude_error_from_errno(errno);
        }

        retval = _prelude_client_profile_init(cp);
        if ( retval < 0 )
                return retval;

        *ret = cp;

        return 0;
}



/**
 * prelude_client_profile_destroy:
 * @cp: Pointer to a #prelude_client_profile_t.
 *
 * Destroys @cp.
 */
void prelude_client_profile_destroy(prelude_client_profile_t *cp)
{
        prelude_return_if_fail(cp);

        if ( --cp->refcount )
                return;

        if ( cp->credentials )
                gnutls_certificate_free_credentials(cp->credentials);

        if ( cp->name )
                free(cp->name);

        free(cp);
}



/**
 * prelude_client_profile_get_uid:
 * @cp: Pointer to a #prelude_client_profile_t object.
 *
 * Gets the UID associated with @cp.
 *
 * Returns: the UID associated used by @cp.
 */
prelude_uid_t prelude_client_profile_get_uid(const prelude_client_profile_t *cp)
{
        prelude_return_val_if_fail(cp, 0);
        return cp->uid;
}



/**
 * prelude_client_profile_set_uid:
 * @cp: Pointer to a #prelude_client_profile_t object.
 * @uid: UID to be used by @cp.
 *
 * Sets the UID used by @cp to @uid.
 */
void prelude_client_profile_set_uid(prelude_client_profile_t *cp, prelude_uid_t uid)
{
        prelude_return_if_fail(cp);
        cp->uid = uid;
}



/**
 * prelude_client_profile_get_gid:
 * @cp: Pointer to a #prelude_client_profile_t object.
 *
 * Gets the GID associated with @cp.
 *
 * Returns: the GID associated used by @cp.
 */
prelude_gid_t prelude_client_profile_get_gid(const prelude_client_profile_t *cp)
{
        prelude_return_val_if_fail(cp, 0);
        return cp->gid;
}



/**
 * prelude_client_profile_set_gid:
 * @cp: Pointer to a #prelude_client_profile_t object.
 * @gid: GID to be used by @cp.
 *
 * Sets the GID used by @cp to @gid.
 */
void prelude_client_profile_set_gid(prelude_client_profile_t *cp, prelude_gid_t gid)
{
        prelude_return_if_fail(cp);
        cp->gid = gid;
}



/**
 * prelude_client_profile_set_analyzerid:
 * @cp: Pointer to a #prelude_client_profile_t object.
 * @analyzerid: Analyzer ID to be used by @cp.
 *
 * Sets the Analyzer ID used by @cp to @analyzerid.
 */
void prelude_client_profile_set_analyzerid(prelude_client_profile_t *cp, uint64_t analyzerid)
{
        prelude_return_if_fail(cp);
        cp->analyzerid = analyzerid;
}



/**
 * prelude_client_profile_get_analyzerid:
 * @cp: Pointer to a #prelude_client_profile_t object.
 *
 * Gets the unique and permanent analyzer ident associated with @cp.
 *
 * Returns: the analyzer ident used by @cp.
 */
uint64_t prelude_client_profile_get_analyzerid(const prelude_client_profile_t *cp)
{
        prelude_return_val_if_fail(cp, 0);
        return cp->analyzerid;
}



/**
 * prelude_client_profile_get_name:
 * @cp: Pointer to a #prelude_client_profile_t object.
 *
 * Gets the name of @cp client profile.
 *
 * Returns: the name used by @cp.
 */
const char *prelude_client_profile_get_name(const prelude_client_profile_t *cp)
{
        prelude_return_val_if_fail(cp, NULL);
        return cp->name;
}


/**
 * prelude_client_profile_set_name:
 * @cp: Pointer to a #prelude_client_profile_t object.
 * @name: Name to associate the profile with.
 *
 * Sets the prelude client profile name.
 *
 * Returns: 0 on success or a negative value if an error occured.
 */
int prelude_client_profile_set_name(prelude_client_profile_t *cp, const char *name)
{
        prelude_return_val_if_fail(cp, prelude_error(PRELUDE_ERROR_ASSERTION));
        prelude_return_val_if_fail(name, prelude_error(PRELUDE_ERROR_ASSERTION));

        if ( cp->name )
                free(cp->name);

        cp->name = strdup(name);
        if ( ! cp->name )
                return prelude_error_from_errno(errno);

        return 0;
}



/**
 * prelude_client_profile_get_crendentials:
 * @cp: Pointer to a #prelude_client_profile_t object.
 * @credentials: The GNU TLS certificate credentials retrieved.
 *
 * Gets the prelude client profile credentials
 *
 * Returns: 0 on success or a negative value if an error occured.
 */
int prelude_client_profile_get_credentials(prelude_client_profile_t *cp, void **credentials)
{
        int ret;

        prelude_return_val_if_fail(cp, prelude_error(PRELUDE_ERROR_ASSERTION));

        if ( cp->credentials ) {
                *credentials = cp->credentials;
                return 0;
        }

        ret = tls_auth_init(cp, &cp->credentials);
        if ( ret < 0 )
                return ret;

        *credentials = cp->credentials;

        return 0;
}


prelude_client_profile_t *prelude_client_profile_ref(prelude_client_profile_t *cp)
{
        prelude_return_val_if_fail(cp, NULL);

        cp->refcount++;
        return cp;
}
