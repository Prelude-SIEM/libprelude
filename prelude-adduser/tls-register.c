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
#include <termios.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

#include <gnutls/gnutls.h>
#include <gnutls/x509.h>

#include "prelude-strbuf.h"
#include "prelude-client.h"
#include "tls-register.h"
#include "tls-util.h"



extern int generated_key_size;
extern int authority_certificate_lifetime;
extern int generated_certificate_lifetime;



static int safe_close(int fd)
{
        int ret;

        do {
                ret = close(fd);
        } while ( ret < 0 && errno == EINTR );

        return ret;
}



static ssize_t safe_write(int fd, const char *buf, size_t size)
{
        ssize_t ret;
        
        do {
                ret = write(fd, buf, size);
                
        } while ( ret < 0 && errno == EINTR );

        return ret;
}




static int save_buf(const char *filename, uid_t uid, gid_t gid, char *buf, size_t size)
{
        int fd, ret;
        
        fd = open(filename, O_CREAT|O_WRONLY|O_APPEND, S_IRUSR|S_IWUSR|S_IRGRP);
        if ( fd < 0 ) {
                fprintf(stderr, "couldn't open %s for appending: %s.\n", filename, strerror(errno));
                return -1;
        }

        ret = fchown(fd, uid, gid);
        if ( ret < 0 ) {
                fprintf(stderr, "couldn't set %s owner to UID %d: %s.\n", filename, uid, strerror(errno));
                safe_close(fd);
                return -1;
        }

        ret = safe_write(fd, buf, size);        
        if ( ret != size ) {
                fprintf(stderr, "error writing to %s: %s.\n", filename, strerror(errno));
                safe_close(fd);
                return -1;
        }

        return safe_close(fd);
}



static gnutls_x509_crt generate_certificate(prelude_client_t *client, gnutls_x509_privkey key, int expire)
{
        int ret;
        char buf[1024];
        gnutls_x509_crt crt;
        size_t size = sizeof(buf);
        
        ret = gnutls_x509_crt_init(&crt);
        if ( ret < 0 ) {
                fprintf(stderr, "error creating x509 certificate: %s.\n", gnutls_strerror(ret));
                return NULL;
        }

        if ( ! expire ) 
                expire = 0x7fffffff;
        else
                expire = time(NULL) + expire * 24 * 60 * 60;
        
        gnutls_x509_crt_set_version(crt, 3);
        gnutls_x509_crt_set_ca_status(crt, 0);
        gnutls_x509_crt_set_activation_time(crt, time(NULL));
        gnutls_x509_crt_set_expiration_time(crt, expire);
        
        ret = snprintf(buf, sizeof(buf), "%llu", prelude_client_get_analyzerid(client));
        ret = gnutls_x509_crt_set_dn_by_oid(crt, GNUTLS_OID_X520_COMMON_NAME, 0, buf, ret);
        if ( ret < 0 ) {
                fprintf(stderr, "error setting common name: %s.\n", gnutls_strerror(ret));
                return NULL;
        }
                
        buf[3] = 0 & 0xff;
        buf[2] = (0 >> 8) & 0xff;
        buf[1] = (0 >> 16) & 0xff;
        buf[0] = 0;

        gnutls_x509_crt_set_serial(crt, buf, 4);
        
        if ( ! key )
                return crt;
        
        gnutls_x509_crt_set_key(crt, key);
        
        ret = gnutls_x509_crt_get_key_id(crt, 0, buf, &size);
        if ( ret == 0 ) {
                
                ret = gnutls_x509_crt_set_subject_key_id(crt, buf, size);
                if ( ret < 0 ) {
                        gnutls_x509_crt_deinit(crt);
                        fprintf(stderr, "error setting subject key ID: %s\n", gnutls_strerror(ret));
                        return NULL;
                }
        }
        
        return crt;
}



