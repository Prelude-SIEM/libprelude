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

#include "libmissing.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

#include <gnutls/gnutls.h>
#include <gnutls/x509.h>
#include <gcrypt.h>

#include "prelude-client.h"
#include "prelude-error.h"
#include "tls-register.h"
#include "common.h"


#if (defined _WIN32 || defined __WIN32__) && !defined __CYGWIN__
# define fchown(x, y, z) (0)
#endif

extern int server_confirm;
extern int generated_key_size;
extern int authority_certificate_lifetime;
extern int generated_certificate_lifetime;


static int cmp_certificate_dn(gnutls_x509_crt crt, uint64_t wanted_dn, uint64_t wanted_issuer_dn)
{
        int ret;
        char buf[128];
        size_t size = sizeof(buf);

        ret = gnutls_x509_crt_get_dn_by_oid(crt, GNUTLS_OID_X520_DN_QUALIFIER, 0, 0, buf, &size);
        if ( ret < 0 ) {
                fprintf(stderr, "couldn't get certificate issue dn: %s.\n", gnutls_strerror(ret));
                return -1;
        }

        if ( strtoull(buf, NULL, 10) != wanted_dn )
                return -1;

        size = sizeof(buf);
        ret = gnutls_x509_crt_get_issuer_dn_by_oid(crt, GNUTLS_OID_X520_DN_QUALIFIER, 0, 0, buf, &size);
        if ( ret < 0 ) {
                fprintf(stderr, "couldn't get certificate issue dn: %s.\n", gnutls_strerror(ret));
                return -1;
        }

        if ( strtoull(buf, NULL, 10) != wanted_issuer_dn )
                return -1;

        return 0;
}


static char *const_to_char(const char *ptr)
{
        union {
                char *rw;
                const char *ro;
        } hack;

        hack.ro = ptr;

        return hack.rw;
}



static int remove_old_certificate(const char *filename, uint64_t dn, uint64_t issuer_dn)
{
        int ret;
        char buf[65536];
        unsigned char *data, *datap;
        size_t size;
        FILE *fd;
        gnutls_datum datum;
        gnutls_x509_crt crt;
        prelude_string_t *out;

        ret = _prelude_load_file(filename, &data, &size);
        if ( ret < 0 ) {
                if ( prelude_error_get_code(ret) == PRELUDE_ERROR_ENOENT )
                        return 0;

                fprintf(stderr, "error loading '%s': %s.\n", filename, prelude_strerror(ret));
                return ret;
        }

        ret = prelude_string_new(&out);
        if ( ret < 0 ) {
                _prelude_unload_file(data, size);
                return ret;
        }

        unlink(filename);

        fd = fopen(filename, "w");
        if ( ! fd ) {
                fprintf(stderr, "error opening '%s' for writing: %s.\n", filename, strerror(errno));
                _prelude_unload_file(data, size);
                prelude_string_destroy(out);
                return -1;
        }

        datap = data;
        while ( 1 ) {
                ret = sscanf((const char *) data, "-----BEGIN CERTIFICATE-----\n%65535[^-]%*[^\n]\n", buf);
                if ( ret != 1 ) {
                        ret = 0;
                        break;
                }

                prelude_string_sprintf(out, "-----BEGIN CERTIFICATE-----\n%s-----END CERTIFICATE-----\n", buf);

                datum.size = prelude_string_get_len(out);
                datum.data = (unsigned char *) const_to_char(prelude_string_get_string(out));
                data += datum.size;

                ret = gnutls_x509_crt_init(&crt);
                if ( ret < 0 )
                        break;

                ret = gnutls_x509_crt_import(crt, &datum, GNUTLS_X509_FMT_PEM);
                if ( ret < 0 ) {
                        fprintf(stderr, "error importing certificate: %s\n", gnutls_strerror(ret));
                        break;
                }

                ret = cmp_certificate_dn(crt, dn, issuer_dn);
                if ( ret == 0 )
                        prelude_log_debug(1, "Removing old certificate with subject=%"
                                PRELUDE_PRIu64 " issuer=%" PRELUDE_PRIu64 ".\n", dn, issuer_dn);
                else {
                        if ( fwrite(datum.data, 1, datum.size, fd) != datum.size )
                                fprintf(stderr, "error writing certificate: %s\n", strerror(errno));
                }

                prelude_string_clear(out);
                gnutls_x509_crt_deinit(crt);
        }

        fclose(fd);
        _prelude_unload_file(datap, size);
        prelude_string_destroy(out);

        return ret;
}



