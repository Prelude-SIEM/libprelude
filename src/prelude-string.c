/*****
*
* Copyright (C) 2004 Yoann Vandoorselaere <yoann@prelude-ids.org>
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

/*
 * This code include an heavily modified version of the prelude-strbuf
 * API made by Krzysztof Zaraska <kzaraska@student.uci.agh.edu.pl>, that
 * is now part of prelude-string.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <assert.h>

#include "common.h"
#include "libmissing.h"
#include "prelude-log.h"
#include "prelude-inttypes.h"
#include "prelude-string.h"

#define MAX(x, y) (((x) > (y)) ? (x) : (y))


#define CHUNK_SIZE 1024


/*
 * String structure may be free'd
 */
#define PRELUDE_STRING_OWN_STRUCTURE  0x1

/*
 * String data may be free'd
 */
#define PRELUDE_STRING_OWN_DATA       0x2


/*
 * Whether we can reallocate this string
 */
#define PRELUDE_STRING_CAN_REALLOC    0x4


#define check_string(str, len) check_string_f(__FUNCTION__, __LINE__, (str), (len))


inline static int check_string_f(const char *f, int l, const char *str, size_t len)
{
        if ( str[len] != 0 ) {
                fprintf(stderr, "*** %s:%d: warning, string is not NULL terminated.\n", f, l);
                return -1;
        }

        return 0;
}


static int string_buf_alloc(prelude_string_t *string, size_t len)
{
        /*
         * include room for terminating \0.
         */
        string->data.rwbuf = malloc(len + 1);
        if ( ! string->data.rwbuf ) 
                return -1;
        
        string->index = len;
        string->size = len + 1;
        
        return 0;
}



static void string_buf_copy(prelude_string_t *string, const char *buf, size_t len)
{
        assert(len < string->size);
        
        memcpy(string->data.rwbuf, buf, len);
        string->data.rwbuf[len] = '\0';
}



static int allocate_more_chunk_if_needed(prelude_string_t *s, size_t needed_len)
{
        char *ptr;
        size_t len;

        if ( needed_len )
                len = MAX(needed_len - (s->size - s->index), CHUNK_SIZE);
        else
                len = CHUNK_SIZE;
        
        if ( s->flags & PRELUDE_STRING_CAN_REALLOC ) {
                
                ptr = prelude_realloc(s->data.rwbuf, s->size + len);
                if ( ! ptr ) {
                        log(LOG_ERR, "memory exhausted.\n");
                        return -1;
                }
        }

        else {                        
                                
                ptr = malloc(s->size + len);
                if ( ! ptr ) {
                        log(LOG_ERR, "memory exhausted.\n");
                        return -1;
                }
                
                if ( s->data.robuf )
                        memcpy(ptr, s->data.robuf, s->index + 1);
                
                s->flags |= PRELUDE_STRING_CAN_REALLOC|PRELUDE_STRING_OWN_DATA;
        }

        s->size += len;
        s->data.rwbuf = ptr;

	return 0;
}



/*
 * creates an empty data
 */        
prelude_string_t *prelude_string_new(void)
{
        prelude_string_t *ret;

        ret = calloc(1, sizeof(*ret));
        if ( ! ret )
                return NULL;

        ret->refcount = 1;
        ret->flags = PRELUDE_STRING_OWN_STRUCTURE;

        return ret;
}




prelude_string_t *prelude_string_ref(prelude_string_t *data)
{
        data->refcount++;
        return data;
}



prelude_string_t *prelude_string_new_dup_fast(const char *str, size_t len)
{
        prelude_string_t *ret;

        if ( check_string(str, len) < 0 )
                return NULL;
        
        ret = prelude_string_new();
        if ( ! ret )
                return NULL;

        if ( string_buf_alloc(ret, len) < 0 )
                return NULL;
        
        string_buf_copy(ret, str, len);
        
        ret->flags |= PRELUDE_STRING_OWN_DATA|PRELUDE_STRING_CAN_REALLOC;
                
        return ret;
}



