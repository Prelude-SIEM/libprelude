/*****
*
* Copyright (C) 2003, 2004, 2005 PreludeIDS Technologies. All Rights Reserved.
* Author: Yoann Vandoorselaere <yoann.v@prelude-ids.com>
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

#ifndef _IDMEF_VALUE_TYPE_H
#define _IDMEF_VALUE_TYPE_H

#include "idmef-time.h"
#include "prelude-string.h"

#ifdef __cplusplus
 extern "C" {
#endif

typedef enum {
        IDMEF_VALUE_TYPE_ERROR   =  -1,
        IDMEF_VALUE_TYPE_UNKNOWN =   0,
        IDMEF_VALUE_TYPE_INT8    =   1,
        IDMEF_VALUE_TYPE_UINT8   =   2,
        IDMEF_VALUE_TYPE_INT16   =   3,
        IDMEF_VALUE_TYPE_UINT16  =   4,
        IDMEF_VALUE_TYPE_INT32   =   5,
        IDMEF_VALUE_TYPE_UINT32  =   6,
        IDMEF_VALUE_TYPE_INT64   =   7,
        IDMEF_VALUE_TYPE_UINT64  =   8,
        IDMEF_VALUE_TYPE_FLOAT   =   9,
        IDMEF_VALUE_TYPE_DOUBLE  =  10,
        IDMEF_VALUE_TYPE_STRING  =  11,
        IDMEF_VALUE_TYPE_TIME    =  12,
        IDMEF_VALUE_TYPE_DATA    =  13,
        IDMEF_VALUE_TYPE_ENUM    =  14,
        IDMEF_VALUE_TYPE_LIST    =  15,
        IDMEF_VALUE_TYPE_CLASS   =  16
} idmef_value_type_id_t;


typedef struct {
        void *object;
        int class_id;
} idmef_value_type_class_t;

typedef struct {
        int value;
        int class_id;
} idmef_value_type_enum_t;


typedef union {
        int8_t int8_val;
        uint8_t uint8_val;
        int16_t int16_val;
        uint16_t uint16_val;
        int32_t int32_val;
        uint32_t uint32_val;
        int64_t int64_val;
        uint64_t uint64_val;
        float float_val;
        double double_val;
        prelude_string_t *string_val;
        idmef_time_t *time_val;
        idmef_data_t *data_val;
        prelude_list_t list_val;
        idmef_value_type_enum_t enum_val;
        idmef_value_type_class_t class_val;
} idmef_value_type_data_t;


typedef struct {
        idmef_value_type_id_t id;
        idmef_value_type_data_t data;
} idmef_value_type_t;


#include "idmef-criteria.h"

int idmef_value_type_ref(const idmef_value_type_t *src);

int idmef_value_type_copy(const idmef_value_type_t *src, void *dst);

int idmef_value_type_read(idmef_value_type_t *dst, const char *buf);

int idmef_value_type_write(const idmef_value_type_t *src, prelude_string_t *out);

void idmef_value_type_destroy(idmef_value_type_t *type);

int idmef_value_type_clone(const idmef_value_type_t *src, idmef_value_type_t *dst);

int idmef_value_type_compare(const idmef_value_type_t *type1, const idmef_value_type_t *type2,
                             idmef_criterion_operator_t op);

int idmef_value_type_check_operator(idmef_value_type_id_t type, idmef_criterion_operator_t op);

int idmef_value_type_get_applicable_operators(idmef_value_type_id_t type, idmef_criterion_operator_t *result);

const char *idmef_value_type_to_string(idmef_value_type_id_t type);

#ifdef __cplusplus
 }
#endif

#endif
