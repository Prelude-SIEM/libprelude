/*****
*
* Copyright (C) 2004, 2005 PreludeIDS Technologies. All Rights Reserved.
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
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <gnutls/gnutls.h>
#include <gnutls/x509.h>

#define PRELUDE_ERROR_SOURCE_DEFAULT PRELUDE_ERROR_SOURCE_CLIENT
#include "prelude-error.h"

#include "common.h"
#include "prelude-log.h"
#include "prelude-client.h"
#include "tls-util.h"


#ifndef HAVE_GNUTLS_X509_CRT_LIST_IMPORT

#define X509_BEGIN_STR1 "-----BEGIN X509 CERTIFICATE"
#define X509_BEGIN_STR2 "-----BEGIN CERTIFICATE"

int _prelude_tls_crt_list_import(gnutls_x509_crt *certs, unsigned int *cmax,
                                 const gnutls_datum *indata, gnutls_x509_crt_fmt format)
{
        int ret;
        size_t skiplen;
        gnutls_datum data;
        unsigned int i = 0;
        unsigned char *ptr;

        data.size = indata->size;
        data.data = indata->data;

        while ( i < *cmax || ! certs ) {
                skiplen = sizeof(X509_BEGIN_STR1) - 1;
                ptr = memmem(data.data, data.size, X509_BEGIN_STR1, skiplen);
                if ( ! ptr ) {
                        skiplen = sizeof(X509_BEGIN_STR2) - 1;
                        ptr = memmem(data.data, data.size, X509_BEGIN_STR2, skiplen);
                }

                if ( ! ptr )
                        break;

                data.data = ptr;
                data.size = data.size - (ptr - data.data);

                if ( ! certs )
                        i++;
                else {
                        ret = gnutls_x509_crt_init(&certs[i]);
                        if ( ret < 0 )
                                goto err;

                        ret = gnutls_x509_crt_import(certs[i++], &data, format);
                        if ( ret < 0 )
                                goto err;
                }

                data.data += skiplen;
                data.size -= skiplen;
        }

        *cmax = i;
        return i;

   err:
        *cmax = 0;
        while ( i-- >= 0 )
                gnutls_x509_crt_deinit(certs[i]);

        return ret;
}

#else

int _prelude_tls_crt_list_import(gnutls_x509_crt *certs, unsigned int *cmax,
                                 const gnutls_datum *indata, gnutls_x509_crt_fmt format)
{
        return gnutls_x509_crt_list_import(certs, cmax, indata, format, GNUTLS_X509_CRT_LIST_IMPORT_FAIL_IF_EXCEED);
}

#endif


int tls_certificates_load(gnutls_x509_privkey key, const char *certfname, gnutls_certificate_credentials cred)
{
        int ret;
        size_t size;
        gnutls_datum certfile;
        unsigned int cert_max, i;
        gnutls_x509_crt certs[1024];

        ret = _prelude_load_file(certfname, &certfile.data, &size);
        if ( ret < 0 )
                return ret;
        certfile.size = (unsigned int) size;

        cert_max = sizeof(certs) / sizeof(*certs);
        ret = _prelude_tls_crt_list_import(certs, &cert_max, &certfile, GNUTLS_X509_FMT_PEM);
        if ( ret < 0 ) {
                ret = prelude_error_verbose(PRELUDE_ERROR_PROFILE, "error importing certificate listing: %s", gnutls_strerror(ret));
                goto err;
        }

        for ( i = 0; i < cert_max; i++) {
                ret = gnutls_certificate_set_x509_key(cred, &certs[i], 1, key);
                gnutls_x509_crt_deinit(certs[i]);

                if ( ret < 0 ) {
                        ret = prelude_error_verbose(PRELUDE_ERROR_PROFILE, "error importing certificate: %s", gnutls_strerror(ret));
                        break;
                }
        }

err:
        _prelude_unload_file(certfile.data, certfile.size);

        return ret;
}



int tls_certificate_get_peer_analyzerid(gnutls_session session, uint64_t *analyzerid)
{
        int ret;
        char buf[1024];
        gnutls_x509_crt cert;
        size_t size = sizeof(buf);
        unsigned int cert_list_size;
        const gnutls_datum *cert_list;

        cert_list = gnutls_certificate_get_peers(session, &cert_list_size);
        if ( ! cert_list || cert_list_size != 1 )
                return prelude_error_verbose(PRELUDE_ERROR_TLS, "invalid number of peer certificate: %d", cert_list_size);

        ret = gnutls_x509_crt_init(&cert);
        if ( ret < 0 )
                return prelude_error_verbose(PRELUDE_ERROR_TLS, "%s", gnutls_strerror(ret));

        ret = gnutls_x509_crt_import(cert, &cert_list[0], GNUTLS_X509_FMT_DER);
        if ( ret < 0) {
                gnutls_x509_crt_deinit(cert);
                return prelude_error_verbose(PRELUDE_ERROR_TLS, "error importing certificate: %s", gnutls_strerror(ret));
        }

        size = sizeof(buf);
        ret = gnutls_x509_crt_get_dn_by_oid(cert, GNUTLS_OID_X520_DN_QUALIFIER, 0, 0, buf, &size);
        if ( ret < 0 ) {
                gnutls_x509_crt_deinit(cert);
                return prelude_error_verbose(PRELUDE_ERROR_TLS, "certificate miss DN qualifier");
        }

        ret = sscanf(buf, "%" PRELUDE_PRIu64, analyzerid);
        if ( ret != 1 ) {
                gnutls_x509_crt_deinit(cert);
                return prelude_error_verbose(PRELUDE_ERROR_TLS, "certificate analyzerid '%s' is invalid", buf);
        }

        gnutls_x509_crt_deinit(cert);

        return 0;
}



int tls_certificate_get_permission(gnutls_session session,
                                   prelude_connection_permission_t *permission)
{
        int ret, tmp;
        char buf[1024];
        gnutls_x509_crt cert;
        size_t size = sizeof(buf);
        const gnutls_datum *data;

        data = gnutls_certificate_get_ours(session);
        if ( ! data )
                return prelude_error_verbose(PRELUDE_ERROR_TLS, "could not get own certificate");

        ret = gnutls_x509_crt_init(&cert);
        if ( ret < 0 )
                return prelude_error_verbose(PRELUDE_ERROR_TLS, "error initializing certificate: %s", gnutls_strerror(ret));

        ret = gnutls_x509_crt_import(cert, data, GNUTLS_X509_FMT_DER);
        if ( ret < 0 ) {
                gnutls_x509_crt_deinit(cert);
                return prelude_error_verbose(PRELUDE_ERROR_TLS, "error importing certificate: %s", gnutls_strerror(ret));
        }

        ret = gnutls_x509_crt_get_dn_by_oid(cert, GNUTLS_OID_X520_COMMON_NAME, 0, 0, buf, &size);
        if ( ret < 0 ) {
                gnutls_x509_crt_deinit(cert);
                return prelude_error_verbose(PRELUDE_ERROR_TLS, "could not get certificate CN field: %s", gnutls_strerror(ret));
        }

        ret = sscanf(buf, "%d", &tmp);
        if ( ret != 1 ) {
                gnutls_x509_crt_deinit(cert);
                return prelude_error_verbose(PRELUDE_ERROR_TLS, "certificate analyzerid value '%s' is invalid", buf);
        }

        *permission = (prelude_connection_permission_t) tmp;
        gnutls_x509_crt_deinit(cert);

        return 0;
}
