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
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <openssl/des.h>
#include <openssl/bio.h>
#include <openssl/x509.h>
#include <openssl/pem.h>
#include <openssl/buffer.h>
#include <openssl/sha.h>
#include <inttypes.h>

#include "prelude-io.h"
#include "ssl-registration-msg.h"
#include "prelude-log.h"



X509 *load_x509(const char *certfilename)
{

	BIO *certin;
	X509 *x509;

	certin = BIO_new_file(certfilename, "r");
	if (certin == NULL)
		return NULL;

	if ((x509 = PEM_read_bio_X509(certin, NULL, NULL, NULL)) == NULL)
		return NULL;

	BIO_free(certin);

	return x509;
}



int x509_to_msg(X509 * x509, char *msg, int msglen,
		des_key_schedule * key1, des_key_schedule * key2)
{

	BIO *mem;
	BUF_MEM *mycert;

	mem = BIO_new(BIO_s_mem());
	if (mem == NULL)
		return -1;

	if (!PEM_write_bio_X509(mem, x509))
		return -1;

	BIO_get_mem_ptr(mem, &mycert);
	msglen = build_install_msg(mycert, msg, msglen, key1, key2);
	if (msglen < 0)
		return -2;

	BIO_free(mem);

	return msglen;
}




int des_generate_2key(des_key_schedule * key1, des_key_schedule * key2,
		      int verify)
{
	int res;
	des_cblock prekey1;
	des_cblock prekey2;

	res = des_read_2passwords(&prekey1, &prekey2,
                                  "Enter registration one shot password :",
                                  verify);
	if (res != SUCCESS) {
		memset(&prekey1, 5, sizeof(des_cblock));
		memset(&prekey2, 5, sizeof(des_cblock));
		return res;
	}

	res = des_set_key_checked(&prekey1, *key1);
	memset(&prekey1, 5, sizeof(des_cblock));
	if (res != SUCCESS) {
		memset(&prekey2, 5, sizeof(des_cblock));
		return res;
	}

	res = des_set_key_checked(&prekey2, *key2);
	memset(&prekey2, 5, sizeof(des_cblock));
	if (res != SUCCESS)
		return res;

	return SUCCESS;
}




int build_install_msg(BUF_MEM * input, char *output, int outputlen,
		      des_key_schedule * key1, des_key_schedule * key2)
{
	des_cblock ivec;
	char head[HEADLENGTH] = HEAD;
	int len;
	int pad;

	len = SHA_DIGEST_LENGTH + HEADLENGTH + input->length - 1;
	pad = len % sizeof(des_cblock);
	if (pad != 0) {
		pad = sizeof(des_cblock) - pad;
		len += pad;
	}
	len += 1;

	if (pad != 0) {
		int i;
		for (i = 0; i < pad; i++)
			output[SHA_DIGEST_LENGTH + i] = 64 + pad;
	}

	*(output + pad + SHA_DIGEST_LENGTH) = '\0';
	
	/*
         * message is : |SHA digest (SHA_DIGEST_LENGTH)|padding|HEAD (HEADLENGTH)| txt|
         */
	strcat(output + pad + SHA_DIGEST_LENGTH, head);
	strncat(output + pad + HEADLENGTH + SHA_DIGEST_LENGTH - 1,
		input->data, input->length);

	--len;
	/*
         * generate SHA digest of head + txt and add it to the message
         */
	SHA1((unsigned char *)output + SHA_DIGEST_LENGTH,
             len - SHA_DIGEST_LENGTH, (unsigned char *) output);

        /*
         * set Initialization Vector and encrypt the message
         */
	memset(&ivec, 1, sizeof(des_cblock));

	des_ede3_cbc_encrypt((unsigned char *)output, (unsigned char *)output,
                             outputlen, *key1, *key2, *key1, &ivec, DES_ENCRYPT);

	return len;
}



/*
 * decrypt, verify the structure of the message, and extract its content
 */
