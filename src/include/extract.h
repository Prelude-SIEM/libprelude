/*****
*
* Copyright (C) 2002, 2003 Yoann Vandoorselaere <yoann@prelude-ids.org>
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

#ifndef _LIBPRELUDE_EXTRACT_H
#define _LIBPRELUDE_EXTRACT_H


#include <inttypes.h>


#ifdef NEED_ALIGNED_ACCESS

#include <string.h> /* for memmove */


/*
 * Using memmove make the generated code substencially slower,
 * we seen difference from 20MB/s to 200MB/s from the memmove version
 * to this version in doing checksum test.
 */

#ifdef WORDS_BIGENDIAN
 #define byte(type, buf, pos) (type) ((const uint8_t *) (buf))[(pos)]
#else
 #define byte(type, buf, pos) (type) ((const uint8_t *) (buf))[sizeof(type) - 1 - (pos)]
#endif


static inline uint16_t align_uint16(const void *buf)
{
        return byte(uint16_t, buf, 0) << 8 | byte(uint16_t, buf, 1);
}




static inline int32_t align_int32(const void *buf) 
{
        return  byte(int32_t, buf, 0) << 24 | byte(int32_t, buf, 1) << 16 |
                byte(int32_t, buf, 2) <<  8 | byte(int32_t, buf, 3);
}



static inline uint32_t align_uint32(const void *buf) 
{
        return  byte(uint32_t, buf, 0) << 24 | byte(uint32_t, buf, 1) << 16 |
                byte(uint32_t, buf, 2) <<  8 | byte(uint32_t, buf, 3);
}




static inline uint64_t align_uint64(const void *buf) 
{
        return  byte(uint64_t, buf, 0) << 56 | byte(uint64_t, buf, 1) << 48 | byte(uint64_t, buf, 2) << 40 |
                byte(uint64_t, buf, 3) << 32 | byte(uint64_t, buf, 4) << 24 | byte(uint64_t, buf, 5) << 16 |
                byte(uint64_t, buf, 6) <<  8 | byte(uint64_t, buf, 7);
}



static inline struct in_addr align_ipv4_addr(const void *buf) 
{
        struct in_addr tmp;
        
        memmove(&tmp, buf, sizeof(tmp));

        return tmp;
}


#else

#define align_uint16(x) (*(const uint16_t *) (x))
#define align_int32(x) (*(const int32_t *) (x))
#define align_uint32(x) (*(const uint32_t *) (x))
#define align_uint64(x) (*(const uint64_t *) (x))
#define align_ipv4_addr(x) *((const struct in_addr *) (x))

#endif

#include "idmef-string.h"
#include "idmef-time.h"
#include "idmef-data.h"

static inline struct in_addr extract_ipv4_addr(const void *buf) 
{
        return align_ipv4_addr(buf);
}



static inline uint16_t extract_uint16(const void *buf) 
{
        return ntohs(align_uint16(buf));
}



static inline int32_t extract_int32(const void *buf) 
{
        return ntohl(align_int32(buf));
}



static inline uint32_t extract_uint32(const void *buf) 
{
        return ntohl(align_uint32(buf));
}



static inline uint64_t extract_uint64(const void *buf) 
{    
#ifdef WORDS_BIGENDIAN
        
        return align_uint64(buf);

#else
        union {
                uint64_t val64;
                uint32_t val32[2];
        } combo_r, combo_w;

        combo_r.val64 = align_uint64(buf);

        combo_w.val32[0] = ntohl(combo_r.val32[1]);
        combo_w.val32[1] = ntohl(combo_r.val32[0]);

        return combo_w.val64;
#endif
}



static inline float extract_float(const void *buf)
{
	return *((const float *) buf); /* FIXME: do we need alignment for float */
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



static inline int extract_int32_safe(int32_t *out, const void *buf, size_t len) 
{
        if ( len != sizeof(int32_t) ) {
                log(LOG_ERR, "Datatype error, buffer doesn't contain 32 bits integer.\n");
                return -1;
        }

        *out = extract_int32(buf);
	
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



static inline int extract_float_safe(float *out, const void *buf, size_t len)
{
	if ( len != sizeof (*out) ) {
		log(LOG_ERR, "Datatype error, buffer doesn't contain a float.\n");
		return -1;
	}

	*out = extract_float(buf);

	return 0;
}



static inline int extract_characters_safe(const char **out, char *buf, size_t len)
{
	if ( buf[len - 1] != '\0' ) {
                log(LOG_ERR, "Datatype error: buffer is not a string.\n");
                return -1;
        }

	*out = buf;

	return 0;
}

#endif /* _LIBPRELUDE_EXTRACT_H */
