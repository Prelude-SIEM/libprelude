/*****
*
* Copyright (C) 2003 Krzysztof Zaraska <kzaraska@student.uci.agh.edu.pl>
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
#include <stdlib.h>
#include <stdarg.h>

#include "prelude-log.h"

#include "prelude-strbuf.h"


#define CHUNK_SIZE 1024
#define STRBUF_OWN_DATA 0x01



struct prelude_strbuf {
	char *buf;
        int flags;
        size_t size;
        size_t index;
};




prelude_strbuf_t *prelude_strbuf_new(void)
{
	prelude_strbuf_t *new;
	
	new = malloc(sizeof(*new));
	if ( ! new ) {
		log(LOG_ERR, "memory exhausted.\n");
		return NULL;
	}
	
	new->buf = malloc(CHUNK_SIZE);
	if ( ! new->buf ) {
		log(LOG_ERR, "memory exhausted.\n");
		free(new);
		return NULL;
	}

        new->index = 0;
	new->buf[0] = '\0';
	new->size = CHUNK_SIZE;

        new->flags = 0;
	new->flags |= STRBUF_OWN_DATA;
        
	return new;
}




int prelude_strbuf_vprintf(prelude_strbuf_t *s, const char *fmt, va_list ap)
{
        int ret;
        char *ptr;
        
        ret = vsnprintf(s->buf + s->index, s->size - s->index, fmt, ap);
        
        /*
         * From sprintf(3) on GNU/Linux:
         *
         * snprintf  and vsnprintf do not write more than
         * size bytes (including the trailing '\0'), and return -1 if
         * the  output  was truncated due to this limit.  (Thus until
         * glibc 2.0.6. Since glibc 2.1 these  functions  follow  the
         * C99  standard and return the number of characters (exclud-
         * ing the trailing '\0') which would have  been  written  to
         * the final string if enough space had been available.)
         */
        if ( ret >= 0 && ret < s->size - s->index ) {
                s->index += ret;
                return ret;
        }

        ptr = realloc(s->buf, s->size + CHUNK_SIZE);
        if ( ! ptr ) {
                log(LOG_ERR, "memory exhausted.\n");
                s->buf[s->size - 1] = '\0';
                return -1;
        }
		
        s->buf = ptr;
        s->size += CHUNK_SIZE;

        return prelude_strbuf_vprintf(s, fmt, ap);
}




int prelude_strbuf_sprintf(prelude_strbuf_t *s, const char *fmt, ...)
{
	int ret;
	va_list ap;
	
        va_start(ap, fmt);
        ret = prelude_strbuf_vprintf(s, fmt, ap);
        va_end(ap);
                		
	return ret;
}



void prelude_strbuf_dont_own(prelude_strbuf_t *s)
{
        s->flags &= ~STRBUF_OWN_DATA;
}




char *prelude_strbuf_get_string(prelude_strbuf_t *s)
{
	return s->buf;
}



size_t prelude_strbuf_get_len(prelude_strbuf_t *s)
{
        return s->index;
}



int prelude_strbuf_is_empty(prelude_strbuf_t *s)
{
	return (s->index == 0) ? 0 : -1;
}




void prelude_strbuf_clear(prelude_strbuf_t *s)
{
	*(s->buf) = '\0';
	s->index = 0;
}



void prelude_strbuf_destroy(prelude_strbuf_t *s) 
{
        if ( s->flags & STRBUF_OWN_DATA )
                free(s->buf);

        free(s);
}