static int remove_old(const char *filename, unsigned char *buf, size_t size)
{
        int ret;
        char out[512];
        gnutls_datum data;
        gnutls_x509_crt crt;
        uint64_t dn, issuer_dn;

        data.size = size;
        data.data = buf;

        gnutls_x509_crt_init(&crt);

        ret = gnutls_x509_crt_import(crt, &data, GNUTLS_X509_FMT_PEM);
        if ( ret < 0 ) {
                fprintf(stderr, "error importing certificate: %s.\n", gnutls_strerror(ret));
                goto err;
        }

        size = sizeof(out);
        ret = gnutls_x509_crt_get_issuer_dn_by_oid(crt, GNUTLS_OID_X520_DN_QUALIFIER, 0, 0, out, &size);
        if ( ret < 0 ) {
                fprintf(stderr, "error reading issuer dn: %s.\n", gnutls_strerror(ret));
                goto err;
        }
        issuer_dn = strtoull(out, NULL, 10);

        size = sizeof(out);
        ret = gnutls_x509_crt_get_dn_by_oid(crt, GNUTLS_OID_X520_DN_QUALIFIER, 0, 0, out, &size);
        if ( ret < 0 ) {
                fprintf(stderr, "error reading issuer dn: %s.\n", gnutls_strerror(ret));
                goto err;
        }
        dn = strtoull(out, NULL, 10);

        ret = remove_old_certificate(filename, dn, issuer_dn);
        if ( ret < 0 )
                return ret;

 err:
        gnutls_x509_crt_deinit(crt);
        return ret;
}



static int safe_close(int fd)
{
        int ret;

        do {
                ret = close(fd);

        } while ( ret < 0 && errno == EINTR );

        return ret;
}



static ssize_t safe_write(int fd, const unsigned char *buf, size_t size)
{
        ssize_t ret;

        do {
                ret = write(fd, buf, size);

        } while ( ret < 0 && errno == EINTR );

        return ret;
}




static int save_buf(const char *filename, prelude_uid_t uid, prelude_gid_t gid, const unsigned char *buf, size_t size)
{
        ssize_t sret;
        int fd, ret, flags = 0;

#if !((defined _WIN32 || defined __WIN32__) && !defined __CYGWIN__)
        flags |= S_IRGRP;
#endif

        fd = open(filename, O_CREAT|O_WRONLY|O_APPEND, S_IRUSR|S_IWUSR|flags);
        if ( fd < 0 ) {
                fprintf(stderr, "couldn't open %s for writing: %s.\n", filename, strerror(errno));
                return -1;
        }

        ret = fchown(fd, uid, gid);
        if ( ret < 0 ) {
                fprintf(stderr, "could not set %s owner to UID %d GID %d: %s.\n",
                        filename, (int) uid, (int) gid, strerror(errno));
                safe_close(fd);
                return -1;
        }

        sret = safe_write(fd, buf, size);
        if ( sret < 0 || (size_t) sret != size ) {
                fprintf(stderr, "error writing to %s: %s.\n", filename, strerror(errno));
                safe_close(fd);
                return -1;
        }

        return safe_close(fd);
}



