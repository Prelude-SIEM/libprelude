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

#include "config.h"

#ifdef HAVE_SSL

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

#include "ssl.h"
#include "common.h"
#include "prelude-path.h"



static SSL_CTX *ctx;



SSL *ssl_connect_server(int socket)
{
	int err;
        SSL *ssl;
        
	ssl = SSL_new(ctx);
	if (!ssl) {
		ERR_print_errors_fp(stderr);
		return NULL;
	}
        
	err = SSL_set_fd(ssl, socket);
	if (err <= 0) {
		ERR_print_errors_fp(stderr);
                SSL_free(ssl);
                return NULL;
	}
        
	/*
         * handshake
         */
	err = SSL_connect(ssl);        
        if (err <= 0) {
		ERR_print_errors_fp(stderr);
                SSL_free(ssl);
                return NULL;
	}
        
        return ssl;
}




/**
 * ssl_init_client:
 *
 * Initialize a new ssl client.
 *
 * Returns: 0 on sucess, -1 on error.
 */
int ssl_init_client(void)
{
        int ret;
        char filename[256];
        SSL_METHOD *method;
        
        /*
         * OpenSSL Initialisation.
         */
	SSL_load_error_strings();
	SSL_library_init();

	method = SSLv3_client_method();

	ctx = SSL_CTX_new(method);
	if ( ! ctx ) {
		ERR_print_errors_fp(stderr);
                ERR_free_strings();
		return -1;
	}

	SSL_CTX_set_options(ctx, SSL_OP_NO_SSLv2 | SSL_OP_NO_TLSv1);
	SSL_CTX_set_verify_depth(ctx, 1);
        
	/*
         * no callback, mutual authentication.
         */
	SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER, NULL);

        prelude_get_ssl_cert_filename(filename, sizeof(filename));
        
	ret = SSL_CTX_load_verify_locations(ctx, filename, NULL);
	if ( ret <= 0 ) {
                log(LOG_INFO, "\nNo Manager certificate available. Please restart with the\n"
                    "-c command line argument.\n");
                goto err;
	}

        prelude_get_ssl_key_filename(filename, sizeof(filename));
             
	ret = SSL_CTX_use_certificate_file(ctx, filename, SSL_FILETYPE_PEM);
	if ( ret <= 0 ) {
                log(LOG_ERR, "error loading certificate file %s.\n", filename);
		goto err;
        }

	ret = SSL_CTX_use_PrivateKey_file(ctx, filename, SSL_FILETYPE_PEM);
	if ( ret <= 0 ) {
                log(LOG_ERR, "error loading private key file %s.\n", filename);
                goto err;
        }
        
	if ( ! SSL_CTX_check_private_key(ctx) ) {
		fprintf(stderr,
			"Private key does not match the certificate public key\n");
		goto err;
	}

	return 0;

 err:
        ERR_print_errors_fp(stderr);
        SSL_CTX_free(ctx);
        ERR_free_strings();

        return -1;
}

#endif

















