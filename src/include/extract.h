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

#ifndef EXTRACT_H
#define EXTRACT_H


#ifdef NEED_ALIGNED_ACCESS

/*
 * Using memmove make the generated code 200% slower,
 * that's the reason we use theses hack.
 */

#define byte(buf, x) ((uint8_t *) (buf))[(x)]


static inline uint16_t align_uint16(const void *buf)
{
        return byte(buf, 0) | byte(buf, 1);
}




static inline uint32_t align_uint32(const void *buf) 
{
        return byte(buf, 0) | byte(buf, 1) | byte(buf, 2) | byte(buf, 3);
}




static inline uint64_t align_uint64(const void *buf) 
{
        return  byte(buf, 0) | byte(buf, 1) | byte(buf, 2) | byte(buf, 3) |
                byte(buf, 4) | byte(buf, 5) | byte(buf, 6) | byte(buf, 7);
}



static inline struct in_addr align_ipv4_addr(const void *buf) 
{
        struct in_addr tmp;
        
        memmove(&tmp, buf, sizeof(tmp));

        return tmp;
}


#else


#define align_uint16(x) (*(const uint16_t *) (x))
#define align_uint32(x) (*(const uint32_t *) (x))
#define align_uint64(x) (*(const uint64_t *) (x))
#define align_ipv4_addr(x) *((const struct in_addr *) (x))

#endif


static inline struct in_addr extract_ipv4_addr(const void *buf) 
{
        return align_ipv4_addr(buf);
}



static inline uint16_t extract_uint16(const void *buf) 
{
        return ntohs(align_uint16(buf));
}



static inline uint32_t extract_uint32(const void *buf) 
{
        return ntohl(align_uint32(buf));
}



static inline uint64_t extract_uint64(const void *buf) 
{
        uint64_t tmp, swap;

        tmp = align_uint64(buf);

        ((uint32_t *) &swap)[0] = ntohl(((uint32_t *) &tmp)[1]);
        ((uint32_t *) &swap)[1] = ntohl(((uint32_t *) &tmp)[0]);
        
        return swap;
}



/*
 * Theses function check the buffer size for safety. 
 */
static inline int extract_uint8_safe(uint8_t *out, const void *buf, size_t len) 
{
        if ( len != sizeof(uint8_t) ) {
                log(LOG_ERR, "Datatype error, buffer doesn't contain 8 bits integer.\n");
                return -1;
        }

        *out = *(const uint8_t *) buf;
        
        return 0;
}


static inline int extract_uint16_safe(uint16_t *out, const void *buf, size_t len) 
{
        if ( len != sizeof(uint16_t) ) {
                log(LOG_ERR, "Datatype error, buffer doesn't contain 16 bits integer.\n");
                return -1;
        }

        *out = extract_uint16(buf);

        return 0;
}



static inline int extract_uint32_safe(uint32_t *out, const void *buf, size_t len) 
{
        if ( len != sizeof(uint32_t) ) {
                log(LOG_ERR, "Datatype error, buffer doesn't contain 32 bits integer.\n");
                return -1;
        }

        *out = extract_uint32(buf);

        return 0;
}



static inline int extract_uint64_safe(uint64_t *out, const void *buf, size_t len) 
{
        if ( len != sizeof(uint64_t) ) {
                log(LOG_ERR, "Datatype error, buffer doesn't contain 64 bits integer.\n");
                return -1;
        }
        
        *out = extract_uint64(buf);

        return 0;
}



static inline int extract_string_safe(const char **out, const char *buf, size_t len) 
{
        if ( buf[len - 1] != '\0' ) {
                log(LOG_ERR, "Datatype error: buffer is not a string.\n");
                return -1;
        }
        
        *out = buf;

        return 0;
}



#endif

