/*****
*
* Copyright (C) 2001, 2002 Jeremie Brebec / Toussaint Mathieu
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
#include <time.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <fcntl.h>
#include <time.h>
#include <inttypes.h>

#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/conf.h>
#include <openssl/err.h>
#include <openssl/asn1.h>
#include <openssl/x509.h>
#include <openssl/x509v3.h>
#include <openssl/objects.h>
#include <openssl/pem.h>
#include <openssl/e_os.h>

#include "prelude-log.h"
#include "prelude-path.h"
#include "prelude-io.h"
#include "prelude-message.h"
#include "prelude-client.h"
#include "ssl-gencrypto.h"



static int req_check_len(int len, int min, int max)
{

	if ( len < min ) {
                log(LOG_ERR, "string is too short, it needs to be at least %d bytes long\n", min);
		return -1;
	}

	if ( (max != 0) && (len > max) ) {
		log(LOG_ERR, "string is too long, it needs to be less than  %d bytes long\n", max);
		return -1;
	}

	return 0;
}



static int get_full_hostname(char *buf, size_t len) 
{
        int ret, i;
        
        ret = gethostname(buf, len);
        if ( ret < 0 ) {
                fprintf(stderr, "couldn't get system hostname.\n");
                return -1;
        }

        i = strlen(buf);
        buf[i++] = '.';
        
        ret = getdomainname(buf + i, len - i);
        if ( ret < 0 ) {
                fprintf(stderr, "couldn't get system domainname.\n");
                return -1;
        }

        return 0;
}




static int add_DN_object(X509_NAME *name, char *text, int nid, int min, int max)
{
        int ret;
        struct timeval tv;
        X509_NAME_ENTRY *entry;
        char buf[1024], host[256];

        get_full_hostname(host, sizeof(host));
        
        gettimeofday(&tv, NULL);
        srand(getpid() * tv.tv_usec);

        ret = snprintf(buf, sizeof(buf), "%s:%s:%llu:%d", host,
                       prelude_get_sensor_name(),
                       prelude_client_get_analyzerid(), rand());
        
	if ( req_check_len(ret, min, max) < 0)
		return -1;
        
	entry = X509_NAME_ENTRY_create_by_NID(NULL, nid, V_ASN1_APP_CHOOSE,
                                              (unsigned char *)buf, -1);
        
	ret = X509_NAME_add_entry(name, entry, 0, 0);
	if ( ! ret )
		return -1;
        
        X509_NAME_ENTRY_free(entry);

	return 0;
}



static int prompt_info(X509_REQ * req)
{

	int nid, min, max;
	CONF_VALUE v;
	X509_NAME *subj;

	subj = X509_REQ_get_subject_name(req);

	v.name = "CN";
	v.value = "Name";
	min = 5;
	max = 100;
	nid = OBJ_txt2nid(v.name);

        if ( add_DN_object(subj, v.value, nid, min, max) < 0 )
		return 0;
        
	/*
         * we need one DN at least
         */
	if (X509_NAME_entry_count(subj) == 0)
		return 0;

	return 1;

}




static int make_REQ(X509_REQ * req, EVP_PKEY * pkey)
{
	/*
         * setup version number
         */
	if (!X509_REQ_set_version(req, 0L))
		return 0;

	/*
         * ask user for DN
         */
	if (!prompt_info(req))
		return 0;

	X509_REQ_set_pubkey(req, pkey);

	return 1;
}




static void MS_CALLBACK req_cb(int p, int n, void *arg)
{

	char c = 'B';

	switch (p) {
	case 0:
		c = '.';
		break;
	case 1:
		c = '+';
		break;
	case 2:
		c = '*';
		break;
	case 3:
		c = '\n';
		break;
	}

        fputc(c, stderr);
}



static EVP_PKEY *generate_private_key(int len)
{
        int ret;
        RSA *rsa;
        EVP_PKEY *pkey;
        
        pkey = EVP_PKEY_new();
        if ( ! pkey )
		return NULL;

        rsa = RSA_generate_key(len, RSA_F4, req_cb, NULL);
        if ( ! rsa ) {
                EVP_PKEY_free(pkey);
                return NULL;
        }

        ret = EVP_PKEY_assign_RSA(pkey, rsa);
        if ( ! ret ) {
                RSA_free(rsa);
                EVP_PKEY_free(pkey);
                return NULL;
        }

	return pkey;
}




