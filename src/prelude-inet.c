/*****
*
* Copyright (C) 2003, 2004 Yoann Vandoorselaere <yoann@prelude-ids.org>
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <errno.h>

#include "prelude-log.h"
#include "prelude-error.h"
#include "libmissing.h"
#include "prelude-inttypes.h"
#include "prelude-log.h"
#include "prelude-inet.h"


#ifndef INADDR_LOOPBACK
 #define INADDR_LOOPBACK        ((uint32_t) 0x7f000001) /* Inet 127.0.0.1.  */
#endif


#ifdef HAVE_IPV6

static inline int is_ipv6_loopback(void *addr) 
{
        if ( ((const uint32_t *) addr)[0] == 0 &&
             ((const uint32_t *) addr)[1] == 0 &&
             ((const uint32_t *) addr)[2] == 0 &&
             ((const uint32_t *) addr)[3] == htonl(1) )
                return 0;

        return -1;
}

#endif


#ifndef HAVE_INET_NTOP

static const char *inet_ntop_compat(int af, const struct in_addr *addr, char *out, size_t cnt) 
{
        char *ptr;
        
        if ( af != AF_INET ) {
                errno = EAFNOSUPPORT;
                return NULL;
        }
        
        ptr = inet_ntoa(*addr);
        if ( ! ptr ) 
                return NULL;

        if ( (strlen(ptr) + 1) > cnt ) {
                errno = ENOSPC;
                return NULL;
        }
        
        memcpy(out, ptr, cnt);
        
        return out;
}

#endif




const char *prelude_inet_ntop(int af, const void *src, char *dst, size_t cnt) 
{

#ifdef HAVE_INET_NTOP
        return inet_ntop(af, src, dst, cnt);
#else
        return inet_ntop_compat(af, src, dst, cnt);
#endif       
}



void *prelude_inet_sockaddr_get_inaddr(struct sockaddr *sa) 
{
        void *ret = NULL;
        
        if ( sa->sa_family == AF_INET )
                ret = &((struct sockaddr_in *) sa)->sin_addr;

#ifdef HAVE_IPV6
        else if ( sa->sa_family == AF_INET6 )
                ret = &((struct sockaddr_in6 *) sa)->sin6_addr;
#endif
        
        return ret;
}



int prelude_inet_addr_is_loopback(int af, void *addr) 
{
        int ret = -1;
        
        if ( af == AF_INET ) {
                uint32_t tmp = *(uint32_t *) addr;
                ret = (htonl(tmp) == INADDR_LOOPBACK) ? 0 : -1;
        }
        
#ifdef HAVE_IPV6
        else if ( af == AF_INET6 ) 
                ret = is_ipv6_loopback(addr);
#endif

        return ret;
}



#if 0
int main(int argc, char **argv) 
{
        int ret;
        void *addr;
        char out[128];
        struct addrinfo *ptr, hints;

        memset(&hints, 0, sizeof(hints));

        hints.ai_family = PF_UNSPEC;
        hints.ai_protocol = IPPROTO_TCP;
        
        ret = getaddrinfo(argv[1], NULL, &hints, &ptr);        
        if ( ret < 0 )
                return -1;

        for ( ; ptr != NULL; ptr = ptr->ai_next ) {
                
                addr = prelude_inet_sockaddr_get_inaddr(ptr->ai_addr);
                printf("name=%s\n", ptr->ai_canonname);
                printf("addr=%s\n", prelude_inet_ntop(ptr->ai_family, addr, out, sizeof(out)));
        }
        
        freeaddrinfo(ptr);
}
#endif

