#include "config.h"

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

#ifdef HAVE_SSL

#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <assert.h>
#include <errno.h>

#include <openssl/des.h>
#include <openssl/bio.h>
#include <openssl/x509.h>
#include <openssl/pem.h>
#include <openssl/buffer.h>
#include <openssl/sha.h>
#include <openssl/ssl.h>
#include <openssl/x509v3.h>
#include <openssl/pem.h>
#include <openssl/bio.h>

#include "ssl.h"
#include "common.h"
#include "prelude-io.h"
#include "prelude-message.h"
#include "prelude-message-id.h"
#include "ssl-register.h"
#include "ssl-settings.h"
#include "ssl-gencrypto.h"
#include "prelude-path.h"
#include "ssl-registration-msg.h"


#define ACKMSGLEN ACKLENGTH + SHA_DIGEST_LENGTH + HEADLENGTH + PADMAXSIZE




static int send_own_certificate(prelude_io_t *pio, des_key_schedule *skey1,
                                des_key_schedule *skey2, int expire, int keysize, uid_t uid) 
{
        int ret;
        char filename[256];

        prelude_get_ssl_key_filename(filename, sizeof(filename));
        
        ret = prelude_ssl_gen_crypto(keysize, expire, filename, 0, uid);
	if ( ret < 0 ) {
		fprintf(stderr, "\nRegistration failed\n");
		return -1;
	}
        
        ret = prelude_ssl_send_cert(pio, filename, skey1, skey2);
        if ( ret < 0 ) {
                fprintf(stderr, "Error sending certificate.\n");
                return -1;
        }
        
        return 0;
}




static int recv_manager_certificate(prelude_io_t *pio, des_key_schedule *skey1,
                                    des_key_schedule *skey2, uid_t uid)  
{
        uint16_t len;
        BUF_MEM ackbuf;
        int ret, certlen;
        unsigned char *buf;
        char filename[256];
        char cert[BUFMAXSIZE];
        
        len = prelude_io_read_delimited(pio, (void **)&buf);
        if ( len <= 0 ) {
                fprintf(stderr, "Error receiving registration message\n");
                return -1;
        }
        
	certlen = analyse_install_msg(buf, len, cert, len, skey1, skey2);
	if ( certlen < 0 ) {
		fprintf(stderr, "Bad message received - Registration failed.\n");
                return -1;
	}

	fprintf(stderr, "writing Prelude Manager certificate.\n");

        
	/*
         * save Manager certificate
         */
        prelude_get_ssl_cert_filename(filename, sizeof(filename));        

        ret = prelude_ssl_save_cert(filename, cert, certlen, uid);
        if ( ret < 0 ) {
		fprintf(stderr, "error writing Prelude-Report Certificate to %s\n", filename);
                return -1;
	}
        
        /*
         * send ack
         */
	ackbuf.length = ACKLENGTH;
	ackbuf.data = ACK;
	ackbuf.max = ACKLENGTH;

	len = build_install_msg(&ackbuf, buf, ACKMSGLEN, skey1, skey2);
	if ( len <= 0 ) {
		fprintf(stderr, "Error building message - Registration failed.\n");
                return -1;
	}
        
        ret = prelude_io_write_delimited(pio, buf, len);
        if ( ret < 0 ) {
                fprintf(stderr, "Error sending registration message.\n");
                return -1;
        }
        

        return 0;
}




static void ask_configuration(int *keysize, int *expire) 
{
        int ret;
        char buf[1024];
        
        prelude_ssl_ask_settings(keysize, expire);
        
        if ( *expire )
                snprintf(buf, sizeof(buf), "%d days", *expire);
        else
                snprintf(buf, sizeof(buf), "Never");
        
        fprintf(stderr, "\n\n"
                "Key length        : %d\n"
                "Expire            : %s\n", *keysize, buf);

        
        while ( 1 ) {
                fprintf(stderr, "\nIs this okay [yes/no] : ");

                fgets(buf, sizeof(buf), stdin);
                buf[strlen(buf) - 1] = '\0';
                
                ret = strcmp(buf, "yes");
                if ( ret == 0 )
                        break;
                
                ret = strcmp(buf, "no");
                if ( ret == 0 )
                        ask_configuration(keysize, expire);
        }
        
        fprintf(stderr, "\n");
}



static int tell_ssl_usage(prelude_io_t *fd) 
{
        ssize_t ret;
        prelude_msg_t *msg;
        
        msg = prelude_msg_new(1, 0, PRELUDE_MSG_AUTH, 0);
        if ( ! msg )
                return -1;

        prelude_msg_set(msg, PRELUDE_MSG_AUTH_SSL, 0, NULL);     
        ret = prelude_msg_write(msg, fd);
        prelude_msg_destroy(msg);

        return ret;
}



int ssl_add_certificate(prelude_io_t *fd, char *pass, size_t size, uid_t uid)
{
	int ret;
        int keysize, expire;
        des_cblock pre1, pre2;
	des_key_schedule skey1, skey2;

        des_string_to_2keys(pass, &pre1, &pre2);
        memset(pass, 0, size);
        
        ret = des_set_key(&pre1, skey1);
        memset(&pre1, 0, sizeof(des_cblock));
        if ( ret < 0 ) 
		return -1;

	ret = des_set_key(&pre2, skey2);
	memset(&pre2, 0, sizeof(des_cblock));
        if ( ret < 0 )
		return -1;

        /*
         * Tell manager-adduser we're gonna use SSL.
         */
        ret = tell_ssl_usage(fd);
        if ( ret < 0 )
                return -1;

        ask_configuration(&keysize, &expire);

        
        ret = send_own_certificate(fd, &skey1, &skey2, expire, keysize, uid);
        if ( ret < 0 ) {
                fprintf(stderr, "Error sending own certificate - Registration failed.\n");
                return -1;
        }

        ret = recv_manager_certificate(fd, &skey1, &skey2, uid);
        if ( ret < 0 ) {
                fprintf(stderr, "Error receiving Manager certificate - Registration failed.\n");
                return -1;
        }
        
        prelude_io_close(fd);
        prelude_io_destroy(fd);
        
	return 0;
}

#endif /* HAVE_SSL */

