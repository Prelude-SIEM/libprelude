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
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>

#include <gnutls/gnutls.h>
#include <gnutls/x509.h>

#define PRELUDE_ERROR_SOURCE_DEFAULT PRELUDE_ERROR_SOURCE_CLIENT
#include "prelude-error.h"

#include "common.h"
#include "prelude-log.h"
#include "prelude-client.h"
#include "tls-util.h"

/*
 * can be:
 * -----BEGIN CERTIFICATE-----
 * -----BEGIN X509 CERTIFICATE-----
 */

#define BEGIN_STR       "-----BEGIN "
#define END_STR         "-----END "



static int load_individual_cert(FILE *fd, gnutls_datum *key, gnutls_certificate_credentials cred)
{
        size_t len;
        char buf[65535];
        gnutls_datum cert;
        int ret = -1, got_start = 0;
        
        cert.data = buf;
        
        for ( cert.size = 0;
              cert.size < sizeof(buf) && fgets(buf + cert.size, sizeof(buf) - cert.size, fd);
              cert.size += len ) {

                len = strlen(buf + cert.size);
                
                if ( ! got_start && strstr(buf + cert.size, BEGIN_STR) ) {
                        got_start = 1;                        
                        continue;
                }

                if ( ! strstr(buf + cert.size, END_STR) ) 
                        continue;
                
                cert.size += len;

                ret = gnutls_certificate_set_x509_key_mem(cred, &cert, key, GNUTLS_X509_FMT_PEM);
                if ( ret < 0 ) {
                        prelude_log(PRELUDE_LOG_WARN, "error importing certificate: %s.\n", gnutls_strerror(ret));
                        goto out;
                }
                
                len = cert.size = 0;
        }

  out:
        
        return ret;
}



int tls_load_file(const char *filename, gnutls_datum *data)
{
        int ret, fd;
        struct stat st;

        fd = open(filename, O_RDONLY);
        if ( fd < 0 )
                return prelude_error(PRELUDE_ERROR_TLS_KEY);
        
        ret = fstat(fd, &st);
        if ( ret < 0 ) {
                close(fd);
                return prelude_error_from_errno(errno);
        }
        
        data->data = mmap(NULL, st.st_size, PROT_READ, MAP_SHARED, fd, 0);
        if ( ! data->data ) {
                close(fd);
                return prelude_error_from_errno(errno);
        }

        close(fd);
        data->size = st.st_size;
        
        return 0;
}



void tls_unload_file(gnutls_datum *data)
{
        munmap(data->data, data->size);
}




int tls_certificates_load(const char *keyfile, const char *certfile, gnutls_certificate_credentials cred)
{
        int ret;
        FILE *fd;
        gnutls_datum key;

        ret = tls_load_file(keyfile, &key);
        if ( ret < 0 )
                return ret;
        
        fd = fopen(certfile, "r");
        if ( ! fd ) {
                tls_unload_file(&key);
                return prelude_error(PRELUDE_ERROR_TLS_CERTIFICATE_FILE);
        }
        
        ret = load_individual_cert(fd, &key, cred);
        if ( ret < 0 )
                ret = prelude_error_from_errno(errno);
        
        tls_unload_file(&key);
        fclose(fd);
        
        return ret;
}



int tls_certificate_get_peer_analyzerid(gnutls_session session, uint64_t *analyzerid)
{
        size_t size;
        char buf[1024];
        int cert_list_size, ret;
        const gnutls_datum_t* cert_list;
        gnutls_x509_crt cert;
        
        cert_list = gnutls_certificate_get_peers(session, &cert_list_size);
        if ( ! cert_list )
                return -1;

        ret = gnutls_x509_crt_init(&cert);
        if ( ret < 0 )
                return -1;
        
        ret = gnutls_x509_crt_import(cert, &cert_list[0], GNUTLS_X509_FMT_DER);
        if ( ret < 0) {
                gnutls_x509_crt_deinit(cert);
                return prelude_error(PRELUDE_ERROR_TLS_CERTIFICATE_PARSE);
        }

        size = sizeof(buf);
        ret = gnutls_x509_crt_get_dn(cert, buf, &size);
        if ( ret < 0 ) {
                gnutls_x509_crt_deinit(cert);
                return prelude_error(PRELUDE_ERROR_TLS_INVALID_CERTIFICATE);
        }

        ret = sscanf(buf, "CN=%" PRIu64, analyzerid);
        if ( ret != 1 ) {
                gnutls_x509_crt_deinit(cert);
                return prelude_error(PRELUDE_ERROR_TLS_INVALID_CERTIFICATE);
        }
        
        gnutls_x509_crt_deinit(cert);
        
        return 0;
}
