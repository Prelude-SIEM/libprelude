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
#include "prelude-log.h"
#include "prelude-io.h"
#include "prelude-path.h"


static void ask_keysize(int *keysize) 
{
        char buf[10];
        
        fprintf(stderr, "\n\nWhat keysize do you want [1024] ? ");

        if ( ! fgets(buf, sizeof(buf), stdin) )
                *buf = '\n';
        
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

        if ( ! fgets(buf, sizeof(buf), stdin) )
                *buf = '\n';
        
        *expire = ( *buf == '\n' ) ? 0 : atoi(buf);
}



void prelude_ssl_ask_settings(int *keysize, int *expire) 
{
        ask_keysize(keysize);
        ask_expiration_time(expire);
}


#endif /* HAVE_SSL */



