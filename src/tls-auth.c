/*****
*
* Copyright (C) 2004,2005,2006,2007 PreludeIDS Technologies. All Rights Reserved.
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
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>

#include <gnutls/gnutls.h>
#include <gnutls/x509.h>

#include "glthread/lock.h"

#define PRELUDE_ERROR_SOURCE_DEFAULT PRELUDE_ERROR_SOURCE_CONNECTION
#include "prelude-error.h"

#include "common.h"
#include "prelude-log.h"
#include "prelude-client.h"
#include "prelude-message-id.h"
#include "prelude-extract.h"

#include "tls-util.h"
#include "tls-auth.h"



#ifdef HAVE_GNUTLS_STRING_PRIORITY

static gnutls_priority_t tls_priority;

#endif

static prelude_bool_t priority_set = FALSE;



static int read_auth_result(prelude_io_t *fd)
{
        int ret;
        void *buf;
        uint8_t tag;
        uint32_t len;
        prelude_msg_t *msg = NULL;

        do {
                ret = prelude_msg_read(&msg, fd);
        } while ( ret < 0 && prelude_error_get_code(ret) == PRELUDE_ERROR_EAGAIN );

        if ( ret < 0 )
                return ret;

        if ( prelude_msg_get_tag(msg) != PRELUDE_MSG_AUTH ) {
                prelude_msg_destroy(msg);
                return prelude_error(PRELUDE_ERROR_INVAL_MESSAGE);
        }

        ret = prelude_msg_get(msg, &tag, &len, &buf);
        if ( ret < 0 ) {
                prelude_msg_destroy(msg);
                return ret;
        }

        if ( tag != PRELUDE_MSG_AUTH_SUCCEED ) {
                prelude_msg_destroy(msg);
                return prelude_error(PRELUDE_ERROR_TLS_AUTH_REJECTED);
        }

        prelude_msg_destroy(msg);

        return 0;
}



static int verify_certificate(gnutls_session session)
{
        time_t now;
        int ret, alert = 0;
        unsigned int status;
        const prelude_error_code_t code = PRELUDE_ERROR_PROFILE;

        ret = gnutls_certificate_verify_peers2(session, &status);
        if ( ret < 0 ) {
                gnutls_alert_send_appropriate(session, ret);
                return prelude_error_verbose(code, "TLS certificate verification failed: %s", gnutls_strerror(ret));
        }

        if ( status & GNUTLS_CERT_INVALID ) {
                alert = GNUTLS_A_BAD_CERTIFICATE;
                ret = prelude_error_verbose(code, "TLS server certificate is NOT trusted");
        }

        else if ( status & GNUTLS_CERT_REVOKED ) {
                alert = GNUTLS_A_CERTIFICATE_REVOKED;
                ret = prelude_error_verbose(code, "TLS server certificate was revoked");
        }

        else if ( status & GNUTLS_CERT_SIGNER_NOT_FOUND) {
                alert = GNUTLS_A_UNKNOWN_CA;
                ret = prelude_error_verbose(code, "TLS server certificate issuer is unknown");
        }

        else if ( status & GNUTLS_CERT_SIGNER_NOT_CA ) {
                alert = GNUTLS_A_CERTIFICATE_UNKNOWN;
                ret = prelude_error_verbose(code, "TLS server certificate issuer is not a CA");
        }

#ifdef GNUTLS_CERT_INSECURE_ALGORITHM
        else if ( status & GNUTLS_CERT_INSECURE_ALGORITHM ) {
                alert = GNUTLS_A_INSUFFICIENT_SECURITY;
                ret = prelude_error_verbose(code, "TLS server certificate use insecure algorithm");
        }
#endif

        now = time(NULL);

        if ( gnutls_certificate_activation_time_peers(session) > now )
                ret = prelude_error_verbose(code, "TLS server certificate not yet activated");

        if ( gnutls_certificate_expiration_time_peers(session) < now )
                ret = prelude_error_verbose(code, "TLS server certificate expired");

        if ( ret < 0 )
                gnutls_alert_send(session, GNUTLS_AL_FATAL, alert);

        return ret;
}




static int handle_gnutls_error(gnutls_session session, int ret)
{
        int last_alert;

        if ( ret == GNUTLS_E_WARNING_ALERT_RECEIVED ) {
                last_alert = gnutls_alert_get(session);
                prelude_log(PRELUDE_LOG_WARN, "TLS: received warning alert: %s.\n", gnutls_alert_get_name(last_alert));
                return 0;
        }

        else if ( ret == GNUTLS_E_FATAL_ALERT_RECEIVED ) {
                last_alert = gnutls_alert_get(session);
                prelude_log(PRELUDE_LOG_WARN, "TLS: received fatal alert: %s.\n", gnutls_alert_get_name(last_alert));
                return -1;
        }

        if ( ret == GNUTLS_E_AGAIN || ret == GNUTLS_E_INTERRUPTED )
                return 0;

        gnutls_alert_send_appropriate(session, ret);
        return ret;
}



static inline gnutls_transport_ptr fd_to_ptr(int fd)
{
        union {
                gnutls_transport_ptr ptr;
                int fd;
        } data;

        data.fd = fd;

        return data.ptr;
}


static inline int ptr_to_fd(gnutls_transport_ptr ptr)
{
        union {
                gnutls_transport_ptr ptr;
                int fd;
        } data;

        data.ptr = ptr;

        return data.fd;
}


static void set_default_priority(gnutls_session session)
{
#ifdef HAVE_GNUTLS_STRING_PRIORITY
        gnutls_priority_set(session, tls_priority);
#else
        const int c_prio[] = { GNUTLS_COMP_NULL, 0 };

        gnutls_set_default_priority(session);

        /*
         * We override the default compression method since in early GnuTLS
         * version, DEFLATE would be the default, and NULL would not be
         * available.
         */
        gnutls_compression_set_priority(session, c_prio);
