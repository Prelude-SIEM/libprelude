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

void extract_ipv4_addr(struct in_addr *out, struct in_addr *addr);

int extract_uint64(uint64_t *dst, const void *buf, uint32_t blen);

int extract_uint32(uint32_t *dst, const void *buf, uint32_t blen);

int extract_uint16(uint16_t *dst, const void *buf, uint32_t blen);

int extract_uint8(uint8_t *dst, const void *buf, uint32_t blen);

const char *extract_str(void *buf, uint32_t blen);



#define extract_int(type, buf, blen, dst) do {        \
        int ret;                                      \
        ret = extract_ ## type (&dst, buf, blen);     \
        if ( ret < 0 )                                \
                return -1;                            \
} while (0)
           

#define extract_string(buf, blen, dst)                                    \
        dst = extract_str(buf, blen);                                     \
        if ( ! dst ) {                                                    \
               log(LOG_ERR, "Datatype error, buffer is not a string.\n"); \
               return -1;                                                 \
        }
