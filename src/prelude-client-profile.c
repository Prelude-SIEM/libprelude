/*****
*
* Copyright (C) 2004, 2005 PreludeIDS Technologies. All Rights Reserved.
* Author: Yoann Vandoorselaere <yoann.v@prelude-ids.com>
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
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>

#include <gnutls/gnutls.h>

#define PRELUDE_ERROR_SOURCE_DEFAULT PRELUDE_ERROR_SOURCE_CLIENT_PROFILE

#include "prelude-error.h"
#include "prelude-client-profile.h"
#include "tls-auth.h"

/*
 * directory where TLS private keys file are stored.
 */
#define TLS_KEY_DIR PRELUDE_CONFIG_DIR "/tls/keys"

/*
 * directory where TLS client certificate file are stored.
 */
#define TLS_CLIENT_CERT_DIR PRELUDE_CONFIG_DIR "/tls/client"

/*
 * directory where TLS server certificate file are stored.
 */
#define TLS_SERVER_CERT_DIR PRELUDE_CONFIG_DIR "/tls/server"


/*
 * directory where analyzerID file are stored.
 */
#define IDENT_DIR PRELUDE_CONFIG_DIR "/analyzerid"



struct prelude_client_profile {
        uid_t uid;
        gid_t gid;
        char *name;
        uint64_t analyzerid;
        gnutls_certificate_credentials credentials;
};



static int get_profile_analyzerid(prelude_client_profile_t *cp) 
{
        int ret;
        FILE *fd;
        char *ptr, filename[256], buf[256];

        prelude_client_profile_get_analyzerid_filename(cp, filename, sizeof(filename));
        
        fd = fopen(filename, "r");
        if ( ! fd )
                return prelude_error(PRELUDE_ERROR_ANALYZERID_FILE);

        ptr = fgets(buf, sizeof(buf), fd);
        fclose(fd);
        
        if ( ! ptr )
                return prelude_error(PRELUDE_ERROR_ANALYZERID_PARSE);
        
        ret = sscanf(buf, "%" PRIu64, &cp->analyzerid);
        if ( ret != 1 )
                return prelude_error(PRELUDE_ERROR_ANALYZERID_PARSE);
        
        return 0;
}



/**
 * prelude_client_profile_get_analyzerid_filename:
 * @cp: pointer on a #prelude_client_profile_t object.
 * @buf: buffer to write the returned filename to.
 * @size: size of @buf.
 *
 * Writes the filename used to store @cp unique and permanent analyzer ident.
 */
void prelude_client_profile_get_analyzerid_filename(prelude_client_profile_t *cp, char *buf, size_t size) 
{
        snprintf(buf, size, IDENT_DIR "/%s", cp->name);
}



/**
 * prelude_client_profile_get_tls_key_filename:
 * @cp: pointer on a #prelude_client_profile_t object.
 * @buf: buffer to write the returned filename to.
 * @size: size of @buf.
 *
 * Writes the filename used to store @cp private key.
 */
void prelude_client_profile_get_tls_key_filename(prelude_client_profile_t *cp, char *buf, size_t size) 
{
        snprintf(buf, size, TLS_KEY_DIR "/%s", cp->name);
}



/**
 * prelude_client_profile_get_tls_server_filename:
 * @cp: pointer on a #prelude_client_profile_t object.
 * @buf: buffer to write the returned filename to.
 * @size: size of @buf.
 *
 * Writes the filename used to store @cp related CA certificate.
 * This only apply to @cp receiving connection from analyzer (server).
 */
void prelude_client_profile_get_tls_server_ca_cert_filename(prelude_client_profile_t *cp, char *buf, size_t size) 
{
        snprintf(buf, size, TLS_SERVER_CERT_DIR "/%s.ca", cp->name);
}



/**
 * prelude_client_profile_get_tls_server_trusted_cert_filename:
 * @cp: pointer on a #prelude_client_profile_t object.
 * @buf: buffer to write the returned filename to.
 * @size: size of @buf.
 *
 * Writes the filename used to store certificate that this @cp trust.
 * This only apply to @cp receiving connection from analyzer (server).
 */
void prelude_client_profile_get_tls_server_trusted_cert_filename(prelude_client_profile_t *cp, char *buf, size_t size) 
{
        snprintf(buf, size, TLS_SERVER_CERT_DIR "/%s.trusted", cp->name);
}



