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
#include "ssl-settings.h"
#include "prelude-io.h"
#include "prelude-path.h"


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



static void ask_manager_addr(char **addr, uint16_t *port) 
{
        char buf[1024];
 
        fprintf(stderr, "\n\nWhat is the Manager address ? ");
        fgets(buf, sizeof(buf), stdin);

        buf[strlen(buf) - 1] = '\0';
        *addr = strdup(buf);

        fprintf(stderr, "What is the Manager port [5554] ? ");
        fgets(buf, sizeof(buf), stdin);
        *port = ( *buf == '\n' ) ? 5554 : atoi(buf);
}




static void ask_configuration(char **addr, uint16_t *port, int *keysize, int *expire) 
{
        int ret;
        char buf[1024];
        
        ask_manager_addr(addr, port);
        prelude_ssl_ask_settings(keysize, expire);
        
        if ( *expire )
                snprintf(buf, sizeof(buf), "%d days", *expire);
        else
                snprintf(buf, sizeof(buf), "Never");
        
        fprintf(stderr, "\n\n"
                "Manager address   : %s:%d\n"
                "Key length        : %d\n"
                "Expire            : %s\n",
                *addr, *port, *keysize, buf);

        
        while ( 1 ) {
                fprintf(stderr, "Is this okay [yes/no] : ");

                fgets(buf, sizeof(buf), stdin);
                buf[strlen(buf) - 1] = '\0';
                
                ret = strcmp(buf, "yes");
                if ( ret == 0 )
                        break;
                
                ret = strcmp(buf, "no");
                if ( ret == 0 )
                        ask_configuration(addr, port, keysize, expire);
        }
        
        fprintf(stderr, "\n");
}


void prelude_ssl_ask_settings(int *keysize, int *expire) 
{
        ask_keysize(keysize);
        ask_expiration_time(expire);
}


#endif /* HAVE_SSL */