#endif
}


int tls_auth_init_priority(const char *tlsopts)
{
#ifdef HAVE_GNUTLS_STRING_PRIORITY
        {
                int ret;
                const char *errptr;

                ret = gnutls_priority_init(&tls_priority, (tlsopts) ? tlsopts : "NORMAL", &errptr);
                if ( ret < 0 )
                        return prelude_error_verbose_make(PRELUDE_ERROR_SOURCE_CLIENT, PRELUDE_ERROR_TLS,
                                                          "TLS options '%s': %s", errptr, gnutls_strerror(ret));
        }
#else
        if ( tlsopts )
                return prelude_error_verbose_make(PRELUDE_ERROR_SOURCE_CLIENT, PRELUDE_ERROR_TLS,
                                                  "settings TLS options require GnuTLS 2.2.0 or above.\n");
#endif

        priority_set = TRUE;

        return 0;
}



static ssize_t tls_pull(gnutls_transport_ptr fd, void *buf, size_t count)
{
        return read(ptr_to_fd(fd), buf, count);
}



static ssize_t tls_push(gnutls_transport_ptr fd, const void *buf, size_t count)
{
        return write(ptr_to_fd(fd), buf, count);
}


int tls_auth_connection(prelude_client_profile_t *cp, prelude_io_t *io, int crypt,
                        uint64_t *analyzerid, prelude_connection_permission_t *permission)
{
        void *cred;
        int ret, fd;
        gnutls_session session;

        if ( ! priority_set ) {
                ret = tls_auth_init_priority(NULL);
                if ( ret < 0 )
                        return ret;
        }

        ret = prelude_client_profile_get_credentials(cp, &cred);
        if ( ret < 0 )
                return ret;

        ret = gnutls_init(&session, GNUTLS_CLIENT);
        if ( ret < 0 )
                return prelude_error_verbose(PRELUDE_ERROR_PROFILE, "TLS initialization error: %s", gnutls_strerror(ret));

        set_default_priority(session);
        gnutls_credentials_set(session, GNUTLS_CRD_CERTIFICATE, cred);

        fd = prelude_io_get_fd(io);
        gnutls_transport_set_ptr(session, fd_to_ptr(fd));
        gnutls_transport_set_pull_function(session, tls_pull);
        gnutls_transport_set_push_function(session, tls_push);

        do {
                ret = gnutls_handshake(session);
        } while ( ret < 0 && handle_gnutls_error(session, ret) == 0 );

        if ( ret < 0 ) {
                gnutls_deinit(session);
                return prelude_error_verbose(PRELUDE_ERROR_PROFILE, "TLS handshake failed: %s", gnutls_strerror(ret));
        }

        ret = verify_certificate(session);
        if ( ret < 0 ) {
                gnutls_deinit(session);
                return ret;
        }

        prelude_io_set_tls_io(io, session);

        ret = read_auth_result(io);
        if ( ret < 0 )
                return ret;

        ret = tls_certificate_get_peer_analyzerid(session, analyzerid);
        if ( ret < 0 )
                return ret;

        ret = tls_certificate_get_permission(session, permission);
        if ( ret < 0 )
                return ret;

        if ( ! crypt ) {

                do {
                        ret = gnutls_bye(session, GNUTLS_SHUT_RDWR);
                } while ( ret < 0 && handle_gnutls_error(session, ret) == 0 );

                if ( ret < 0 )
                        ret = prelude_error_verbose(PRELUDE_ERROR_TLS, "TLS bye failed: %s", gnutls_strerror(ret));

                gnutls_deinit(session);
                prelude_io_set_sys_io(io, fd);
        }

        return ret;
}



