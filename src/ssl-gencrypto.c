#include "config.h"

#ifdef HAVE_SSL

/*****
*
* Copyright (C) 2001 Jeremie Brebec / Toussaint Mathieu
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

#include "ssl-gencrypto.h"


static int req_check_len(int len, int min, int max)
{

	if (len < min) {
		fprintf(stderr,
			"string is too short, it needs to be at least %d bytes long\n",
			min);
		return 0;
	}

	if ((max != 0) && (len > max)) {
		fprintf(stderr,
			"string is too long, it needs to be less than  %d bytes long\n",
			max);
		return 0;
	}

	return (1);
}



static int get_full_hostname(char *str, size_t len) 
{
        int ret, i;
        char buf[1024];
        
        ret = gethostname(buf, sizeof(buf));
        if ( ret < 0 ) {
                fprintf(stderr, "couldn't get system hostname.\n");
                return -1;
        }
        
        i = snprintf(str, len, "%s", buf);

        ret = getdomainname(buf, sizeof(buf));
        if ( ret < 0 ) {
                fprintf(stderr, "couldn't get system domainname.\n");
                return -1;
        }
        
        snprintf(str + i, len - i, "%c%s",
                 (buf[0] != '\0') ? '.' : ' ', buf);

        return 0;
}




static int add_DN_object(X509_NAME *n, char *text, int nid, int min, int max)
{

        int i, ret;
        char buf[1024];
        char def[1024], *ptr;

        def[0] = '\0';
        ret = get_full_hostname(def, sizeof(def));
        if ( ! buf )
                return -1;
        
 start:
	fprintf(stderr, "%s [%s]: ", text, def);
        
        buf[0] = '\0';
        fgets(buf, 1024, stdin);
        
	if (buf[0] == '\n')
                ptr = def;
        else
                ptr = buf;
        
        i = strlen(ptr);
        if ( buf[i - 1] == '\n' )
                buf[--i] = '\0';
        
        /*
         * If Prelude Report get two Prelude certificate with the same
         * name, it will act like if theses two certificate doesn't
         * exist... This is why we use rand() to prevent this.
         */
        snprintf(ptr + i, sizeof(buf) - i, " - %d", rand());
        
	if (!req_check_len(i, min, max))
		goto start;

	if (!X509_NAME_add_entry_by_NID(n, nid, MBSTRING_ASC,
                                        (unsigned char *) ptr, -1, -1, 0))
		return -1;

	return 0;
}



static int prompt_info(X509_REQ * req)
{

	int nid, min, max;
	CONF_VALUE v;
	X509_NAME *subj;

	subj = X509_REQ_get_subject_name(req);

	fprintf(stderr,
		"You will be asked a name that will be enclosed in your certificate\n");
	fprintf(stderr, "If you type enter, a default value will be used.\n");

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

	char c = '*';

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

	BIO_write((BIO *) arg, &c, 1);
	(void) BIO_flush((BIO *) arg);
}



static EVP_PKEY *ssl_gen_privkey(int len, BIO * bio_err)
{
	EVP_PKEY *pkey;

	if ((pkey = EVP_PKEY_new()) == NULL)
		return NULL;;

	if (!EVP_PKEY_assign_RSA
	    (pkey, RSA_generate_key(len, 0x10001, req_cb, bio_err)))
		return NULL;

	return pkey;
}




static X509 *ssl_gen_sscert(EVP_PKEY * pkey, const EVP_MD * digest, int days)
{
	X509_REQ *req;
	EVP_PKEY *tmppkey;
	X509V3_CTX ext_ctx;
	X509 *x509ss;

	req = X509_REQ_new();
	if (req == NULL)
		return NULL;

	if (!make_REQ(req, pkey)) {
		X509_REQ_free(req);
		return NULL;
	}

	if ((x509ss = X509_new()) == NULL) {
		X509_REQ_free(req);
		return NULL;
	};

	/*
         * Set version to V3
         */
	if (!X509_set_version(x509ss, 2)) {
		X509_REQ_free(req);
		X509_free(x509ss);
		return NULL;
	}

	ASN1_INTEGER_set(X509_get_serialNumber(x509ss), 0L);
	X509_set_issuer_name(x509ss, X509_REQ_get_subject_name(req));
	X509_gmtime_adj(X509_get_notBefore(x509ss), 0);
	X509_gmtime_adj(X509_get_notAfter(x509ss),
			(long) 60 * 60 * 24 * days);
	X509_set_subject_name(x509ss, X509_REQ_get_subject_name(req));
	tmppkey = X509_REQ_get_pubkey(req);
	X509_set_pubkey(x509ss, tmppkey);
	EVP_PKEY_free(tmppkey);

	/*
         * Set up V3 context struct
         */
        
	X509V3_set_ctx(&ext_ctx, x509ss, x509ss, NULL, NULL, 0);

	if (!X509_sign(x509ss, pkey, digest)) {
		X509_free(x509ss);
		return NULL;
	}

	X509_REQ_free(req);

	return x509ss;
}



