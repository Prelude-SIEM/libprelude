/*****
*
* Copyright (C) 2003 Yoann Vandoorselaere <yoann@prelude-ids.org>
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

#ifndef _IDMEF_VALUE_TYPE_H
#define _IDMEF_VALUE_TYPE_H


#define IDMEF_RELATION_EQUAL       0x01
#define IDMEF_RELATION_NOT_EQUAL   0x02
#define IDMEF_RELATION_LESSER      0x04
#define IDMEF_RELATION_GREATER     0x08
#define IDMEF_RELATION_SUBSTR      0x10
#define IDMEF_RELATION_REGEX       0x20
#define IDMEF_RELATION_IS_NULL	   0x40
#define IDMEF_RELATION_IS_NOT_NULL 0x80


typedef enum {
        relation_error = 0,
        relation_substring        = IDMEF_RELATION_SUBSTR,
        relation_regexp           = IDMEF_RELATION_REGEX,
        relation_greater          = IDMEF_RELATION_GREATER,
        relation_greater_or_equal = IDMEF_RELATION_GREATER|IDMEF_RELATION_EQUAL,
        relation_less             = IDMEF_RELATION_LESSER,
        relation_less_or_equal    = IDMEF_RELATION_LESSER|IDMEF_RELATION_EQUAL,
        relation_equal            = IDMEF_RELATION_EQUAL,
        relation_not_equal        = IDMEF_RELATION_NOT_EQUAL,
        relation_is_null          = IDMEF_RELATION_IS_NULL,
        relation_is_not_null      = IDMEF_RELATION_IS_NOT_NULL
} idmef_relation_t;



typedef enum {
	type_error = -1,
	type_unknown = 0,
	type_int16 = 1,
	type_uint16 = 2,
	type_int32 = 3,
	type_uint32 = 4,
	type_int64 = 5,
	type_uint64 = 6,
	type_float = 7,
	type_double = 8,
	type_string = 9,
	type_time = 10,
	type_data = 11,
        type_enum = 12,

        type_list = 99,
        type_object = 100,
} idmef_value_type_id_t;



typedef union {
        int16_t int16_val;
        uint16_t uint16_val;
        int32_t int32_val;
        uint32_t uint32_val;
        int64_t int64_val;
        uint64_t uint64_val;
        float float_val;
        double double_val;
        idmef_string_t *string_val;
        idmef_time_t *time_val;
        idmef_data_t *data_val;
        void *object_val;
        struct list_head list_val;
        int enum_val;
} idmef_value_type_data_t;


typedef struct {
        idmef_value_type_id_t id;
        idmef_value_type_data_t data;
} idmef_value_type_t;


int idmef_value_type_copy(void *dst, idmef_value_type_t *src);

int idmef_value_type_read(idmef_value_type_t *dst, const char *buf);

int idmef_value_type_write(char *buf, size_t size, idmef_value_type_t *src);

void idmef_value_type_destroy(idmef_value_type_t *type);

int idmef_value_type_clone(idmef_value_type_t *dst, idmef_value_type_t *src);

int idmef_value_type_compare(idmef_value_type_t *type1, idmef_value_type_t *type2, idmef_relation_t relation);


#endif
