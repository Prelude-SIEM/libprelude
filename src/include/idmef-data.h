/*****
*
* Copyright (C) 2003, 2004, 2005 PreludeIDS Technologies. All Rights Reserved.
* Author: Nicolas Delon <nicolas.delon@prelude-ids.com>
*
* This file is part of the Prelude library.
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

#ifndef _LIBPRELUDE_IDMEF_DATA_H
#define _LIBPRELUDE_IDMEF_DATA_H

#include "prelude-list.h"

#ifdef __cplusplus
 extern "C" {
#endif

typedef enum {
	IDMEF_DATA_TYPE_UNKNOWN      = 0,
	IDMEF_DATA_TYPE_CHAR         = 1,
	IDMEF_DATA_TYPE_BYTE         = 2,
	IDMEF_DATA_TYPE_UINT32       = 3,
	IDMEF_DATA_TYPE_UINT64       = 4,
	IDMEF_DATA_TYPE_FLOAT        = 5,
	IDMEF_DATA_TYPE_CHAR_STRING  = 6,
	IDMEF_DATA_TYPE_BYTE_STRING  = 7
} idmef_data_type_t;


typedef struct {
        int refcount;

        int flags;
        idmef_data_type_t type;
        size_t len;

        union {
		char char_data;
		uint8_t byte_data;
		uint32_t uint32_data;
		uint64_t uint64_data;
		float float_data;
		void *rw_data;
		const void *ro_data;
        } data;
        
        prelude_list_t list;
} idmef_data_t;



int idmef_data_new(idmef_data_t **data);

idmef_data_t *idmef_data_ref(idmef_data_t *data);


int idmef_data_new_char(idmef_data_t **data, char c);
int idmef_data_new_byte(idmef_data_t **data, uint8_t i);
int idmef_data_new_uint32(idmef_data_t **data, uint32_t i);
int idmef_data_new_uint64(idmef_data_t **data, uint64_t i);
int idmef_data_new_float(idmef_data_t **data, float f);


void idmef_data_set_char(idmef_data_t *data, char c);
void idmef_data_set_byte(idmef_data_t *data, uint8_t i);
void idmef_data_set_uint32(idmef_data_t *data, uint32_t i);
void idmef_data_set_uint64(idmef_data_t *data, uint64_t i);
void idmef_data_set_float(idmef_data_t *data, float f);

int idmef_data_set_ptr_dup_fast(idmef_data_t *data, idmef_data_type_t type, const void *ptr, size_t len);
int idmef_data_set_ptr_ref_fast(idmef_data_t *data, idmef_data_type_t type, const void *ptr, size_t len);
int idmef_data_set_ptr_nodup_fast(idmef_data_t *data, idmef_data_type_t type, void *ptr, size_t len);

int idmef_data_new_ptr_dup_fast(idmef_data_t **data, idmef_data_type_t type, const void *ptr, size_t len);         
int idmef_data_new_ptr_ref_fast(idmef_data_t **data, idmef_data_type_t type, const void *ptr, size_t len);
int idmef_data_new_ptr_nodup_fast(idmef_data_t **data, idmef_data_type_t type, void *ptr, size_t len);

         
/*
 * String functions
 */
int idmef_data_set_char_string_dup_fast(idmef_data_t *data, const char *str, size_t len);
int idmef_data_new_char_string_dup_fast(idmef_data_t **data, const char *str, size_t len);         
int idmef_data_new_char_string_ref_fast(idmef_data_t **data, const char *ptr, size_t len);
int idmef_data_new_char_string_nodup_fast(idmef_data_t **data, char *ptr, size_t len);
int idmef_data_set_char_string_ref_fast(idmef_data_t *data, const char *ptr, size_t len);
int idmef_data_set_char_string_nodup_fast(idmef_data_t *data, char *ptr, size_t len);
int idmef_data_new_char_string_ref(idmef_data_t **data, const char *ptr);
int idmef_data_new_char_string_dup(idmef_data_t **data, const char *ptr);
int idmef_data_new_char_string_nodup(idmef_data_t **data, char *ptr);
int idmef_data_set_char_string_ref(idmef_data_t *data, const char *ptr);
int idmef_data_set_char_string_dup(idmef_data_t *data, const char *ptr);
int idmef_data_set_char_string_nodup(idmef_data_t *data, char *ptr);

#define idmef_data_set_char_string_constant(string, str)		\
	idmef_data_set_char_string_ref_fast((string), (str), sizeof((str)) - 1)

         
/*
 * Byte functions
 */
int idmef_data_new_byte_string_ref(idmef_data_t **data, const unsigned char *ptr, size_t len);
int idmef_data_new_byte_string_dup(idmef_data_t **data, const unsigned char *ptr, size_t len);
int idmef_data_new_byte_string_nodup(idmef_data_t **data, unsigned char *ptr, size_t len);
int idmef_data_set_byte_string_ref(idmef_data_t *data, const unsigned char *ptr, size_t len);
int idmef_data_set_byte_string_dup(idmef_data_t *data, const unsigned char *ptr, size_t len);
int idmef_data_set_byte_string_nodup(idmef_data_t *data, unsigned char *ptr, size_t len);



/*
 *
 */
         
void idmef_data_destroy(idmef_data_t *data);

int idmef_data_copy_ref(const idmef_data_t *src, idmef_data_t *dst);

int idmef_data_copy_dup(const idmef_data_t *src, idmef_data_t *dst);

int idmef_data_clone(const idmef_data_t *src, idmef_data_t **dst);

idmef_data_type_t idmef_data_get_type(const idmef_data_t *data);

size_t idmef_data_get_len(const idmef_data_t *data);

const void *idmef_data_get_data(const idmef_data_t *data);

char idmef_data_get_char(const idmef_data_t *data);

uint8_t idmef_data_get_byte(const idmef_data_t *data);

uint32_t idmef_data_get_uint32(const idmef_data_t *data);

uint64_t idmef_data_get_uint64(const idmef_data_t *data);

float idmef_data_get_float(const idmef_data_t *data);

const char *idmef_data_get_char_string(const idmef_data_t *data);

const unsigned char *idmef_data_get_byte_string(const idmef_data_t *data);

prelude_bool_t idmef_data_is_empty(const idmef_data_t *data);

int idmef_data_to_string(const idmef_data_t *data, prelude_string_t *out);

void idmef_data_destroy_internal(idmef_data_t *data);

int idmef_data_compare(const idmef_data_t *data1, const idmef_data_t *data2);
         
#ifdef __cplusplus
 }
#endif
         
#endif /* _LIBPRELUDE_IDMEF_DATA_H */