int tls_auth_init(prelude_client_profile_t *cp, gnutls_certificate_credentials *cred)
{
        int ret;
        size_t size;
        gnutls_datum data;
        gnutls_x509_privkey key;
        char keyfile[PATH_MAX], certfile[PATH_MAX];

        *cred = NULL;

        prelude_client_profile_get_tls_key_filename(cp, keyfile, sizeof(keyfile));
        ret = access(keyfile, F_OK);
        if ( ret < 0 )
                return prelude_error_verbose_make(PRELUDE_ERROR_SOURCE_CLIENT, PRELUDE_ERROR_PROFILE,
                                                  "access to %s failed: %s", keyfile, strerror(errno));

        prelude_client_profile_get_tls_client_keycert_filename(cp, certfile, sizeof(certfile));
        ret = access(certfile, F_OK);
        if ( ret < 0 )
                return prelude_error_verbose_make(PRELUDE_ERROR_SOURCE_CLIENT, PRELUDE_ERROR_PROFILE,
                                                  "access to %s failed: %s", certfile, strerror(errno));

        ret = _prelude_load_file(keyfile, &data.data, &size);
        if ( ret < 0 )
                return ret;
        data.size = (unsigned int) size;

        ret = gnutls_x509_privkey_init(&key);
        if ( ret < 0 ) {
                _prelude_unload_file(data.data, data.size);
                return prelude_error_verbose_make(PRELUDE_ERROR_SOURCE_CLIENT, PRELUDE_ERROR_TLS,
                                                  "Error initializing X509 private key: %s", gnutls_strerror(ret));
        }

        ret = gnutls_x509_privkey_import(key, &data, GNUTLS_X509_FMT_PEM);
        if ( ret < 0 ) {
                ret = prelude_error_verbose_make(PRELUDE_ERROR_SOURCE_CLIENT, PRELUDE_ERROR_TLS,
                                                 "Error importing X509 private key: %s", gnutls_strerror(ret));
                goto err;
        }

        ret = gnutls_certificate_allocate_credentials(cred);
        if ( ret < 0 ) {
                ret = prelude_error_verbose_make(PRELUDE_ERROR_SOURCE_CLIENT, PRELUDE_ERROR_TLS,
                                                 "Error initializing TLS credentials: %s", gnutls_strerror(ret));
                goto err;
        }

        ret = tls_certificates_load(key, certfile, *cred);
        if ( ret < 0 )
                goto err;

        prelude_client_profile_get_tls_client_trusted_cert_filename(cp, certfile, sizeof(certfile));
        ret = gnutls_certificate_set_x509_trust_file(*cred, certfile, GNUTLS_X509_FMT_PEM);
        if ( ret < 0 )
                ret = prelude_error_verbose_make(PRELUDE_ERROR_SOURCE_CLIENT, PRELUDE_ERROR_PROFILE,
                                                 "could not set x509 trust file '%s': %s", certfile, gnutls_strerror(ret));

   err:
        if ( ret < 0 && *cred )
                gnutls_certificate_free_credentials(*cred);

        gnutls_x509_privkey_deinit(key);
        _prelude_unload_file(data.data, data.size);

        return ret;
}


void tls_auth_deinit(void)
{
#ifdef HAVE_GNUTLS_STRING_PRIORITY
        if ( priority_set ) {
                gnutls_priority_deinit(tls_priority);
                priority_set = FALSE;
        }
#endif
}