static gnutls_x509_crt generate_signed_certificate(prelude_client_t *client,
                                                   gnutls_x509_crt ca_crt,
                                                   gnutls_x509_privkey ca_key,
                                                   gnutls_x509_crq crq)
{
        int ret;
        char buf[65535];
        gnutls_x509_crt crt;
        size_t size = sizeof(buf);
        
        crt = generate_certificate(client, NULL, generated_certificate_lifetime);
        if ( ! crt )
                return NULL;
        
        ret = gnutls_x509_crt_set_crq(crt, crq);
        if ( ret < 0 ) {
                fprintf(stderr, "error associating certificate with CRQ: %s.\n", gnutls_strerror(ret));
                goto err;
        }
        
        ret = gnutls_x509_crt_get_key_id(ca_crt, 0, buf, &size);
        if ( ret < 0 ) {
                fprintf(stderr, "error getting CA key ID: %s.\n", gnutls_strerror(ret));
                goto err;
        }
        
        ret = gnutls_x509_crt_set_authority_key_id(crt, buf, size);
        if ( ret < 0 ) {
                fprintf(stderr, "error setting authority key ID: %s?\n", gnutls_strerror(ret));
                goto err;
        }

        ret = gnutls_x509_crt_sign(crt, ca_crt, ca_key);
        if ( ret < 0 ) {
                fprintf(stderr, "error signing certificate: %s.\n", gnutls_strerror(ret));
                goto err;
        }
                
        return crt;

 err:
        gnutls_x509_crt_deinit(crt);
        return NULL;
}




static gnutls_x509_crt generate_ca_certificate(prelude_client_t *client, gnutls_x509_privkey key)
{
        int ret;
        gnutls_x509_crt crt;
        unsigned int usage = 0;
        
        crt = generate_certificate(client, key, authority_certificate_lifetime);
        if ( ! crt )
                return NULL;

        usage |= GNUTLS_KEY_CRL_SIGN;
        usage |= GNUTLS_KEY_KEY_CERT_SIGN;
        usage |= GNUTLS_KEY_KEY_AGREEMENT;
        usage |= GNUTLS_KEY_KEY_ENCIPHERMENT;
        usage |= GNUTLS_KEY_DATA_ENCIPHERMENT;
        usage |= GNUTLS_KEY_DIGITAL_SIGNATURE;
        
        gnutls_x509_crt_set_ca_status(crt, 1);
        gnutls_x509_crt_set_key_usage(crt, usage);
        
        ret = gnutls_x509_crt_sign(crt, crt, key);
        if ( ret < 0 ) {
                fprintf(stderr, "error self-signing certificate: %s.\n", gnutls_strerror(ret));
                gnutls_x509_crt_deinit(crt);
                return NULL;
        }
        
        return crt;
}




static gnutls_x509_privkey generate_private_key(void)
{
        int ret;
        gnutls_x509_privkey key;
                
        ret = gnutls_x509_privkey_init(&key);
        if ( ret < 0 ) {
                fprintf(stderr, "error initializing private key: %s.\n", gnutls_strerror(ret));
                return NULL;
        }

        fprintf(stderr, "    - Generating %d bits RSA private key... ", generated_key_size);
        tcdrain(STDOUT_FILENO);
        
        ret = gnutls_x509_privkey_generate(key, GNUTLS_PK_RSA, generated_key_size, 0);
        if ( ret < 0 ) {
                fprintf(stderr, "error generating private RSA key: %s\n", gnutls_strerror(ret));
                gnutls_x509_privkey_deinit(key);
                return NULL;
        }

        printf("Done.\n\n");
        
        return key;
}



static gnutls_x509_crq generate_certificate_request(prelude_client_t *client,
                                                    gnutls_x509_privkey key, char *buf, size_t *size)
{
        int ret;
        gnutls_x509_crq crq;
                
        ret = gnutls_x509_crq_init(&crq);
        if ( ret < 0 ) {
                fprintf(stderr, "error creating certificate request: %s.\n", gnutls_strerror(ret));
                return NULL;
        }
        
        ret = gnutls_x509_crq_set_key(crq, key);
        if ( ret < 0 ) {
                fprintf(stderr, "error setting certificate request key: %s.\n", gnutls_strerror(ret));
                gnutls_x509_crq_deinit(crq);
                return NULL;
        }
        
        gnutls_x509_crq_set_version(crq, 1);
        
        ret = snprintf(buf, *size, "%llu", prelude_client_get_analyzerid(client));
        ret = gnutls_x509_crq_set_dn_by_oid(crq, GNUTLS_OID_X520_COMMON_NAME, 0, buf, ret);
        if ( ret < 0 ) {
                fprintf(stderr, "error setting common name: %s.\n", gnutls_strerror(ret));
                return NULL;
        }
        
        ret = gnutls_x509_crq_sign(crq, key);
        if ( ret < 0 ) {
                fprintf(stderr, "error signing certificate request: %s.\n", gnutls_strerror(ret));
                gnutls_x509_crq_deinit(crq);
                return NULL;
        }
        
        ret = gnutls_x509_crq_export(crq, GNUTLS_X509_FMT_PEM, buf, size);
        if ( ret < 0 ) {
                fprintf(stderr, "error exporting certificate request: %s.\n", gnutls_strerror(ret));
                gnutls_x509_crq_deinit(crq);
                return NULL;
        }
        
        return crq;
}



