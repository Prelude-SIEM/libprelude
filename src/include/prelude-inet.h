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

#ifndef _PRELUDE_INET_H
#define _PRELUDE_INET_H

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>

#ifndef MISSING_GETADDRINFO

#ifndef AI_CANONNAME
 #define AI_CANONNAME 0x01
#endif

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