static gnutls_x509_crt generate_certificate(prelude_client_profile_t *cp, gnutls_x509_privkey key, int expire)
{
        int ret;
        char buf[1024];
        gnutls_x509_crt crt;
        uint64_t analyzerid;
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

        analyzerid = prelude_client_profile_get_analyzerid(cp);

        ret = snprintf(buf, sizeof(buf), "%" PRELUDE_PRIu64, analyzerid);
        ret = gnutls_x509_crt_set_dn_by_oid(crt, GNUTLS_OID_X520_DN_QUALIFIER, 0, buf, ret);
        if ( ret < 0 ) {
                fprintf(stderr, "error setting common name: %s.\n", gnutls_strerror(ret));
                return NULL;
        }

        gnutls_x509_crt_set_serial(crt, &analyzerid, sizeof(analyzerid));

        if ( ! key )
                return crt;

        gnutls_x509_crt_set_key(crt, key);

        ret = gnutls_x509_crt_get_key_id(crt, 0, (unsigned char *) buf, &size);
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



static gnutls_x509_crt generate_signed_certificate(prelude_client_profile_t *cp,
                                                   uint64_t analyzerid,
                                                   gnutls_x509_crt ca_crt,
                                                   gnutls_x509_privkey ca_key,
                                                   gnutls_x509_crq crq)
{
        int ret;
        gnutls_x509_crt crt;
        unsigned char buf[65535];
        size_t size = sizeof(buf);

        crt = generate_certificate(cp, NULL, generated_certificate_lifetime);
        if ( ! crt )
                return NULL;

        ret = gnutls_x509_crt_set_crq(crt, crq);
        if ( ret < 0 ) {
                fprintf(stderr, "error associating certificate with CRQ: %s.\n", gnutls_strerror(ret));
                goto err;
        }

        ret = gnutls_x509_crt_set_serial(crt, &analyzerid, sizeof(analyzerid));
        if ( ret < 0 ) {
                fprintf(stderr, "error setting certificate serial: %s.\n", gnutls_strerror(ret));
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




static gnutls_x509_crt generate_ca_certificate(prelude_client_profile_t *cp, gnutls_x509_privkey key)
{
        int ret;
        gnutls_x509_crt crt;
        unsigned int usage = 0;

        crt = generate_certificate(cp, key, authority_certificate_lifetime);
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



static void entropy_progress_cb(void *cb_data, const char *what, int printchar, int current, int total)
{
        //printf("\nwhat='%s' printchar='%c' current='%d' total='%d\n", what, printchar, current, total);
        if ( printchar == '\n')
                printchar = 'O';

        fprintf(stderr, "%c", printchar);
        fflush(stderr);
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

        gcry_set_progress_handler(entropy_progress_cb, NULL);

        fprintf(stderr, "Generating %d bits RSA private key... This might take a very long time.\n", generated_key_size);
        fprintf(stderr, "[Increasing system activity will speed-up the process].\n");
        fprintf(stderr, "Generation in progress... ");
        fflush(stderr);

        ret = gnutls_x509_privkey_generate(key, GNUTLS_PK_RSA, generated_key_size, 0);
        if ( ret < 0 ) {
                fprintf(stderr, "error generating private RSA key: %s\n", gnutls_strerror(ret));
                gnutls_x509_privkey_deinit(key);
                return NULL;
        }


        fprintf(stderr, "\n\n");

        return key;
}



static gnutls_x509_crq generate_certificate_request(prelude_client_profile_t *cp,
                                                    prelude_connection_permission_t permission,
                                                    gnutls_x509_privkey key, unsigned char *buf, size_t *size)
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

        if ( permission ) {
                ret = snprintf((char *) buf, *size, "%d", (int) permission);
                ret = gnutls_x509_crq_set_dn_by_oid(crq, GNUTLS_OID_X520_COMMON_NAME, 0, buf, ret);
                if ( ret < 0 ) {
                        fprintf(stderr, "error setting common name: %s.\n", gnutls_strerror(ret));
                        return NULL;
                }
        }

        gnutls_x509_crq_set_version(crq, 3);

        ret = snprintf((char*) buf, *size, "%" PRELUDE_PRIu64, prelude_client_profile_get_analyzerid(cp));
        ret = gnutls_x509_crq_set_dn_by_oid(crq, GNUTLS_OID_X520_DN_QUALIFIER, 0, buf, ret);
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



static gnutls_x509_privkey gen_crypto(prelude_client_profile_t *cp,
                                      const char *filename, prelude_uid_t uid, prelude_gid_t gid)
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

        ret = save_buf(filename, uid, gid, (unsigned char *) buf, size);
        if ( ret < 0 ) {
                fprintf(stderr, "error saving private key.\n");
                return NULL;
        }

        return key;
}



gnutls_x509_privkey tls_load_privkey(prelude_client_profile_t *cp)
{
        int ret;
        size_t size;
        gnutls_datum data;
        char filename[256];
        gnutls_x509_privkey key;

        prelude_client_profile_get_tls_key_filename(cp, filename, sizeof(filename));

        ret = access(filename, F_OK);
        if ( ret < 0 )
                return gen_crypto(cp, filename,
                                  prelude_client_profile_get_uid(cp),
                                  prelude_client_profile_get_gid(cp));

        ret = _prelude_load_file(filename, &data.data, &size);
        if ( ret < 0 ) {
                fprintf(stderr, "could not load '%s': %s.\n", filename, prelude_strerror(ret));
                return NULL;
        }

        data.size = (unsigned int) size;

        gnutls_x509_privkey_init(&key);
        gnutls_x509_privkey_import(key, &data, GNUTLS_X509_FMT_PEM);

        _prelude_unload_file(data.data, size);

        return key;
}



static char ask_req_confirmation(void)
{
        int ret;
        char c;

        do {
                fprintf(stderr, "Approve registration? [y/n]: ");

                c = 0;
                while ( (ret = getchar()) != '\n' )
                        if ( ! c ) c = ret;

                if ( c == 'y' )
                        return c;

                else if ( c == 'n' )
                        return c;

        }  while ( 1 );
}



static int check_req(const char *srcinfo, prelude_io_t *fd, gnutls_x509_crq crq, uint64_t *analyzerid)
{
        int ret;
        size_t size;
        char buf[128], c;
        prelude_string_t *out;
        prelude_connection_permission_t perms;

        size = sizeof(buf);
        ret = gnutls_x509_crq_get_dn_by_oid(crq, GNUTLS_OID_X520_DN_QUALIFIER, 0, 0, buf, &size);
        if ( ret < 0 ) {
                fprintf(stderr, "could not retrieve dn qualifier: %s.\n", gnutls_strerror(ret));
                return -1;
        }
        *analyzerid = strtoull(buf, NULL, 10);

        size = sizeof(buf);
        ret = gnutls_x509_crq_get_dn_by_oid(crq, GNUTLS_OID_X520_COMMON_NAME, 0, 0, buf, &size);
        if ( ret < 0 ) {
                fprintf(stderr, "could not retrieve common name: %s.\n", gnutls_strerror(ret));
                return -1;
        }
        perms = atoi(buf);

        ret = prelude_string_new(&out);
        if ( ret < 0 ) {
                prelude_perror(ret, "error creating prelude-string");
                return -1;
        }

        ret = prelude_connection_permission_to_string(perms, out);
        if ( ret < 0 ) {
                prelude_perror(ret, "could not convert permission to string");
                return -1;
        }

        fprintf(stderr, "Registration request for analyzerID=\"%" PRELUDE_PRIu64 "\" permission=\"%s\".\n",
                *analyzerid, prelude_string_get_string(out));


        if ( server_confirm ) {
                c = ask_req_confirmation();
                if ( c != 'y' ) {
                        gnutls_alert_send(prelude_io_get_fdptr(fd), GNUTLS_AL_FATAL, GNUTLS_A_USER_CANCELED);
                        return -1;
                }
        }

        prelude_string_destroy(out);

        return 0;
}



int tls_handle_certificate_request(const char *srcinfo, prelude_client_profile_t *cp, prelude_io_t *fd,
                                   gnutls_x509_privkey cakey, gnutls_x509_crt cacrt,
                                   gnutls_x509_crt crt)
{
        ssize_t ret;
        size_t size;
        char buf[65535];
        gnutls_datum data;
        gnutls_x509_crq crq;
        unsigned char *rbuf;
        uint64_t analyzerid;
        gnutls_x509_crt gencrt;

        /*
         * Read the client CRQ and generate a certificate for it.
         */
        prelude_log_debug(1, "Waiting for client certificate request.\n");
        ret = prelude_io_read_delimited(fd, &rbuf);
        if ( ret < 0 ) {
                prelude_perror(ret, "error receiving client certificate request");
                return -1;
        }

        data.size = ret;
        data.data = rbuf;
        gnutls_x509_crq_init(&crq);
        gnutls_x509_crq_import(crq, &data, GNUTLS_X509_FMT_PEM);
        free(rbuf);

        ret = check_req(srcinfo, fd, crq, &analyzerid);
        if ( ret < 0 )
                return ret;

        /*
         * Generate a CA signed certificate for this CRQ.
         */
        prelude_log_debug(1, "Generating signed certificate for client.\n");

        gencrt = generate_signed_certificate(cp, analyzerid, cacrt, cakey, crq);
        if ( ! gencrt ) {
                fprintf(stderr, "error generating signed certificate for this request.\n");
                return -1;
        }
        gnutls_x509_crq_deinit(crq);

        size = sizeof(buf);
        gnutls_x509_crt_export(gencrt, GNUTLS_X509_FMT_PEM, buf, &size);

        ret = prelude_io_write_delimited(fd, buf, size);
        if ( ret < 0 || (size_t) ret != size ) {
                prelude_perror(ret, "error sending signed certificate");
                return -1;
        }

        gnutls_x509_crt_deinit(gencrt);

        /*
         * write our own certificate back to the client.
         */
        prelude_log_debug(1, "Sending server certificate to client.\n");

        size = sizeof(buf);
        gnutls_x509_crt_export(crt, GNUTLS_X509_FMT_PEM, buf, &size);

        ret = prelude_io_write_delimited(fd, buf, size);
        if ( ret < 0 || (size_t) ret != size ) {
                prelude_perror(ret, "error sending signed certificate");
                return -1;
        }

        return 0;
}




int tls_request_certificate(prelude_client_profile_t *cp, prelude_io_t *fd,
                            gnutls_x509_privkey key, prelude_connection_permission_t permission)
{
        ssize_t ret;
        ssize_t rsize;
        char buf[65535];
        unsigned char *rbuf;
        gnutls_x509_crq crq;
        size_t size = sizeof(buf);

        prelude_log_debug(1, "Sending certificate request.\n");

        crq = generate_certificate_request(cp, permission, key, (unsigned char *) buf, &size);
        if ( ! crq )
                return -1;

        gnutls_x509_crq_deinit(crq);

        ret = prelude_io_write_delimited(fd, buf, size);
        if ( ret < 0 || (size_t) ret != size ) {
                prelude_perror(ret, "error sending certificate request");
                return -1;
        }

        prelude_log_debug(1, "Receiving signed certificate.\n");
        rsize = prelude_io_read_delimited(fd, &rbuf);
        if ( rsize < 0 ) {
                prelude_perror(rsize, "error receiving CA-signed certificate");
                return -1;
        }

        prelude_client_profile_get_tls_client_keycert_filename(cp, buf, sizeof(buf));
        ret = remove_old(buf, rbuf, rsize);
        if ( ret < 0 )
                return ret;

        ret = save_buf(buf, prelude_client_profile_get_uid(cp), prelude_client_profile_get_gid(cp), rbuf, rsize);
        if ( ret < 0 )
                return ret;

        free(rbuf);

        prelude_log_debug(1, "Receiving CA certificate.\n");

        rsize = prelude_io_read_delimited(fd, &rbuf);
        if ( rsize < 0 ) {
                prelude_perror(rsize, "error receiving server certificate");
                return rsize;
        }

        prelude_client_profile_get_tls_client_trusted_cert_filename(cp, buf, sizeof(buf));
        ret = remove_old(buf, rbuf, rsize);
        if ( ret < 0 )
                return ret;

        ret = save_buf(buf, prelude_client_profile_get_uid(cp), prelude_client_profile_get_gid(cp), rbuf, rsize);
        if ( ret < 0 )
                return ret;

        free(rbuf);

        return 0;
}


static int crt_import(gnutls_x509_crt *crt, const char *filename)
{
        int ret;
        size_t dsize;
        gnutls_datum data;

        ret = _prelude_load_file(filename, &data.data, &dsize);
        if ( ret < 0 ) {
                fprintf(stderr, "error loading '%s': %s.\n", filename, prelude_strerror(ret));
                return ret;
        }

        data.size = (unsigned int) dsize;
        gnutls_x509_crt_init(crt);

        ret = gnutls_x509_crt_import(*crt, &data, GNUTLS_X509_FMT_PEM);
        if ( ret < 0 )
                fprintf(stderr, "error importing certificate: %s.\n", gnutls_strerror(ret));

        _prelude_unload_file(data.data, dsize);
        return ret;
}



static int crt_export(prelude_client_profile_t *cp, gnutls_x509_crt *crt, const char *filename)
{
        int ret;
        size_t size;
        unsigned char buf[65535];

        size = sizeof(buf);

        ret = gnutls_x509_crt_export(*crt, GNUTLS_X509_FMT_PEM, buf, &size);
        if ( ret < 0 ) {
                fprintf(stderr, "error exporting self-signed certificate: %s.\n", gnutls_strerror(ret));
                gnutls_x509_crt_deinit(*crt);
                return -1;
        }

        ret = save_buf(filename, prelude_client_profile_get_uid(cp), prelude_client_profile_get_gid(cp), buf, size);
        if ( ret < 0 ) {
                fprintf(stderr, "error saving private key certificate.\n");
                gnutls_x509_crt_deinit(*crt);
                return -1;
        }

        return 0;
}


int tls_load_ca_certificate(prelude_client_profile_t *cp, gnutls_x509_privkey key, gnutls_x509_crt *crt)
{
        int ret;
        char filename[256];

        prelude_client_profile_get_tls_server_ca_cert_filename(cp, filename, sizeof(filename));

        ret = access(filename, F_OK);
        if ( ret == 0 )
                return crt_import(crt, filename);

        *crt = generate_ca_certificate(cp, key);
        if ( ! *crt )
                return -1;

        ret = crt_export(cp, crt, filename);
        if ( ret < 0 )
                gnutls_x509_crt_deinit(*crt);

        return ret;
}



int tls_load_ca_signed_certificate(prelude_client_profile_t *cp,
                                   gnutls_x509_privkey cakey,
                                   gnutls_x509_crt cacrt,
                                   gnutls_x509_crt *crt)
{
        int ret;
        char filename[256];
        gnutls_x509_crq crq;
        unsigned char buf[65535];
        size_t size = sizeof(buf);

        prelude_client_profile_get_tls_server_keycert_filename(cp, filename, sizeof(filename));

        ret = access(filename, F_OK);
        if ( ret == 0 )
                return crt_import(crt, filename);

        crq = generate_certificate_request(cp, 0, cakey, buf, &size);
        if ( ! crq )
                return -1;

        *crt = generate_signed_certificate(cp, prelude_client_profile_get_analyzerid(cp), cacrt, cakey, crq);
        if ( ! *crt )
                return -1;

        ret = crt_export(cp, crt, filename);
        if ( ret < 0 )
                gnutls_x509_crt_deinit(*crt);

        return ret;
}



int tls_revoke_analyzer(prelude_client_profile_t *cp, gnutls_x509_privkey key,
                        gnutls_x509_crt crt, uint64_t revoked_analyzerid)
{
        int ret, i;
        size_t len, dsize;
        gnutls_datum data;
        gnutls_x509_crl crl;
        uint64_t analyzerid;
        char crlfile[PATH_MAX], buf[65535];

        ret = gnutls_x509_crl_init(&crl);
        prelude_client_profile_get_tls_server_crl_filename(cp, crlfile, sizeof(crlfile));

        ret = access(crlfile, R_OK);
        if ( ret == 0 ) {
                ret = _prelude_load_file(crlfile, &data.data, &dsize);
                if ( ret < 0 ) {
                        fprintf(stderr, "error loading '%s': %s.\n", crlfile, prelude_strerror(ret));
                        return ret;
                }

                data.size = (unsigned int) dsize;

                ret = gnutls_x509_crl_import(crl, &data, GNUTLS_X509_FMT_PEM);
                if ( ret < 0 ) {
                        fprintf(stderr, "error importing CRL: %s.\n", gnutls_strerror(ret));
                        return -1;
                }
        }

        for ( i = 0; i < gnutls_x509_crl_get_crt_count(crl); i++ ) {
                len = sizeof(analyzerid);
                gnutls_x509_crl_get_crt_serial(crl, i, (unsigned char *) &analyzerid, &len, NULL);

                if ( analyzerid == revoked_analyzerid )
                        return 0;
        }

        ret = gnutls_x509_crl_set_crt_serial(crl, &revoked_analyzerid, sizeof(revoked_analyzerid), time(NULL));
        if ( ret < 0 ) {
                fprintf(stderr, "error setting CRL certificate serial: %s.\n", gnutls_strerror(ret));
                return -1;
        }

        gnutls_x509_crl_set_version(crl, 2);
        gnutls_x509_crl_set_next_update(crl, time(NULL));
        gnutls_x509_crl_set_this_update(crl, time(NULL));

        ret = gnutls_x509_crl_sign(crl, crt, key);
        if ( ret < 0 ) {
                fprintf(stderr, "error signing CRL: %s.\n", gnutls_strerror(ret));
                return -1;
        }

        len = sizeof(buf);
        ret = gnutls_x509_crl_export(crl, GNUTLS_X509_FMT_PEM, buf, &len);
        if ( ret < 0 ) {
                fprintf(stderr, "error exporting certificate revocation list: %s\n", gnutls_strerror(ret));
                gnutls_x509_crl_deinit(crl);
                return -1;
        }

        gnutls_x509_crl_deinit(crl);

        unlink(crlfile);
        ret = save_buf(crlfile, prelude_client_profile_get_uid(cp),
                       prelude_client_profile_get_gid(cp), (unsigned char *) buf, len);
        if ( ret < 0 ) {
                fprintf(stderr, "error saving private key.\n");
                return -1;
        }

        return 1;
}
