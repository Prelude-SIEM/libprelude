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


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <netinet/in.h>

#include "libmissing.h"
#include "prelude-log.h"
#include "prelude-string.h"
#include "prelude-inttypes.h"
#include "idmef.h"
#include "idmef-tree-wrap.h"
#include "idmef-additional-data.h"
#include "idmef-message-id.h"


#define IDMEF_ADDITIONAL_DATA_DECL(d_type, d_name, ad_type, c_type, name)		\
idmef_additional_data_t *idmef_additional_data_new_ ## name(c_type val)			\
{											\
	idmef_additional_data_t *ptr;							\
											\
	ptr = idmef_additional_data_new();						\
	if ( ! ptr )									\
		return NULL;								\
											\
        idmef_additional_data_set_type(ptr, ad_type);					\
	idmef_data_set_ ## d_name(idmef_additional_data_get_data(ptr), val);		\
											\
	return ptr;									\
}											\
											\
void idmef_additional_data_set_ ## name(idmef_additional_data_t *ptr, c_type val)	\
{											\
        idmef_additional_data_set_type(ptr, ad_type);					\
	idmef_data_set_ ## d_name(idmef_additional_data_get_data(ptr), val);		\
}											\
											\
c_type idmef_additional_data_get_ ## name(idmef_additional_data_t *ptr)			\
{											\
	return (c_type) idmef_data_get_ ## d_name(idmef_additional_data_get_data(ptr));	\
}


IDMEF_ADDITIONAL_DATA_DECL(IDMEF_DATA_TYPE_FLOAT, float, IDMEF_ADDITIONAL_DATA_TYPE_REAL, float, real)
IDMEF_ADDITIONAL_DATA_DECL(IDMEF_DATA_TYPE_UINT32, uint32, IDMEF_ADDITIONAL_DATA_TYPE_INTEGER, uint32_t, integer)
IDMEF_ADDITIONAL_DATA_DECL(IDMEF_DATA_TYPE_BYTE, byte, IDMEF_ADDITIONAL_DATA_TYPE_BOOLEAN, prelude_bool_t, boolean)
IDMEF_ADDITIONAL_DATA_DECL(IDMEF_DATA_TYPE_CHAR, char, IDMEF_ADDITIONAL_DATA_TYPE_CHARACTER, char, character)


static struct {
	idmef_additional_data_type_t ad_type;
	idmef_data_type_t d_type;
	size_t len;
} idmef_additional_data_type_table[] = {
	{ IDMEF_ADDITIONAL_DATA_TYPE_BOOLEAN, IDMEF_DATA_TYPE_BYTE, sizeof(prelude_bool_t) },
	{ IDMEF_ADDITIONAL_DATA_TYPE_BYTE, IDMEF_DATA_TYPE_BYTE, sizeof(uint8_t) },
	{ IDMEF_ADDITIONAL_DATA_TYPE_CHARACTER, IDMEF_DATA_TYPE_CHAR, sizeof(char) },
	{ IDMEF_ADDITIONAL_DATA_TYPE_DATE_TIME, IDMEF_DATA_TYPE_CHAR_STRING, 0 },
	{ IDMEF_ADDITIONAL_DATA_TYPE_INTEGER, IDMEF_DATA_TYPE_UINT32, sizeof(uint32_t) },
	{ IDMEF_ADDITIONAL_DATA_TYPE_NTPSTAMP, IDMEF_DATA_TYPE_UINT64, sizeof(uint64_t) },
	{ IDMEF_ADDITIONAL_DATA_TYPE_PORTLIST, IDMEF_DATA_TYPE_CHAR_STRING, 0 },
	{ IDMEF_ADDITIONAL_DATA_TYPE_REAL, IDMEF_DATA_TYPE_FLOAT, sizeof(float) },
	{ IDMEF_ADDITIONAL_DATA_TYPE_STRING, IDMEF_DATA_TYPE_CHAR_STRING, 0 },
	{ IDMEF_ADDITIONAL_DATA_TYPE_BYTE_STRING, IDMEF_DATA_TYPE_BYTE_STRING, 0 },
	{ IDMEF_ADDITIONAL_DATA_TYPE_XML, IDMEF_DATA_TYPE_CHAR_STRING, 0 }
};



