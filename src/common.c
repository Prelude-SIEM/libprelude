/*****
*
* Copyright (C) 2002 Yoann Vandoorselaere <yoann@mandrakesoft.com>
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
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <assert.h>

#include "common.h"


int prelude_resolve_addr(const char *hostname, struct in_addr *addr) 
{
        int ret;
        struct hostent *h;

        /*
         * This is not an hostname. No need to resolve.
         */
        ret = inet_aton(hostname, addr);
        if ( ret != 0 ) 
                return 0;
        
        h = gethostbyname(hostname);
        if ( ! h )
                return -1;

        assert(h->h_length <= sizeof(*addr));
        
        memcpy(addr, h->h_addr, h->h_length);
                
        return 0;
}