X509 *prelude_ssl_gen_crypto(int keysize, int expire, const char *keyout, int crypt)
{
	int i;
        mode_t old_mask;
	X509 *x509ss = NULL;
	EVP_PKEY *pkey = NULL;
	BIO *out = NULL, *bio_err = NULL;
	EVP_CIPHER *cipher = NULL;
	const EVP_MD *digest = EVP_md5();

        if ( crypt )
                cipher = EVP_des_ede3_cbc();

        /*
         * FIXME: What is that ?
         * Don't it cause exportation issue ?
         */
	if ( keysize < DEFAULT_KEY_LENGTH )
                keysize = DEFAULT_KEY_LENGTH;

	if ((bio_err = BIO_new(BIO_s_file())) != NULL)
		BIO_set_fp(bio_err, stderr, BIO_NOCLOSE | BIO_FP_TEXT);

	out = BIO_new(BIO_s_file());
	if ((out == NULL) || bio_err == NULL) {
		BIO_free_all(out);
		return NULL;
	}
        
	BIO_printf(bio_err, "Generating a %d bit RSA private key\n", keysize);
	pkey = ssl_gen_privkey(keysize, bio_err);
	if (pkey == NULL) {
		ERR_print_errors(bio_err);
		BIO_printf(bio_err, "problems making RSA private key\n");
		BIO_free_all(out);
		return NULL;
	}

        /*
         * FIXME : BIO API probably have a way to set
         * permission... don't do a chmod() to avoid a race
         * (see ChangeLog of 2001-03-30).
         */
        old_mask = umask(S_IRWXG|S_IRWXO);
        
	BIO_printf(bio_err, "Writing new private key to '%s'\n", keyout);
	if (BIO_write_filename(out, keyout) <= 0) {
		perror(keyout);
		BIO_free_all(out);
		EVP_PKEY_free(pkey);
                umask(old_mask);
		return NULL;
	}
        umask(old_mask);

	i = 0;

        while ( ! PEM_write_bio_PrivateKey(out, pkey, cipher, NULL, 0, NULL, NULL) ) {
                if ((ERR_GET_REASON(ERR_peek_error()) ==
                     PEM_R_PROBLEMS_GETTING_PASSWORD) && (i < 3)) {
			ERR_clear_error();
			i++;
                        continue;
		}

                ERR_print_errors(bio_err);
		BIO_printf(bio_err, "problems making RSA private key\n");
		BIO_free_all(out);
		EVP_PKEY_free(pkey);

                return NULL;
        }
        
	BIO_printf(bio_err, "-----\n");

	//-----------------------------------------------------
	x509ss = ssl_gen_sscert(pkey, digest, expire);
	if (x509ss == NULL) {
		ERR_print_errors(bio_err);
		BIO_printf(bio_err, "problems making self signed Certificate\n");
		BIO_free_all(out);
		EVP_PKEY_free(pkey);
		return NULL;
	}

	BIO_printf(bio_err, "Adding self signed Certificate to '%s'\n", keyout);
	//------------------------------------------------
	if (!(int) BIO_append_filename(out, keyout)) {
		perror(keyout);
		BIO_free_all(out);
		EVP_PKEY_free(pkey);
		X509_free(x509ss);
		return NULL;
	}

	if (!PEM_write_bio_X509(out, x509ss)) {
		ERR_print_errors(bio_err);
		BIO_printf(bio_err, "unable to write X509 certificate\n");
		BIO_free_all(out);
		EVP_PKEY_free(pkey);
		X509_free(x509ss);
		return NULL;
	}

	BIO_free_all(out);
	EVP_PKEY_free(pkey);
	OBJ_cleanup();

	return x509ss;
}




static void ask_keysize(int *keysize) 
{
        char buf[10];
        
        fprintf(stderr, "\n\nWhat keysize do you want [1024] ? ");
        fgets(buf, sizeof(buf), stdin);
        *keysize = ( *buf == '\n' ) ? 1024 : atoi(buf);
}




static void ask_expiration_time(int *expire) 
{
        char buf[10];
        
        fprintf(stderr,
                "\n\nPlease specify how long the key should be valid.\n"
                "\t0    = key does not expire\n"
                "\t<n>  = key expires in n days\n");

        fprintf(stderr, "\nKey is valid for [0] : ");
        fgets(buf, sizeof(buf), stdin);
        *expire = ( *buf == '\n' ) ? 0 : atoi(buf);
}



static void ask_key_storage(int *crypted) 
{
        char buf[10];
        
        fprintf(stderr, "\n\nThe private key can be stored encrypted, but this will\n"
                "require to enter a password each time the sensor start.\n\n"
                "Should the private key be stored encrypted ? [yes] : ");

        fgets(buf, sizeof(buf), stdin);
        buf[strlen(buf) - 1] = '\0';
        
        *crypted = ( strcmp(buf, "no") == 0 ) ? 0 : 1;
}



void prelude_ssl_ask_settings(int *keysize, int *expire, int *crypted) 
{
        ask_keysize(keysize);
        ask_expiration_time(expire);
        ask_key_storage(crypted);    
}


#endif