/**
 * prelude_client_profile_get_tls_server_keycert_filename:
 * @cp: pointer on a #prelude_client_profile_t object.
 * @buf: buffer to write the returned filename to.
 * @size: size of @buf.
 *
 * Writes the filename used to store certificate for @cp private key.
 * This only apply to @cp receiving connection from analyzer (server).
 */
void prelude_client_profile_get_tls_server_keycert_filename(prelude_client_profile_t *cp, char *buf, size_t size) 
{
        snprintf(buf, size, TLS_SERVER_CERT_DIR "/%s.keycrt", cp->name);
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
void prelude_client_profile_get_tls_client_trusted_cert_filename(prelude_client_profile_t *cp, char *buf, size_t size) 
{
        snprintf(buf, size, TLS_CLIENT_CERT_DIR "/%s.trusted", cp->name);
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
void prelude_client_profile_get_tls_client_keycert_filename(prelude_client_profile_t *cp, char *buf, size_t size) 
{
        snprintf(buf, size, TLS_CLIENT_CERT_DIR "/%s.keycrt", cp->name);
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
void prelude_client_profile_get_backup_dirname(prelude_client_profile_t *cp, char *buf, size_t size) 
{
        snprintf(buf, size, PRELUDE_SPOOL_DIR "/%s", cp->name);
}



int _prelude_client_profile_new(prelude_client_profile_t **ret)
{
        *ret = calloc(1, sizeof(**ret));
        if ( ! *ret )
                return prelude_error_from_errno(errno);

        return 0;
}



int _prelude_client_profile_init(prelude_client_profile_t *cp)
{
        int ret;
        
        cp->uid = geteuid();
        cp->gid = getegid();
        
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
        
        cp = calloc(1, sizeof(*cp));
        if ( ! cp )
                return prelude_error_from_errno(errno);

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
uid_t prelude_client_profile_get_uid(prelude_client_profile_t *cp)
{
        return cp->uid;
}



/**
 * prelude_client_profile_set_uid:
 * @cp: Pointer to a #prelude_client_profile_t object.
 * @uid: UID to be used by @cp.
 *
 * Sets the UID used by @cp to @uid.
 */
void prelude_client_profile_set_uid(prelude_client_profile_t *cp, uid_t uid)
{
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
gid_t prelude_client_profile_get_gid(prelude_client_profile_t *cp)
{
        return cp->gid;
}



/**
 * prelude_client_profile_set_gid:
 * @cp: Pointer to a #prelude_client_profile_t object.
 * @gid: GID to be used by @cp.
 *
 * Sets the GID used by @cp to @gid.
 */
void prelude_client_profile_set_gid(prelude_client_profile_t *cp, gid_t gid)
{
        cp->gid = gid;
}



/**
 * prelude_client_profile_set_analyzerid:
 * @cp: Pointer to a #prelude_client_profile_t object.
 * @analizerid: Analyzer ID to be used by @cp.
 *
 * Sets the Analyzer ID used by @cp to @analizerid.
 */
void prelude_client_profile_set_analyzerid(prelude_client_profile_t *cp, uint64_t analizerid)
{
        cp->analyzerid = analizerid;
}



/**
 * prelude_client_profile_get_analyzerid:
 * @cp: Pointer to a #prelude_client_profile_t object.
 *
 * Gets the unique and permanent analyzer ident associated with @cp.
 *
 * Returns: the analyzer ident used by @cp.
 */
uint64_t prelude_client_profile_get_analyzerid(prelude_client_profile_t *cp)
{
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
const char *prelude_client_profile_get_name(prelude_client_profile_t *cp)
{
        return cp->name;
}


/**
 * prelude_client_profile_set_name:
 * @cp: Pointer to a #prelude_client_profile_t object.
 * @name: Name to associate the profilte to.
 * 
 * Sets the prelude client profile name.
 * 
 * Returns: 0 on success or a negative value if an error occured.
 */
int prelude_client_profile_set_name(prelude_client_profile_t *cp, const char *name)
{
        if ( cp->name )
                free(cp->name);
        
        cp->name = strdup(name);
        if ( ! cp->name )
                return prelude_error_from_errno(errno);

        return 0;
}



/**
 * prelude_client_profile_set_name:
 * @cp: Pointer to a #prelude_client_profile_t object.
 * @credentials: The GNU TLS certificate credentials retrieved.
 * 
 * Gets the prelude client profile credentials
 * 
 * Returns: 0 on success or a negative value if an error occured.
 */
int prelude_client_profile_get_credentials(prelude_client_profile_t *cp, gnutls_certificate_credentials *credentials)
{
        int ret;
        
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