static int check_type(idmef_additional_data_type_t type, const unsigned char *buf, size_t len)
{
        if ( type < 0 || type > sizeof(idmef_additional_data_type_table) / sizeof(idmef_additional_data_type_table[0]) )
                return -1;

        if ( idmef_additional_data_type_table[type].len != 0 &&
	     len != idmef_additional_data_type_table[type].len )
                return -1;

        if ( idmef_additional_data_type_table[type].len == 0 && len < 1 )
                return -1;

        if ( type == IDMEF_ADDITIONAL_DATA_TYPE_BYTE_STRING )
                return 0;

        return buf[len - 1] == '\0' ? 0 : -1;
}



static idmef_data_type_t idmef_additional_data_type_to_data_type(idmef_additional_data_type_t type)
{
        if ( type < 0 || type > sizeof(idmef_additional_data_type_table) / sizeof(idmef_additional_data_type_table[0]) )
                return IDMEF_DATA_TYPE_UNKNOWN;

	return idmef_additional_data_type_table[type].d_type;
}



idmef_additional_data_t *idmef_additional_data_new_ptr_ref_fast(idmef_additional_data_type_t type,
                                                                const unsigned char *ptr, size_t len) 
{
        idmef_additional_data_t *new;

        if ( check_type(type, ptr, len) < 0 )
                return NULL;
        
        new = idmef_additional_data_new();
        if ( ! new )
                return NULL;

	idmef_additional_data_set_type(new, type);

	if ( idmef_data_set_ptr_ref_fast(idmef_additional_data_get_data(new), 
					 idmef_additional_data_type_to_data_type(type), ptr, len) < 0 ) {
		idmef_additional_data_destroy(new);
		return NULL;
	}

        return new;
}



idmef_additional_data_t *idmef_additional_data_new_ptr_dup_fast(idmef_additional_data_type_t type,
                                                                const unsigned char *ptr, size_t len) 
{
        idmef_additional_data_t *new;

        if ( check_type(type, ptr, len) < 0 )
                return NULL;
        
        new = idmef_additional_data_new();
        if ( ! new )
                return NULL;

	idmef_additional_data_set_type(new, type);

	if ( idmef_data_set_ptr_dup_fast(idmef_additional_data_get_data(new), 
					 idmef_additional_data_type_to_data_type(type), ptr, len) < 0 ) {
		idmef_additional_data_destroy(new);
		return NULL;
	}

        return new;
}



idmef_additional_data_t *idmef_additional_data_new_ptr_nodup_fast(idmef_additional_data_type_t type,
                                                                  unsigned char *ptr, size_t len) 
{
        idmef_additional_data_t *new;

        if ( check_type(type, ptr, len) < 0 )
                return NULL;
        
        new = idmef_additional_data_new();
        if ( ! new )
                return NULL;

	idmef_additional_data_set_type(new, type);

	if ( idmef_data_set_ptr_dup_fast(idmef_additional_data_get_data(new), 
					 idmef_additional_data_type_to_data_type(type), ptr, len) < 0 ) {
		idmef_additional_data_destroy(new);
		return NULL;
	}

        return new;
}



int idmef_additional_data_set_ptr_ref_fast(idmef_additional_data_t *data,
                                           idmef_additional_data_type_t type, const unsigned char *ptr, size_t len)
{
        if ( check_type(type, ptr, len) < 0 )
                return -1;

	idmef_additional_data_set_type(data, type);

	return idmef_data_set_ptr_ref_fast(idmef_additional_data_get_data(data),
					   idmef_additional_data_type_to_data_type(type), ptr, len);
}



int idmef_additional_data_set_ptr_dup_fast(idmef_additional_data_t *data,
                                           idmef_additional_data_type_t type, const unsigned char *ptr, size_t len)
{
        if ( check_type(type, ptr, len) < 0 )
                return -1;

	idmef_additional_data_set_type(data, type);

	return idmef_data_set_ptr_dup_fast(idmef_additional_data_get_data(data),
					   idmef_additional_data_type_to_data_type(type), ptr, len);
}



