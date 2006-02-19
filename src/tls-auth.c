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
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <gcrypt.h>
#include <gnutls/gnutls.h>
#include <gnutls/x509.h>

#define PRELUDE_ERROR_SOURCE_DEFAULT PRELUDE_ERROR_SOURCE_CONNECTION
#include "prelude-error.h"

#include "prelude-log.h"
#include "prelude-client.h"
#include "prelude-message-id.h"
#include "prelude-extract.h"
#include "prelude-thread.h"

#include "tls-util.h"
#include "tls-auth.h"


GCRY_THREAD_OPTION_PTHREAD_IMPL;


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
	int ret, status, alert = 0;
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
                ret = prelude_error_verbose(code, "TLS server certificate not yet activated.\n");

        if ( gnutls_certificate_expiration_time_peers(session) < now )
                ret = prelude_error_verbose(code, "TLS server certificate expired.\n");
        
        if ( ret < 0 )
                gnutls_alert_send(session, GNUTLS_AL_FATAL, alert);
        
        return ret;
}




static int handle_gnutls_error(gnutls_session session, int ret)
{
        int last_alert;
        
        if ( ret == GNUTLS_E_WARNING_ALERT_RECEIVED ) {
                last_alert = gnutls_alert_get(session);
                prelude_log(PRELUDE_LOG_WARN, "- TLS: received warning alert: %s.\n", gnutls_alert_get_name(last_alert));
                return 0;
        }
        
        else if ( ret == GNUTLS_E_FATAL_ALERT_RECEIVED ) {
                last_alert = gnutls_alert_get(session);
                prelude_log(PRELUDE_LOG_ERR, "- TLS: received fatal alert: %s.\n", gnutls_alert_get_name(last_alert));
                return -1;
        }

        if ( ret == GNUTLS_E_AGAIN || ret == GNUTLS_E_INTERRUPTED )
                return 0;

        gnutls_alert_send_appropriate(session, ret);
        return ret;
}



static void *fd_to_ptr(int fd)
{
        union {
                void *ptr;
                int fd;
        } data;

        data.fd = fd;

        return data.ptr;
}



int tls_auth_connection(prelude_client_profile_t *cp, prelude_io_t *io, int crypt,
                        uint64_t *analyzerid, prelude_connection_permission_t *permission)
{
	int ret, fd;
        void *cred;
        gnutls_session session;

        ret = prelude_client_profile_get_credentials(cp, &cred);
        if ( ret < 0 )
                return ret;
        
        gnutls_init(&session, GNUTLS_CLIENT);
        gnutls_set_default_priority(session);
        gnutls_credentials_set(session, GNUTLS_CRD_CERTIFICATE, cred);

        fd = prelude_io_get_fd(io);
        gnutls_transport_set_ptr(session, fd_to_ptr(fd));

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
        char keyfile[256], certfile[256];

        if ( _prelude_thread_in_use() )
                gcry_control(GCRYCTL_SET_THREAD_CBS, &gcry_threads_pthread);
        
        ret = gnutls_global_init();
        if ( ret < 0 )
                return prelude_error_verbose_make(PRELUDE_ERROR_SOURCE_CLIENT, PRELUDE_ERROR_TLS,
                                                  "TLS initialization failed: %s", gnutls_strerror(ret));
        
        prelude_client_profile_get_tls_key_filename(cp, keyfile, sizeof(keyfile));
        prelude_client_profile_get_tls_client_keycert_filename(cp, certfile, sizeof(certfile));
        
        gnutls_certificate_allocate_credentials(cred);
        
        ret = access(certfile, F_OK);
        if ( ret < 0 )
                return prelude_error_verbose_make(PRELUDE_ERROR_SOURCE_CLIENT, PRELUDE_ERROR_PROFILE,
                                                  "access to %s failed: %s", certfile, strerror(errno));
        
        ret = tls_certificates_load(keyfile, certfile, *cred);
        if ( ret < 0 )
                return ret;

        prelude_client_profile_get_tls_client_trusted_cert_filename(cp, certfile, sizeof(certfile));
        
        ret = gnutls_certificate_set_x509_trust_file(*cred, certfile, GNUTLS_X509_FMT_PEM);
        if ( ret < 0 )
                return prelude_error_verbose_make(PRELUDE_ERROR_SOURCE_CLIENT, PRELUDE_ERROR_PROFILE,
                                                  "could not set x509 trust file '%s': %s", certfile, gnutls_strerror(ret));
        
        return 0;
}
