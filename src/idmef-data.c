/*****
*
* Copyright (C) 2003 Nicolas Delon <delon.nicolas@wanadoo.fr>
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
#include <string.h>
#include <inttypes.h>
#include <sys/types.h>

#include "idmef-data.h"


/*
 * String structure may be free'd
 */
#define IDMEF_DATA_OWN_STRUCTURE  0x1

/*
 * String data may be free'd
 */
#define IDMEF_DATA_OWN_DATA       0x2



/*
 * creates an empty data
 */

idmef_data_t *idmef_data_new(void)
{
        idmef_data_t *ret;

        ret = calloc(1, sizeof(*ret));
        if ( ! ret )
                return NULL;

	ret->flags |= IDMEF_DATA_OWN_STRUCTURE;

        return ret;
}



idmef_data_t *idmef_data_new_dup(const unsigned char *data, size_t len)
{
	idmef_data_t *ret;

	ret = idmef_data_new();
	if ( ! ret )
		return NULL;

	ret->data.rw_data = malloc(len);
	if ( ! ret->data.rw_data ) {
		free(ret);
		return NULL;
	}
        
	ret->len = len;
	ret->flags |= IDMEF_DATA_OWN_DATA;

        memcpy(ret->data.rw_data, data, ret->len);

	return ret;
}



idmef_data_t *idmef_data_new_nodup(unsigned char *data, size_t len)
{
	idmef_data_t *ret;

	ret = idmef_data_new();
	if ( ! ret )
		return NULL;
        
	ret->len = len;
	ret->data.rw_data = data;
        ret->flags |= IDMEF_DATA_OWN_DATA;
        
	return ret;
}




idmef_data_t *idmef_data_new_ref(const unsigned char *data, size_t len)
{
	idmef_data_t *ret;

	ret = idmef_data_new();
	if ( ! ret )
		return NULL;
	
	ret->len = len;
	ret->data.ro_data = data;

	return ret;
}




int idmef_data_set_dup(idmef_data_t *data, const unsigned char *buf, size_t len)
{
	idmef_data_destroy_internal(data);
        
	data->data.rw_data = malloc(len);
	if ( ! data->data.rw_data )
		return -1;
        
	data->len = len;
	data->flags |= IDMEF_DATA_OWN_DATA;
	memcpy(data->data.rw_data, buf, len);

	return 0;
}




int idmef_data_set_nodup(idmef_data_t *data, unsigned char *buf, size_t len)
{
	idmef_data_destroy_internal(data);

	data->len = len;
        data->data.rw_data = buf;
	data->flags |= IDMEF_DATA_OWN_DATA;

	return 0;	
}




int idmef_data_set_ref(idmef_data_t *data, const unsigned char *buf, size_t len)
{
        idmef_data_destroy_internal(data);
        
	data->len = len;
        data->data.ro_data = buf;
	data->flags &= ~IDMEF_DATA_OWN_DATA;

	return 0;
}




/*
 * just make a pointer copy of the embedded data
 */
int idmef_data_copy_ref(idmef_data_t *dst, const idmef_data_t *src)
{
        idmef_data_destroy_internal(dst);
        
	dst->len = src->len;
	dst->data = src->data;
	dst->flags &= ~IDMEF_DATA_OWN_DATA;
		
	return 0;
}



/*
 * also copy the content of the embedded data
 */
int idmef_data_copy_dup(idmef_data_t *dst, const idmef_data_t *src)
{
        idmef_data_destroy_internal(dst);
        
	dst->data.rw_data = malloc(src->len);
        if ( ! dst->data.rw_data )
                return -1;

        dst->len = src->len;
        dst->flags |= IDMEF_DATA_OWN_DATA;
        
        memcpy(dst->data.rw_data, src->data.ro_data, dst->len);
        
	return 0;
}




idmef_data_t *idmef_data_clone(const idmef_data_t *data)
{
	idmef_data_t *ret;
        
	ret = idmef_data_new();
	if ( ! ret )
		return NULL;

	ret->data.rw_data = malloc(data->len);
	if ( ! ret->data.rw_data ) {
                free(ret);
		return NULL;
        }
        
	ret->len = data->len;
        ret->flags |= IDMEF_DATA_OWN_DATA;
	memcpy(ret->data.rw_data, data->data.ro_data, ret->len);
	
	return ret;
}



size_t idmef_data_get_len(const idmef_data_t *data)
{
	return data->len;
}




unsigned char *idmef_data_get_data(const idmef_data_t *data)
{
	return data->data.rw_data;
}




int idmef_data_is_empty(const idmef_data_t *data)
{
	return (data->len == 0);
}




int idmef_data_to_string(idmef_data_t *data, char *buf, size_t size)
{
        int ret;
        unsigned char c;
        size_t i, outpos;

        i = outpos = 0;

        do {
		c = data->data.ro_data[i++];
                
		if ( outpos > size - 2 ) {
			buf[size - 1] = '\0';
			return -1;
		}

		if ( c >= 32 && c <= 127 ) {
			buf[outpos++] = c;
			continue;
		}

		switch (c) {

                case '\\':
			ret = snprintf(buf + outpos, size - outpos, "\\\\");
			break;

		case '\r':
			ret = snprintf(buf + outpos, size - outpos, "\\r");
			break;

		case '\n':
			ret = snprintf(buf + outpos, size - outpos, "\\n");
			break;

		case '\t':
			ret = snprintf(buf + outpos, size - outpos, "\\t");
			break;

		default:
			ret = snprintf(buf + outpos, size - outpos, "\\x%02x", c);
			break;
		}

		if ( ret < 0 || ret > size - outpos ) {
			buf[size - 1] = '\0';
			return -1;
		}

		outpos += ret;

	} while (i < data->len);

	return outpos;
}




/*
 *  This function cannot be declared static because its invoked
 *  from idmef-tree-wrap.c
 */
void idmef_data_destroy_internal(idmef_data_t *ptr)
{        
        if ( (ptr->flags & IDMEF_DATA_OWN_DATA) && ptr->data.rw_data ) {
                free(ptr->data.rw_data);
                ptr->data.rw_data = NULL;
        }

        /*
         * free() should be done by the caller
         */
}




void idmef_data_destroy(idmef_data_t *ptr)
{
        idmef_data_destroy_internal(ptr);
	
	if ( ptr->flags & IDMEF_DATA_OWN_STRUCTURE )
    		free(ptr);
}