prelude_string_t *prelude_string_new_dup(const char *str)
{
        return prelude_string_new_dup_fast(str, strlen(str));
}



prelude_string_t *prelude_string_new_nodup_fast(char *str, size_t len)
{
        prelude_string_t *ret;
        
        if ( check_string(str, len) < 0 )
                return NULL;
                
        ret = prelude_string_new();
        if ( ! ret )
                return NULL;

        ret->index = len;
        ret->size = len + 1;
        ret->data.rwbuf = str;

        ret->flags |= PRELUDE_STRING_OWN_DATA;
        
        return ret;
}



prelude_string_t *prelude_string_new_nodup(char *str)
{
        return prelude_string_new_nodup_fast(str, strlen(str));
}



prelude_string_t *prelude_string_new_ref_fast(const char *buf, size_t len)
{
        prelude_string_t *ret;

        if ( check_string(buf, len) < 0 )
                return NULL;
        
        ret = prelude_string_new();
        if ( ! ret )
                return NULL;

        ret->index = len;
        ret->size = len + 1;
        ret->data.robuf = buf;
        
        return ret;
}



prelude_string_t *prelude_string_new_ref(const char *str)
{
        return prelude_string_new_ref_fast(str, strlen(str));
}



int prelude_string_set_dup_fast(prelude_string_t *string, const char *buf, size_t len)
{
        if ( check_string(buf, len) < 0 )
                return -1;
        
        prelude_string_destroy_internal(string);
        
        if ( string_buf_alloc(string, len) < 0 )
                return -1;

        string_buf_copy(string, buf, len);
        string->flags |= PRELUDE_STRING_OWN_DATA|PRELUDE_STRING_CAN_REALLOC;
        
        return 0;
}



int prelude_string_set_dup(prelude_string_t *string, const char *buf)
{
        return prelude_string_set_dup_fast(string, buf, strlen(buf));
}



int prelude_string_set_nodup_fast(prelude_string_t *string, char *buf, size_t len)
{
        if ( check_string(buf, len) < 0 )
                return -1;
                
        prelude_string_destroy_internal(string);

        string->index = len;
        string->size = len + 1;
        string->data.rwbuf = buf;
        
        string->flags |= PRELUDE_STRING_OWN_DATA;
        string->flags &= ~PRELUDE_STRING_CAN_REALLOC;

        return 0;       
}




int prelude_string_set_nodup(prelude_string_t *string, char *buf)
{
        return prelude_string_set_nodup_fast(string, buf, strlen(buf));
}




int prelude_string_set_ref_fast(prelude_string_t *string, const char *buf, size_t len)
{
        if ( check_string(buf, len) < 0 )
                return -1;
        
        prelude_string_destroy_internal(string);

        string->index = len;
        string->size = len + 1;
        string->data.robuf = buf;
 
        string->flags &= ~(PRELUDE_STRING_OWN_DATA|PRELUDE_STRING_CAN_REALLOC);

        return 0;
}



int prelude_string_set_ref(prelude_string_t *string, const char *buf)
{
        return prelude_string_set_ref_fast(string, buf, strlen(buf));
}



/*
 * just make a pointer copy of the embedded data
 */
int prelude_string_copy_ref(prelude_string_t *dst, const prelude_string_t *src)
{
        prelude_string_destroy_internal(dst);

        dst->size = src->size;
        dst->index = src->index;
        dst->data.robuf = src->data.robuf;
                
        dst->flags = src->flags & ~(PRELUDE_STRING_OWN_DATA|PRELUDE_STRING_CAN_REALLOC);
                
        return 0;
}



/*
 * also copy the content of the embedded data
 */
int prelude_string_copy_dup(prelude_string_t *dst, const prelude_string_t *src)
{
        prelude_string_destroy_internal(dst);

        dst->data.rwbuf = malloc(src->size);
        if ( ! dst->data.rwbuf )
                return -1;
        
        dst->size = src->size;
        dst->index = src->index;
        dst->flags |= PRELUDE_STRING_OWN_DATA|PRELUDE_STRING_CAN_REALLOC;
        
        string_buf_copy(dst, src->data.robuf, src->index);
                
        return 0;
}