static gnutls_x509_privkey gen_crypto(prelude_client_t *client,
                                      const char *filename, uid_t uid, gid_t gid) 
{
        int ret;
        char buf[65535];
        gnutls_x509_privkey key;
        size_t size = sizeof(buf);
                        
	key = generate_private_key();
	if ( ! key ) {
                fprintf(stderr, "error while generating RSA private key.\n");
		return NULL;
	}
        
        ret = gnutls_x509_privkey_export(key, GNUTLS_X509_FMT_PEM, buf, &size);
        if ( ret < 0 ) {
                fprintf(stderr, "error exporting private key: %s\n", gnutls_strerror(ret));
                gnutls_x509_privkey_deinit(key);
                return NULL;
        }
        
        ret = save_buf(filename, uid, gid, buf, size);
        if ( ret < 0 ) {
                fprintf(stderr, "error saving private key.\n");
                return NULL;
        }
        
	return key;
}



gnutls_x509_privkey tls_load_privkey(prelude_client_t *client)
{
        int ret;
        gnutls_datum data;
        char filename[256];
        gnutls_x509_privkey key;
        
        prelude_client_get_tls_key_filename(client, filename, sizeof(filename));

        ret = access(filename, F_OK);
        if ( ret < 0 ) {
                return gen_crypto(client, filename,
                                  prelude_client_get_uid(client),
                                  prelude_client_get_gid(client));
        }

        ret = tls_load_file(filename, &data);
        if ( ret < 0 )
                return NULL;
        
        gnutls_x509_privkey_init(&key);
        gnutls_x509_privkey_import(key, &data, GNUTLS_X509_FMT_PEM);

        tls_unload_file(&data);
        
        return key;
}




int tls_handle_certificate_request(prelude_client_t *client, prelude_io_t *fd,
                                   gnutls_x509_privkey cakey, gnutls_x509_crt cacrt,
                                   gnutls_x509_crt crt)
{
        ssize_t ret;
        size_t size;
        char buf[65535];
        gnutls_datum data;
        gnutls_x509_crq crq;
        unsigned char *rbuf;
        gnutls_x509_crt gencrt;
        
        /*
         * Read the client CRQ and generate a certificate for it.
         */
        fprintf(stderr, "  - Waiting for client certificate request.\n");
        ret = prelude_io_read_delimited(fd, &rbuf);
        if ( ret < 0 ) {
                fprintf(stderr, "error receiving client certificate request.\n");
                return -1;
        }
        
        data.size = ret;
        data.data = rbuf;
        gnutls_x509_crq_init(&crq);
        gnutls_x509_crq_import(crq, &data, GNUTLS_X509_FMT_PEM);
        free(rbuf);
        
        /*
         * Generate a CA signed certificate for this CRQ.
         */
        fprintf(stderr, "  - Generating signed certificate for client.\n");
        
        gencrt = generate_signed_certificate(client, cacrt, cakey, crq);   
        if ( ! gencrt ) {
                fprintf(stderr, "error generating signed certificate for this request.\n");
                return -1;
        }
        
        size = sizeof(buf);
        gnutls_x509_crt_export(gencrt, GNUTLS_X509_FMT_PEM, buf, &size);

        ret = prelude_io_write_delimited(fd, buf, size);        
        if ( ret != size ) {
                fprintf(stderr, "error sending signed certificate.\n");
                return -1;
        }
        
        /*
         * write our own certificate back to the client.
         */
        fprintf(stderr, "  - Sending server certificate to client.\n");
        
        size = sizeof(buf);
        gnutls_x509_crt_export(crt, GNUTLS_X509_FMT_PEM, buf, &size);

        ret = prelude_io_write_delimited(fd, buf, size);
        if ( ret != size ) {
                fprintf(stderr, "error sending signed certificate.\n");
                return -1;
        }
        
        return 0;
}