int idmef_additional_data_set_ptr_nodup_fast(idmef_additional_data_t *data,
                                             idmef_additional_data_type_t type, unsigned char *ptr, size_t len)
{
        if ( check_type(type, ptr, len) < 0 )
                return -1;

	idmef_additional_data_set_type(data, type);

	return idmef_data_set_ptr_nodup_fast(idmef_additional_data_get_data(data),
					     idmef_additional_data_type_to_data_type(type), ptr, len);
}



int idmef_additional_data_set_str_dup_fast(idmef_additional_data_t *data, idmef_additional_data_type_t type,
					   const char *str, size_t len)
{
	idmef_additional_data_set_type(data, type);

	return idmef_data_set_char_string_dup_fast(idmef_additional_data_get_data(data), str, len);
						   
}



idmef_additional_data_t *idmef_additional_data_new_str_dup_fast(idmef_additional_data_type_t type, const char *str, size_t len)
{
	idmef_additional_data_t *ad;

	ad = idmef_additional_data_new();
	if ( ! ad )
		return NULL;

	if ( idmef_additional_data_set_str_dup_fast(ad, type, str, len) < 0 ) {
		idmef_additional_data_destroy(ad);
		return NULL;
	}

	return ad;		
}


/*
 * just make a pointer copy of the embedded data
 */
int idmef_additional_data_copy_ref(idmef_additional_data_t *dst, const idmef_additional_data_t *src)
{
	if ( prelude_string_copy_ref(idmef_additional_data_get_meaning(dst), idmef_additional_data_get_meaning(src)) < 0 )
		return -1;

	idmef_additional_data_set_type(dst, idmef_additional_data_get_type(src));

	return idmef_data_copy_ref(idmef_additional_data_get_data(dst), idmef_additional_data_get_data(src));
}



/*
 * also copy the content of the embedded data
 */
int idmef_additional_data_copy_dup(idmef_additional_data_t *dst, const idmef_additional_data_t *src)
{
	if ( prelude_string_copy_dup(idmef_additional_data_get_meaning(dst), idmef_additional_data_get_meaning(src)) < 0 )
		return -1;

	idmef_additional_data_set_type(dst, idmef_additional_data_get_type(src));

	return idmef_data_copy_dup(idmef_additional_data_get_data(dst), idmef_additional_data_get_data(src));
}




idmef_additional_data_t *idmef_additional_data_clone(const idmef_additional_data_t *data)
{
        idmef_additional_data_t *ret;
        
        ret = idmef_additional_data_new();
        if ( ! ret )
                return NULL;

	if ( idmef_additional_data_copy_dup(ret, data) < 0 ) {
		idmef_additional_data_destroy(ret);
		return NULL;
	}

        return ret;
}



size_t idmef_additional_data_get_len(const idmef_additional_data_t *data)
{
        return idmef_data_get_len(idmef_additional_data_get_data(data));
}



int idmef_additional_data_is_empty(const idmef_additional_data_t *data)
{
        return idmef_data_is_empty(idmef_additional_data_get_data(data));
}



const char *idmef_additional_data_data_to_string(const idmef_additional_data_t *ad, char *buf, size_t *size)
{
        int ret = 0;
        idmef_data_t *data;

        data = idmef_additional_data_get_data(ad);
        if ( idmef_data_is_empty(data) )
                return "";

        switch ( idmef_additional_data_get_type(ad) ) {

	case IDMEF_ADDITIONAL_DATA_TYPE_NTPSTAMP: {
		uint64_t i;

		i = idmef_data_get_uint64(data);
		ret = snprintf(buf, *size, "0x%08ux.0x%08ux", (unsigned long) (i >> 32), (unsigned long) i);

		break;
	}

	default:
		ret = idmef_data_to_string(data, buf, *size);
	}

        if ( ret < 0 || ret >= *size )
                return NULL;

        *size = ret;

        return buf;
}
