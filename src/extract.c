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

#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include <sys/types.h>
#include <netinet/in.h>

#include "prelude-log.h"
#include "extract.h"



static int is_aligned(void *ptr, size_t size) 
{
        unsigned long p = (unsigned long) ptr;
        
        return ( (p % size) == 0) ? 0 : -1;        
}



int extract_uint64(uint64_t *dst, void *buf, uint32_t blen) 
{
        uint64_t tmp;
        
        if ( blen != sizeof(uint64_t) ) {
                log(LOG_ERR, "Datatype error, buffer is not uint64: couldn't convert.\n");
                return -1;
        }

        /*
         * check alignment
         */
        if ( is_aligned(buf, sizeof(*dst)) < 0 ) {
                memmove(&tmp, buf, sizeof(tmp));
                buf = &tmp;
        }

        ((uint32_t *) dst)[0] = ntohl(((uint32_t *) buf)[1]);
        ((uint32_t *) dst)[1] = ntohl(((uint32_t *) buf)[0]);
     
        return 0;
}




int extract_uint32(uint32_t *dst, void *buf, uint32_t blen) 
{
        uint32_t tmp;
        
        if ( blen != sizeof(uint32_t) ) {
                log(LOG_ERR, "Datatype error, buffer is not uint32: couldn't convert.\n");
                return -1;
        }

        /*
         * check alignment
         */
        if ( is_aligned(buf, sizeof(*dst)) < 0 ) {
                memmove(&tmp, buf, sizeof(tmp));
                buf = &tmp;
        }
        
        *dst = ntohl(*(uint32_t *) buf);
               
        return 0;
}




int extract_uint16(uint16_t *dst, void *buf, uint32_t blen) 
{
        uint16_t tmp;
        
        if ( blen != sizeof(uint16_t) ) {
                log(LOG_ERR, "Datatype error, buffer is not uint16: couldn't convert.\n");
                return -1;
        }

        /*
         * check alignment
         */
        if ( is_aligned(buf, sizeof(*dst)) < 0 ) {
                memmove(&tmp, buf, sizeof(tmp));
                buf = &tmp;
        }
        
        *dst = ntohs(*(uint16_t *) buf);

        return 0;
}




int extract_uint8(uint8_t *dst, void *buf, uint32_t blen) 
{
        if ( blen != sizeof(uint8_t) ) {
                log(LOG_ERR, "Datatype error, buffer is not uint8: couldn't convert.\n");
                return -1;
        }

        *dst = *(uint8_t *) buf;

        return 0;
}




const char *extract_str(void *buf, uint32_t blen) 
{
        const char *str = buf;
        
        if ( str[blen - 1] != '\0' ) 
                return NULL;

        return buf;
}