int tls_request_certificate(prelude_client_t *client, prelude_io_t *fd, gnutls_x509_privkey key)
{
        ssize_t ret;
        unsigned char *rbuf;
        gnutls_x509_crq crq;
        unsigned char buf[65535];
        size_t size = sizeof(buf);

        fprintf(stderr, "  - Sending certificate request.\n");

        crq = generate_certificate_request(client, key, buf, &size);
        if ( ! crq )
                return -1;
        
        ret = prelude_io_write_delimited(fd, buf, size);
        if ( ret != size ) {
                fprintf(stderr, "error sending certificate request.\n");
                return -1;
        }
        
        fprintf(stderr, "  - Receiving CA signed certificate.\n");
        ret = prelude_io_read_delimited(fd, &rbuf);
        if ( ret < 0 ) {
                fprintf(stderr, "error receiving CA-signed certificate.\n");
                return -1;
        }
        
        prelude_client_get_tls_client_keycert_filename(client, buf, sizeof(buf));
        ret = save_buf(buf, prelude_client_get_uid(client), prelude_client_get_gid(client), rbuf, ret);
        if ( ret < 0 ) 
                return -1;

        free(rbuf);
        
        fprintf(stderr, "  - Receiving CA certificate.\n");

        ret = prelude_io_read_delimited(fd, &rbuf);
        if ( ret < 0 ) {
                fprintf(stderr, "error receiving server certificate.\n");
                return -1;
        }
        
        prelude_client_get_tls_client_trusted_cert_filename(client, buf, sizeof(buf));
        ret = save_buf(buf, prelude_client_get_uid(client), prelude_client_get_gid(client), rbuf, ret);
        if ( ret < 0 ) 
                return -1;

        free(rbuf);
        
        return 0;
}



int tls_load_ca_certificate(prelude_client_t *client, gnutls_x509_privkey key, gnutls_x509_crt *crt)
{
        int ret;
        char buf[65535];
        gnutls_datum data;
        char filename[256];
        size_t size = sizeof(buf);
        
        prelude_client_get_tls_server_ca_cert_filename(client, filename, sizeof(filename));

        ret = access(filename, F_OK);
        if ( ret == 0 ) {
                ret = tls_load_file(filename, &data);
                if ( ret < 0 )
                        return -1;
        
                gnutls_x509_crt_init(crt);

                ret = gnutls_x509_crt_import(*crt, &data, GNUTLS_X509_FMT_PEM);
                if ( ret < 0 ) {
                        fprintf(stderr, "error importing certificate: %s.\n", gnutls_strerror(ret));
                        return -1;
                }
                
                tls_unload_file(&data);

                return 0;
        }
        
        *crt = generate_ca_certificate(client, key);
        if ( ! *crt )
                return -1;
                
        ret = gnutls_x509_crt_export(*crt, GNUTLS_X509_FMT_PEM, buf, &size);
        if ( ret < 0 ) {
                fprintf(stderr, "error exporting self-signed certificate: %s.\n", gnutls_strerror(ret));
                gnutls_x509_crt_deinit(*crt);
                return -1;
        }

        ret = save_buf(filename, prelude_client_get_uid(client), prelude_client_get_gid(client), buf, size);
        if ( ret < 0 ) {
                fprintf(stderr, "error saving private key certificate.\n");
                gnutls_x509_crt_deinit(*crt);
                return -1;
        }
        
        return 0;
}



int tls_load_ca_signed_certificate(prelude_client_t *client,
                                   gnutls_x509_privkey cakey,
                                   gnutls_x509_crt cacrt,
                                   gnutls_x509_crt *crt)
{
        int ret;
        char buf[65535];
        gnutls_datum data;
        char filename[256];
        gnutls_x509_crq crq;
        size_t size = sizeof(buf);
        
        prelude_client_get_tls_server_keycert_filename(client, filename, sizeof(filename));

        ret = access(filename, F_OK);
        if ( ret == 0 ) {                
                ret = tls_load_file(filename, &data);
                if ( ret < 0 )
                        return -1;
        
                gnutls_x509_crt_init(crt);

                ret = gnutls_x509_crt_import(*crt, &data, GNUTLS_X509_FMT_PEM);
                if ( ret == 0 ) {                
                        tls_unload_file(&data);
                        return 0;
                }
        }
        
        crq = generate_certificate_request(client, cakey, buf, &size);
        if ( ! crq )
                return -1;
        
        *crt = generate_signed_certificate(client, cacrt, cakey, crq);
        if ( ! *crt )
                return -1;

        size = sizeof(buf);
        
        ret = gnutls_x509_crt_export(*crt, GNUTLS_X509_FMT_PEM, buf, &size);
        if ( ret < 0 ) {
                fprintf(stderr, "error exporting self-signed certificate: %s.\n", gnutls_strerror(ret));
                gnutls_x509_crt_deinit(*crt);
                return -1;
        }

        ret = save_buf(filename, prelude_client_get_uid(client), prelude_client_get_gid(client), buf, size);
        if ( ret < 0 ) {
                fprintf(stderr, "error saving private key certificate.\n");
                gnutls_x509_crt_deinit(*crt);
                return -1;
        }
        
        return 0;
}

