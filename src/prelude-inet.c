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
#include "extract.h"
#include "libmissing.h"
#include "prelude-inttypes.h"
#include "prelude-log.h"
#include "prelude-inet.h"


#define PRELUDE_INET_EAI_FAMILY     -1
#define PRELUDE_INET_EAI_SOCKTYPE   -2
#define PRELUDE_INET_EAI_BADFLAGS   -3
#define PRELUDE_INET_EAI_NONAME     -4
#define PRELUDE_INET_EAI_SERVICE    -5
#define PRELUDE_INET_EAI_ADDRFAMILY -6
#define PRELUDE_INET_EAI_NODATA     -7
#define PRELUDE_INET_EAI_MEMORY     -8
#define PRELUDE_INET_EAI_FAIL       -9
#define PRELUDE_INET_EAI_EAGAIN     -10
#define PRELUDE_INET_EAI_SYSTEM     -11


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



#ifdef MISSING_GETADDRINFO

static int addrinfo_new(int flags, const char *host, struct in_addr *addr, uint16_t port, prelude_addrinfo_t **out) 
{
        prelude_addrinfo_t *new;
        struct sockaddr_in *sin;
        
        new = calloc(1, sizeof(*new));
        if ( ! new ) {
                log(LOG_ERR, "memory exhausted.\n");
                return PRELUDE_INET_EAI_MEMORY;
        }

        new->ai_next = NULL;
        new->ai_family = AF_INET;
        new->ai_addrlen = sizeof(*sin);

        if ( flags & AI_CANONNAME ) {
                new->ai_canonname = strdup(host);
                if ( ! new->ai_canonname )
                        return PRELUDE_INET_EAI_MEMORY;
        }

        sin = malloc(sizeof(struct sockaddr_in));
        if ( ! sin ) {
                free(new);
                return PRELUDE_INET_EAI_MEMORY;
        }
        
        sin->sin_family = AF_INET;
        sin->sin_port = htons(port);
        new->ai_addr = (struct sockaddr *) sin;
        memcpy(&sin->sin_addr, addr, sizeof(sin->sin_addr));
        
        *out = new;
        
        return 0;
}



static int getaddrinfo_compat(int flags, const char *host, const char *service, prelude_addrinfo_t **res) 
{
        int i, ret;
        struct hostent *h;
        uint16_t dport = 0;
        struct in_addr tmpaddr;
        prelude_addrinfo_t *head, **tmp;
        
        h = gethostbyname(host);
        if ( ! h )
                return PRELUDE_INET_EAI_NONAME;

        if ( service )
                dport = atoi(service);
        
        tmp = &head;
        for ( i = 0; h->h_addr_list[i] != NULL; i++ ) {

                tmpaddr.s_addr = align_uint32(h->h_addr_list[i]);
                
                ret = addrinfo_new(flags, h->h_name, &tmpaddr, dport, tmp);
                if ( ret < 0 )
                        return ret;

                tmp = &(*tmp)->ai_next;
        }

        *res = head;
        
        return 0;
}




static void freeaddrinfo_compat(prelude_addrinfo_t *info) 
{
        prelude_addrinfo_t *bkp;
        
        while ( info ) {
                
                if ( info->ai_canonname )
                        free(info->ai_canonname);
        
                free(info->ai_addr);

                bkp = info->ai_next;
                free(info);
                
                info = bkp;
        }
}



static const char *gai_strerror_compat(int errcode) 
{
        unsigned int key;
        static const char *tbl[] = {
                /* EAI_FAMILY     */ "ai_family not supported",
                /* EAI_SOCKTYPE   */ "ai_socktype not supported",
                /* EAI_BADFLAGS   */ "Bad value for ai_flags",
                /* EAI_NONAME     */ "Name or service not known",
                /* EAI_SERVICE    */ "Servname not supported for ai_socktype",
                /* EAI_ADDRFAMILY */ "Address family for hostname not supported", 
                /* EAI_NODATA     */ "No address associated with hostname",
                /* EAI_MEMORY     */ "Memory allocation failure",
                /* EAI_FAIL       */ "Non-recoverable failure in name resolution",
                /* EAI_EAGAIN     */ "Temporary failure in name resolution",
                /* EAI_SYSTEM     */ "System error",
        };

        key = 0 - errcode - 1;
        if ( key >= (sizeof(tbl) / sizeof(char *)) ) 
                return "Unknown error";
        
        return tbl[key];
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



int prelude_inet_getaddrinfo(const char *node, const char *service,
                             const prelude_addrinfo_t *hints, prelude_addrinfo_t **res) 
{
        
#ifndef MISSING_GETADDRINFO
        return getaddrinfo(node, service, hints, res);
#else
        return getaddrinfo_compat(hints ? hints->ai_flags : 0, node, service, res);
#endif
}



void prelude_inet_freeaddrinfo(prelude_addrinfo_t *info) 
{
        
#ifndef MISSING_GETADDRINFO
        freeaddrinfo(info);
#else
        freeaddrinfo_compat(info);
#endif
        
}



const char *prelude_inet_gai_strerror(int errcode) 
{
        
#ifdef MISSING_GETADDRINFO
        return gai_strerror_compat(errcode);
#else
        return gai_strerror(errcode);
#endif

}


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
        prelude_addrinfo_t *ptr, hints;

        memset(&hints, 0, sizeof(hints));

        hints.ai_family = PF_UNSPEC;
        hints.ai_protocol = IPPROTO_TCP;
            //hints.ai_flags = AI_CANONNAME|AI_PASSIVE;
        
        ret = prelude_inet_getaddrinfo(argv[1], NULL, &hints, &ptr);        
        if ( ret < 0 )
                return -1;

        for ( ; ptr != NULL; ptr = ptr->ai_next ) {

                addr = prelude_inet_sockaddr_to_inaddr(ptr->ai_family, ptr->ai_addr);
                                
                    //     if ( ptr->ai_flags & AI_CANONNAME )
                printf("name=%s\n", ptr->ai_canonname);
                printf("%s\n", prelude_inet_ntop(ptr->ai_family, addr, out, sizeof(out)));
        }
        
        prelude_inet_freeaddrinfo(ptr);
}
#endif



