/*****
*
* Copyright (C) 2004 Yoann Vandoorselaere <yoann@prelude-ids.org>
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
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <gnutls/gnutls.h>
#include <gnutls/x509.h>

#include "prelude-log.h"
#include "prelude-client.h"
#include "prelude-message-id.h"

#include "tls-util.h"
#include "tls-auth.h"



static int read_auth_result(prelude_io_t *fd)
{
        int ret;
        void *buf;
        uint8_t tag;
        uint32_t len;
        prelude_msg_t *msg = NULL;
        
        do {
                ret = prelude_msg_read(&msg, fd);
                
        } while ( ret == prelude_msg_unfinished );

        if ( ret == prelude_msg_error )
                return -1;
        
        if ( prelude_msg_get_tag(msg) != PRELUDE_MSG_AUTH ) {
                prelude_msg_destroy(msg);
                return -1;
        }
        
        prelude_msg_get(msg, &tag, &len, &buf);
        prelude_msg_destroy(msg);
        
        return (tag == PRELUDE_MSG_AUTH_SUCCEED) ? 0 : -1;
}



static int verify_certificate(gnutls_session session)
{
	int ret;
        
	ret = gnutls_certificate_verify_peers(session);
	if ( ret < 0 ) {
                log(LOG_INFO, "- TLS certificate error: %s.\n", gnutls_strerror(ret));
                return -1;
        }
        
	if ( ret == GNUTLS_E_NO_CERTIFICATE_FOUND ) {
		log(LOG_INFO, "- TLS certificate error: server did not send any certificate.\n");
		return -1;
	}

        if ( ret & GNUTLS_CERT_SIGNER_NOT_FOUND) {
		log(LOG_INFO, "- TLS certificate error: server certificate issuer is unknown.\n");
                return -1;
        }
        
        if ( ret & GNUTLS_CERT_INVALID ) {
                log(LOG_INFO, "- TLS certificate error: server certificate is NOT trusted.\n");
                return -1;
        }

        log(LOG_INFO, "- TLS certificate: server certificate is trusted.\n");

        return 0;
}




static void handle_gnutls_error(gnutls_session session, int ret)
{
        int last_alert;
        
        if ( ret == GNUTLS_E_WARNING_ALERT_RECEIVED || ret == GNUTLS_E_FATAL_ALERT_RECEIVED ) {
                last_alert = gnutls_alert_get(session);
                log(LOG_INFO, "- Received alert: %s.\n", gnutls_alert_get_name(last_alert));
        }

        log(LOG_INFO, "- TLS handshake failed: %s.\n", gnutls_strerror(ret));
}



int tls_auth_connection(prelude_client_t *client, prelude_io_t *io, int crypt)
{
	int ret, fd;
        gnutls_session session;
        gnutls_certificate_credentials cred = prelude_client_get_credentials(client);
        
        gnutls_init(&session, GNUTLS_CLIENT);
        gnutls_set_default_priority(session);
        gnutls_credentials_set(session, GNUTLS_CRD_CERTIFICATE, cred);
        
        fd = prelude_io_get_fd(io);
        gnutls_transport_set_ptr(session, (gnutls_transport_ptr) fd);

        ret = gnutls_handshake(session);
        if ( ret < 0 ) {
                handle_gnutls_error(session, ret);
                log(LOG_INFO, "- TLS handshake failed: %s.\n", gnutls_strerror(ret));
                gnutls_deinit(session);
                return -1;
        }

        ret = verify_certificate(session);
        if ( ret < 0 ) {
                gnutls_deinit(session);
                return -1;
        }
        
        prelude_io_set_tls_io(io, session);

        ret = read_auth_result(io);
        if ( ret < 0 ) {
                gnutls_deinit(session);
                return -1;
        }

        if ( ! crypt ) {
                
                do {
                        ret = gnutls_bye(session, GNUTLS_SHUT_RDWR);
                } while ( ret < 0 && ret == GNUTLS_E_INTERRUPTED );

                if ( ret < 0 )
                        log(LOG_ERR, "gnutls_bye() error: %s.\n", gnutls_strerror(ret));
                
                gnutls_deinit(session);
                prelude_io_set_sys_io(io, fd);
        }
        
        return ret;
}



void *tls_auth_init(prelude_client_t *client)
{
        int ret;
        char keyfile[256], certfile[256];
        gnutls_certificate_credentials cred;
        
        gnutls_global_init();
        gnutls_certificate_allocate_credentials(&cred);

        prelude_client_get_tls_key_filename(client, keyfile, sizeof(keyfile));
        prelude_client_get_tls_client_keycert_filename(client, certfile, sizeof(certfile));
        
        ret = access(certfile, F_OK);
        if ( ret < 0 ) 
                return cred;
        
        ret = tls_certificates_load(keyfile, certfile, cred);
        if ( ret < 0 ) {
                log(LOG_ERR, "error loading certificate list.\n");
                return NULL;
        }

        prelude_client_get_tls_client_trusted_cert_filename(client, certfile, sizeof(certfile));
        
        ret = gnutls_certificate_set_x509_trust_file(cred, certfile, GNUTLS_X509_FMT_PEM);
        if ( ret < 0 ) {
                log(LOG_INFO, "- couldn't set x509 trust file: %s.\n", gnutls_strerror(ret));
                return NULL;
        }

        return cred;
}