prelude_string_t *prelude_string_clone(const prelude_string_t *string)
{
        prelude_string_t *ret;
        
        ret = prelude_string_new();
        if ( ! ret )
                return NULL;

        ret->data.rwbuf = malloc(string->size);
        if ( ! ret->data.rwbuf ) {
                free(ret);
                return NULL;
        }

        ret->size = string->size;
        ret->index = string->index;
        ret->flags |= PRELUDE_STRING_OWN_DATA|PRELUDE_STRING_CAN_REALLOC;

        string_buf_copy(ret, string->data.robuf, string->index);
        
        return ret;
}



size_t prelude_string_get_len(const prelude_string_t *string)
{
        return string->index;
}



const char *prelude_string_get_string_or_default(const prelude_string_t *string, const char *def)
{
        return string->data.robuf ? string->data.robuf : def;
}



const char *prelude_string_get_string(const prelude_string_t *string)
{
        return string->data.robuf;
}



char *prelude_string_get_string_released(prelude_string_t *string)
{
        char *ptr;

        ptr = prelude_realloc(string->data.rwbuf, string->index + 1);
        if ( ! ptr )
                return NULL;

        string->size = 0;
        string->index = 0;
        string->data.rwbuf = NULL;
        
        return ptr;
}



int prelude_string_is_empty(const prelude_string_t *string)
{
        return (string->index == 0);
}



/*
 *  This function cannot be declared static because its invoked
 *  from idmef-tree-wrap.c
 */
void prelude_string_destroy_internal(prelude_string_t *string)
{        
        if ( (string->flags & PRELUDE_STRING_OWN_DATA) && string->data.rwbuf ) {
                free(string->data.rwbuf);
                string->data.rwbuf = NULL;
        }

        /*
         * free() should be done by the caller
         */
}




void prelude_string_destroy(prelude_string_t *string)
{
        if ( --string->refcount )
                return;
        
        prelude_string_destroy_internal(string);

        if ( string->flags & PRELUDE_STRING_OWN_STRUCTURE )
                free(string);
}



int prelude_string_vprintf(prelude_string_t *s, const char *fmt, va_list ap)
{
        int ret;

        if ( ! (s->flags & PRELUDE_STRING_CAN_REALLOC) && allocate_more_chunk_if_needed(s, 0) < 0 )
                return -1;
        
        ret = vsnprintf(s->data.rwbuf + s->index, s->size - s->index, fmt, ap);
        
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

        if ( allocate_more_chunk_if_needed(s, (ret < 0) ? 0 : ret + 1) < 0 )
		return -1;
        
        return prelude_string_vprintf(s, fmt, ap);
}



int prelude_string_sprintf(prelude_string_t *string, const char *fmt, ...)
{
	int ret;
	va_list ap;
	
        va_start(ap, fmt);
        ret = prelude_string_vprintf(string, fmt, ap);
        va_end(ap);
                		
	return ret;
}



int prelude_string_ncat(prelude_string_t *s, const char *str, size_t len)
{        
	if ( s->flags & PRELUDE_STRING_CAN_REALLOC && len < s->size - s->index ) {
                
                memcpy(s->data.rwbuf + s->index, str, len);

                s->index += len;
                s->data.rwbuf[s->index] = '\0';

		return len;
	}
        
	if ( allocate_more_chunk_if_needed(s, len + 1) < 0 )
                return -1;

	return prelude_string_ncat(s, str, len);
}



int prelude_string_cat(prelude_string_t *string, const char *str)
{
	return prelude_string_ncat(string, str, strlen(str));
}



void prelude_string_clear(prelude_string_t *string)
{
        if ( string->data.rwbuf ) {
                *(string->data.rwbuf) = '\0';
                string->index = 0;
        }
}
