/*****
*
* Copyright (C) 2003 Yoann Vandoorselaere <yoann@prelude-ids.org>
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

#ifndef _PRELUDE_INET_H
#define _PRELUDE_INET_H

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>

#include "config.h"


/*
 * inet_ntop() error return
 */
#ifndef EAFNOSUPPORT
 #define EAFNOSUPPORT -2
#endif

#ifndef ENOSPC
 #define ENOSPC -3
#endif


/*
 * getaddrinfo() error return
 */
#ifndef EAI_FAMILY
 #define EAI_FAMILY -1
#endif

#ifndef EAI_SOCKTYPE
 #define EAI_SOCKTYPE -2
#endif

#ifndef EAI_BADFLAGS
 #define EAI_BADFLAGS -3
#endif

#ifndef EAI_NONAME
 #define EAI_NONAME -4
#endif

#ifndef EAI_SERVICE
 #define EAI_SERVICE -5
#endif

#ifndef EAI_ADDRFAMILY
 #define EAI_ADDRFAMILY -6
#endif

#ifndef EAI_NODATA
 #define EAI_NODATA -7
#endif

#ifndef EAI_MEMORY
 #define EAI_MEMORY -8
#endif

#ifndef EAI_FAIL
 #define EAI_FAIL -9
#endif

#ifndef EAI_EAGAIN
 #define EAI_EAGAIN -10
#endif

#ifndef EAI_SYSTEM
 #define EAI_SYSTEM -11
#endif


#ifndef MISSING_GETADDRINFO

 typedef struct addrinfo prelude_addrinfo_t;

#else

typedef struct prelude_addrinfo {
        int     ai_flags;
        int     ai_family;
        int     ai_socktype;
        int     ai_protocol;
        size_t  ai_addrlen;
        struct sockaddr *ai_addr;
        char   *ai_canonname;
        struct prelude_addrinfo *ai_next;
} prelude_addrinfo_t;

#endif


int prelude_inet_addr_is_loopback(int af, void *addr);

void *prelude_inet_sockaddr_get_inaddr(struct sockaddr *sa);

const char *prelude_inet_gai_strerror(int errcode);

void prelude_inet_freeaddrinfo(prelude_addrinfo_t *info);

int prelude_inet_getaddrinfo(const char *node, const char *service,
                             const prelude_addrinfo_t *hints, prelude_addrinfo_t **res);

const char *prelude_inet_ntop(int af, const void *src, char *dst, size_t cnt);

#endif