int analyse_install_msg(char *input, int inputlen, char *output,
			int outputlen, des_key_schedule * key1,
			des_key_schedule * key2)
{
	des_cblock ivec;
	char head[HEADLENGTH] = HEAD;
	char hash[SHA_DIGEST_LENGTH + 1];
	int len;
	char pad;

	if (inputlen % sizeof(des_cblock) != 0) {
                log(LOG_ERR, "packet should only contain DES blocks.\n");
		return WRONG_SIZE;
        }

	memset(&ivec, 1, sizeof(des_cblock));
	des_ede3_cbc_encrypt((unsigned char *)input, (unsigned char *)input,
                             inputlen, *key1, *key2, *key1, &ivec, DES_DECRYPT);

	pad = input[SHA_DIGEST_LENGTH] - 64;
	if ((pad <= 0) || (pad >= sizeof(des_cblock)))
		pad = 0;
	len = inputlen - SHA_DIGEST_LENGTH - HEADLENGTH - pad + 1;

	SHA1((unsigned char *)input + SHA_DIGEST_LENGTH,
             inputlen - SHA_DIGEST_LENGTH, (unsigned char *) hash);

	if (len < 0 || len > outputlen) {
                log(LOG_ERR, "len %d is wrong.\n", len);
		return WRONG_SIZE;
        }

	strncpy(output, input + SHA_DIGEST_LENGTH + pad + HEADLENGTH - 1,
		len);

	*(input + SHA_DIGEST_LENGTH + pad + HEADLENGTH - 1) = '\0';
        
	if (strcmp(input + SHA_DIGEST_LENGTH + pad, head) != 0) {
                log(LOG_ERR, "packet is not an install message.\n");
		return NOT_INSTALL_MSG;
        }

	hash[SHA_DIGEST_LENGTH] = '\0';
	*(input + SHA_DIGEST_LENGTH) = '\0';

        if (strcmp(input, hash) != 0) {
                log(LOG_ERR, "install message corrupted.\n");
		return INSTALL_MSG_CORRUPTED;
        }

	return len;
}




int prelude_ssl_recv_cert(prelude_io_t *pio, char *out, int outlen,
                          des_key_schedule *skey1, des_key_schedule *skey2) 
{
        int len, certlen;       
        unsigned char *buf;
        
        len = prelude_io_read_delimited(pio, (void **) &buf);
        if ( len <= 0 ) {
                fprintf(stderr, "couldn't receive certificate.\n");
                return -1;
        }
        
	certlen = analyse_install_msg(buf, len, out, outlen, skey1, skey2);
	if ( certlen < 0 ) {
		fprintf(stderr, "Bad message received - Registration failed.\n");
                return -1;
	}

        return certlen;
}




int prelude_ssl_send_cert(prelude_io_t *pio, const char *filename,
                          des_key_schedule *skey1, des_key_schedule *skey2) 
{
        int len;
        X509 *x509ss;
        char buf[BUFMAXSIZE];
        
        x509ss = load_x509(filename);
	if ( ! x509ss ) {
		fprintf(stderr, "couldn't read certificate %s.\n", filename);
		return -1;
	}
        
        len = x509_to_msg(x509ss, buf, BUFMAXSIZE, skey1, skey2);
        if (len < 0) {
                fprintf(stderr, "Error reading certificate.\n");
                return -1;
        }

        len = prelude_io_write_delimited(pio, buf, len);
        if ( len <= 0 ) {
                fprintf(stderr, "couldn't send sensor certificate.\n");
                return -1;
        }

        return 0;
}



int prelude_ssl_save_cert(const char *filename, char *cert, int certlen)
{
	BIO *file;
        mode_t old_mask;

        /*
         * FIXME : BIO API probably have a way to set
         * permission... don't do a chmod() to avoid a race.
         * (see ChangeLog of 2001-03-30).
         */

        old_mask = umask(S_IRWXG|S_IRWXO);
        
	file = BIO_new_file(filename, "a");
	if ( !file ) {
                umask(old_mask);
                return 0;
        }

        umask(old_mask);
        
	if (BIO_write(file, cert, certlen) <= 0) {
                BIO_free(file);
		return 0;
        }
        
	BIO_free(file);

	return 1;
}


#endif