static X509 *generate_self_signed_certificate(EVP_PKEY *pkey, int days)
{
	X509 *x509ss;
	X509_REQ *req;
	X509V3_CTX ext_ctx;
        
        x509ss = X509_new();
        if ( ! x509ss )
                return NULL;

	req = X509_REQ_new();
	if ( ! req ) {
                X509_free(x509ss);
                return NULL;
        }
        
	if ( ! make_REQ(req, pkey) ) {
		X509_REQ_free(req);
                return NULL;
	}

        
        X509_set_issuer_name(x509ss, X509_REQ_get_subject_name(req));
        X509_set_subject_name(x509ss, X509_REQ_get_subject_name(req));


	/*
         * Set version to V3
         */
        X509_set_version(x509ss, 3);
	ASN1_INTEGER_set(X509_get_serialNumber(x509ss), 0);
        X509_gmtime_adj(X509_get_notBefore(x509ss), 0);
	X509_gmtime_adj(X509_get_notAfter(x509ss), (long) 60 * 60 * 24 * days);
        X509_set_pubkey(x509ss, pkey);
      
	/*
         * Set up V3 context struct
         */
	X509V3_set_ctx(&ext_ctx, x509ss, x509ss, NULL, NULL, 0);

	if ( ! X509_sign(x509ss, pkey, EVP_md5()) ) {
		X509_free(x509ss);
		return NULL;
	}

	X509_REQ_free(req);

	return x509ss;
}





static void check_key_size(int keysize) 
{
        /*
         * RSA key < 1024 bits are weak. We can't override the provided size
         * due to exportation issue. We'll warn the user at least.
         */
	if ( keysize < 1024 ) {
                fprintf(stderr,
                        "\n\nYou requested the creation of a key with size being %d.\n"
                        "We have to warn you that RSA key with size < 1024 should be considered insecure.\n\n",
                        keysize);
        }
}





/**
 * prelude_ssl_gen_crypto:
 * @keysize: Size of the RSA key to create.
 * @expire: Number of day this key will expire in.
 * @keyout: Place where the create key should be stored.
 * @crypt: Whether the key should be crypted or not.
 * @uid: UID of authentication file owner.
 *
 * Create a new private key of @keysize bits, expiring in @expire days,
 * along with a self signed certificate for this key. Both the key and
 * the certificate are saved in @keyout.
 *
 * If @crypt is true, the key will be stored encrypted.
 *
 * Returns: 0 on success, or -1 if an error occured.
 */
int prelude_ssl_gen_crypto(int keysize, int expire, const char *keyout, int crypt, uid_t uid)
{
        FILE *fdp;
	int ret, fd;
	X509 *x509ss = NULL;
	EVP_PKEY *pkey = NULL;
	EVP_CIPHER *cipher = NULL;

        if ( crypt )
                cipher = EVP_des_ede3_cbc();

        if ( ! expire )
                /*
                 * does SSL allow never expiring key ?
                 */
                expire = 10950;
        
        check_key_size(keysize);

        /*
         * Create the private key.
         */
        fprintf(stderr, "Generating a %d bit RSA private key...\n", keysize);
        
	pkey = generate_private_key(keysize);
	if ( ! pkey ) {
                fprintf(stderr, "Problem generating RSA private key.\n");
		return -1;
	}
        
        x509ss = generate_self_signed_certificate(pkey, expire);
	if ( ! x509ss ) {
		ERR_print_errors_fp(stderr);
                fprintf(stderr, "problems making self signed Certificate.\n");
		EVP_PKEY_free(pkey);
                return -1;
        }
        
        /*
         * write it to the keyout file.
         */
        fprintf(stderr, "Writing new private key to '%s'.\n", keyout);

        fd = open(keyout, O_CREAT|O_TRUNC|O_RDWR, S_IRUSR|S_IWUSR);
        if ( fd < 0 ) {
                fprintf(stderr, "couldn't open %s for writing.\n", keyout);
                ret = -1; goto ferr;
        }

        fdp = fdopen(fd, "w");
        if ( ! fdp ) {
                fprintf(stderr, "couldn't open %s for writing.\n", keyout);
                close(fd);
                ret = -1; goto ferr;
        }

        ret = fchown(fd, uid, -1);
        if ( ret < 0 ) {
                fprintf(stderr, "couldn't change owner pf %s to UID %d.\n", keyout, uid);
                goto err;
        }

        ret = PEM_write_PrivateKey(fdp, pkey, cipher, NULL, 0, NULL, NULL);
        if ( ! ret ) {
                ERR_print_errors_fp(stderr);
                fprintf(stderr, "couldn't write private key to %s.\n", keyout);
                goto err;
        }

        fprintf(stderr, "Adding self signed Certificate to '%s'\n", keyout);
        
        ret = PEM_write_X509(fdp, x509ss);
        if ( ! ret ) {
                ERR_print_errors_fp(stderr);
		fprintf(stderr, "unable to write X509 certificate.\n");
        }

  err:
        fclose(fdp);
        close(fd);
        
  ferr:
        X509_free(x509ss);
	EVP_PKEY_free(pkey);
	OBJ_cleanup();

	return ret;
}


