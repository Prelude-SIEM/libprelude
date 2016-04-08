/*****
*
* Copyright (C) 2003-2016 CS-SI. All Rights Reserved.
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
* You should have received a copy of the GNU General Public License along
* with this program; if not, write to the Free Software Foundation, Inc.,
* 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*
*****/

#include "libmissing.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

#include "common.h"
#include "prelude-inttypes.h"
#include "prelude-string.h"

#define PRELUDE_ERROR_SOURCE_DEFAULT PRELUDE_ERROR_SOURCE_IDMEF_VALUE_TYPE
#include "prelude-error.h"
#include "prelude-inttypes.h"
#include "prelude-linked-object.h"
#include "idmef-object-prv.h"

#include "idmef-time.h"
#include "idmef-data.h"
#include "idmef-value-type.h"


#define CLASS_OPERATOR  IDMEF_CRITERION_OPERATOR_NULL|IDMEF_CRITERION_OPERATOR_NOT| \
                        IDMEF_CRITERION_OPERATOR_EQUAL

#define DATA_OPERATOR    IDMEF_CRITERION_OPERATOR_EQUAL|IDMEF_CRITERION_OPERATOR_NOT| \
                         IDMEF_CRITERION_OPERATOR_LESSER|IDMEF_CRITERION_OPERATOR_GREATER|IDMEF_CRITERION_OPERATOR_SUBSTR

#define TIME_OPERATOR    IDMEF_CRITERION_OPERATOR_LESSER|IDMEF_CRITERION_OPERATOR_GREATER| \
                         IDMEF_CRITERION_OPERATOR_EQUAL|IDMEF_CRITERION_OPERATOR_NOT

#define STRING_OPERATOR  IDMEF_CRITERION_OPERATOR_SUBSTR|IDMEF_CRITERION_OPERATOR_EQUAL| \
                         IDMEF_CRITERION_OPERATOR_NOT|IDMEF_CRITERION_OPERATOR_NOCASE

#define INTEGER_OPERATOR IDMEF_CRITERION_OPERATOR_LESSER|IDMEF_CRITERION_OPERATOR_GREATER|\
                         IDMEF_CRITERION_OPERATOR_EQUAL|IDMEF_CRITERION_OPERATOR_NOT

#define ENUM_OPERATOR    STRING_OPERATOR|INTEGER_OPERATOR


#define GENERIC_ONE_BASE_RW_FUNC(scanfmt, printfmt, name, type)                            \
        static int name ## _read(idmef_value_type_t *dst, const char *buf)                 \
        {                                                                                  \
                char *endptr;                                                              \
                                                                                           \
                (dst)->data. name ##_val = strtod(buf, &endptr);                           \
                if ( buf == endptr || *endptr != '\0' || errno == ERANGE )                 \
                        return prelude_error_verbose(PRELUDE_ERROR_IDMEF_VALUE_TYPE_PARSE, \
                        "Reading " #name " value failed with '%s'", buf);                  \
                return 1;                                                                  \
        }                                                                                  \
                                                                                           \
        static int name ## _write(const idmef_value_type_t *src, prelude_string_t *out)    \
        {                                                                                  \
                return prelude_string_sprintf(out, (printfmt), src->data.name ##_val);     \
        }


#define GENERIC_TWO_BASES_RW_FUNC(rfunc, fmt_dec, name, type, min, max)                              \
        static int name ## _read(idmef_value_type_t *dst, const char *buf)                           \
        {                                                                                            \
                type tmp;                                                                            \
                char *endptr;                                                                        \
                                                                                                     \
                tmp = rfunc(buf, &endptr, 0);                                                        \
                if ( buf == endptr || *endptr != '\0' || tmp < min || tmp > max || errno == ERANGE ) \
                        return prelude_error_verbose(PRELUDE_ERROR_IDMEF_VALUE_TYPE_PARSE,           \
                        "Value out of range, required: [" # min " - " # max "], got '%s'", buf);     \
                                                                                                     \
                dst->data.name ##_val = tmp;                                                         \
                return 0;                                                                            \
        }                                                                                            \
                                                                                                     \
        static int name ## _write(const idmef_value_type_t *src, prelude_string_t *out)              \
        {                                                                                            \
                return prelude_string_sprintf(out, (fmt_dec), src->data.name ##_val);                \
        }



typedef struct {
        const char *name;

        size_t len;

        idmef_criterion_operator_t operator;

        int (*copy)(const idmef_value_type_t *src, void *dst, size_t size);
        int (*clone)(const idmef_value_type_t *src, idmef_value_type_t *dst, size_t size);
        int (*ref)(const idmef_value_type_t *src);

        void (*destroy)(idmef_value_type_t *type);
        int (*compare)(const idmef_value_type_t *t1, const idmef_value_type_t *t2, size_t size, idmef_criterion_operator_t op);

        int (*read)(idmef_value_type_t *dst, const char *buf);
        int (*write)(const idmef_value_type_t *src, prelude_string_t *out);

} idmef_value_type_operation_t;



/*
 * We specify a type bigger than the one handled, in order to catch min/max error.
 */
GENERIC_TWO_BASES_RW_FUNC(strtol, "%d", int8, int, PRELUDE_INT8_MIN, PRELUDE_INT8_MAX)
GENERIC_TWO_BASES_RW_FUNC(strtol, "%d", uint8, unsigned int, 0, PRELUDE_UINT8_MAX)
GENERIC_TWO_BASES_RW_FUNC(strtol, "%hd", int16, int, PRELUDE_INT16_MIN, PRELUDE_INT16_MAX)
GENERIC_TWO_BASES_RW_FUNC(strtoul, "%hu", uint16, unsigned int, 0, PRELUDE_UINT16_MAX)
GENERIC_TWO_BASES_RW_FUNC(strtol, "%d", int32, int64_t, PRELUDE_INT32_MIN, PRELUDE_INT32_MAX)
GENERIC_TWO_BASES_RW_FUNC(strtoul, "%u", uint32, uint64_t, 0, PRELUDE_UINT32_MAX)
GENERIC_TWO_BASES_RW_FUNC(strtoll, "%" PRELUDE_PRId64, int64, int64_t, PRELUDE_INT64_MIN, PRELUDE_INT64_MAX)
GENERIC_TWO_BASES_RW_FUNC(strtoull, "%" PRELUDE_PRIu64, uint64, uint64_t, 0, PRELUDE_UINT64_MAX)

GENERIC_ONE_BASE_RW_FUNC("%f", "%f", float, float)
GENERIC_ONE_BASE_RW_FUNC("%lf", "%f", double, double)




/*
 * generic functions.
 */
static int charstring_compare(const char *s1, const char *s2, idmef_criterion_operator_t op)
{
        if ( (s1 && !s2) || (s2 && !s1) )
                return -1;

        if ( op == (IDMEF_CRITERION_OPERATOR_EQUAL|IDMEF_CRITERION_OPERATOR_NOCASE) && strcasecmp(s1, s2) == 0 )
                return 0;

        else if ( op == IDMEF_CRITERION_OPERATOR_EQUAL && strcmp(s1, s2) == 0 )
                return 0;

        else if ( op == (IDMEF_CRITERION_OPERATOR_SUBSTR|IDMEF_CRITERION_OPERATOR_NOCASE) && strcasestr(s1, s2) )
                return 0;

        else if ( op == IDMEF_CRITERION_OPERATOR_SUBSTR && strstr(s1, s2) )
                return 0;

        return -1;
}



static int generic_copy(const idmef_value_type_t *src, void *dst, size_t size)
{
        memcpy(dst, &src->data, size);
        return 0;
}




static int generic_clone(const idmef_value_type_t *src, idmef_value_type_t *dst, size_t size)
{
        memcpy(&dst->data, &src->data, size);
        return 0;
}



static int generic_compare(const idmef_value_type_t *t1, const idmef_value_type_t *t2,
                           size_t size, idmef_criterion_operator_t op)
{
        int ret;

        if ( (t1 && ! t2) || (t2 && ! t1) )
                return -1;

        ret = memcmp(&t1->data, &t2->data, size);

        if ( ret == 0 && op & IDMEF_CRITERION_OPERATOR_EQUAL )
                return 0;

        if ( ret < 0 && op & IDMEF_CRITERION_OPERATOR_LESSER )
                return 0;

        if ( ret > 0 && op & IDMEF_CRITERION_OPERATOR_GREATER )
                return 0;

        return -1;
}



/*
 * Enum specific
 */
static int enum_copy(const idmef_value_type_t *src, void *dst, size_t size)
{
        *(int *)dst = src->data.enum_val.value;
        return 0;
}


static int enum_read(idmef_value_type_t *dst, const char *buf)
{
        int ret;

        ret = sscanf(buf, "%d", &(dst)->data.enum_val.value);

        return (ret == 1) ? 0 : prelude_error_verbose(PRELUDE_ERROR_IDMEF_VALUE_TYPE_PARSE, "Reading enum value failed");
}



static int enum_write(const idmef_value_type_t *src, prelude_string_t *out)
{
        const char *str;

        str = idmef_class_enum_to_string(src->data.enum_val.class_id, src->data.enum_val.value);
        if ( ! str )
                return prelude_error_verbose(PRELUDE_ERROR_IDMEF_VALUE_TYPE_PARSE, "Enumeration conversion from numeric to string failed");

        return prelude_string_cat(out, str);
}


static int enum_compare(const idmef_value_type_t *src, const idmef_value_type_t *dst, size_t size, idmef_criterion_operator_t op)
{
        const char *s1;

        if ( (src && ! dst) || (dst && ! src) )
                return -1;

        if ( dst->id == IDMEF_VALUE_TYPE_STRING ) {
                s1 = idmef_class_enum_to_string(src->data.enum_val.class_id, src->data.enum_val.value);
                if ( ! s1 )
                        return prelude_error_verbose(PRELUDE_ERROR_IDMEF_VALUE_TYPE_PARSE, "Enumeration conversion from numeric to string failed");

                return charstring_compare(s1, prelude_string_get_string(dst->data.string_val), op);
        }

        return generic_compare(src, dst, size, op);
}


/*
 * time specific function.
 */
static int time_compare(const idmef_value_type_t *t1, const idmef_value_type_t *t2,
                        size_t size, idmef_criterion_operator_t op)
{
        int ret;

        if ( (t1 && ! t2) || (t2 && ! t1) )
                return -1;

        ret = idmef_time_compare(t1->data.time_val, t2->data.time_val);
        if ( op & IDMEF_CRITERION_OPERATOR_EQUAL && ret == 0 )
                return 0;

        else if ( op & IDMEF_CRITERION_OPERATOR_LESSER && ret < 0 )
                return 0;

        else if ( op & IDMEF_CRITERION_OPERATOR_GREATER && ret > 0 )
                return 0;

        return -1;
}



static int time_read(idmef_value_type_t *dst, const char *buf)
{
        int ret;

        ret = idmef_time_new_from_ntpstamp(&dst->data.time_val, buf);
        if ( ret == 0 )
                return 0;

        ret = idmef_time_new_from_string(&dst->data.time_val, buf);
        if ( ret == 0 )
                return 0;

        return prelude_error_verbose(PRELUDE_ERROR_IDMEF_VALUE_TYPE_PARSE,
                                     "Invalid time format specified: '%s'", buf);
}



static int time_write(const idmef_value_type_t *src, prelude_string_t *out)
{
        return idmef_time_to_string(src->data.time_val, out);
}



static int time_copy(const idmef_value_type_t *src, void *dst, size_t size)
{
        return idmef_time_copy(src->data.time_val, dst);
}



static int time_clone(const idmef_value_type_t *src, idmef_value_type_t *dst, size_t size)
{
        return idmef_time_clone(src->data.time_val, &dst->data.time_val);
}


static int time_ref(const idmef_value_type_t *src)
{
        idmef_time_ref(src->data.time_val);
        return 0;
}


static void time_destroy(idmef_value_type_t *type)
{
        idmef_time_destroy(type->data.time_val);
}



/*
 *
 */
static int string_compare(const idmef_value_type_t *t1, const idmef_value_type_t *t2,
                          size_t size, idmef_criterion_operator_t op)
{
        const char *s1 = NULL, *s2 = NULL;

        if ( t1 && t1->data.string_val )
                s1 = prelude_string_get_string(t1->data.string_val);

        if ( t2 && t2->data.string_val )
                s2 = prelude_string_get_string(t2->data.string_val);

        return charstring_compare(s1, s2, op);
}



static int string_read(idmef_value_type_t *dst, const char *buf)
{
        return prelude_string_new_dup(&dst->data.string_val, buf);
}



static int string_copy(const idmef_value_type_t *src, void *dst, size_t size)
{
        return prelude_string_copy_dup(src->data.string_val, dst);
}


static int string_ref(const idmef_value_type_t *src)
{
        prelude_string_ref(src->data.string_val);
        return 0;
}


static int string_clone(const idmef_value_type_t *src, idmef_value_type_t *dst, size_t size)
{
        return prelude_string_clone(src->data.string_val, &dst->data.string_val);
}


static void string_destroy(idmef_value_type_t *type)
{
        prelude_string_destroy(type->data.string_val);
}



static int string_write(const idmef_value_type_t *src, prelude_string_t *out)
{
        return prelude_string_sprintf(out, "%s",
                       prelude_string_get_string_or_default(src->data.string_val, "<empty>"));
}



/*
 * data specific functions
 */
static int data_compare(const idmef_value_type_t *t1, const idmef_value_type_t *t2,
                        size_t len, idmef_criterion_operator_t op)
{
        int ret;
        size_t s1_len, s2_len;
        const void *s1 = NULL, *s2 = NULL;

        if ( t1 && t1->data.data_val )
                s1 = idmef_data_get_data(t1->data.data_val);

        if ( t2 && t2->data.string_val )
                s2 = idmef_data_get_data(t2->data.data_val);

        if ( (s1 && !s2) || (s2 && !s1) )
                return -1;

        if ( op & IDMEF_CRITERION_OPERATOR_SUBSTR ) {
                s1_len = idmef_data_get_len(t1->data.data_val);
                s2_len = idmef_data_get_len(t2->data.data_val);
                return ( memmem(s1, s1_len, s2, s2_len) ) ? 0 : -1;
        }

        ret = idmef_data_compare(t1->data.data_val, t2->data.data_val);
        if ( ret == 0 && op & IDMEF_CRITERION_OPERATOR_EQUAL )
                return 0;

        else if ( ret < 0 && op & IDMEF_CRITERION_OPERATOR_LESSER )
                return 0;

        else if ( ret > 0 && op & IDMEF_CRITERION_OPERATOR_GREATER )
                return 0;


        return -1;
}



static int data_read(idmef_value_type_t *dst, const char *src)
{
        return idmef_data_new_char_string_dup_fast(&dst->data.data_val, src, strlen(src));
}



static int data_write(const idmef_value_type_t *src, prelude_string_t *out)
{
        return idmef_data_to_string(src->data.data_val, out);
}



static int data_copy(const idmef_value_type_t *src, void *dst, size_t size)
{
        return idmef_data_copy_dup(src->data.data_val, dst);
}



static int data_clone(const idmef_value_type_t *src, idmef_value_type_t *dst, size_t size)
{
        return idmef_data_clone(src->data.data_val, &dst->data.data_val);
}


static int data_ref(const idmef_value_type_t *src)
{
        idmef_data_ref(src->data.data_val);
        return 0;
}


static void data_destroy(idmef_value_type_t *type)
{
        idmef_data_destroy(type->data.data_val);
}



/*
 * class specific
 */
static int class_compare(const idmef_value_type_t *c1,
                         const idmef_value_type_t *c2, size_t len, idmef_criterion_operator_t op)
{
        if ( (c1 && !c2) || (c2 && !c1) )
                return -1;

        return idmef_class_compare(c1->data.class_val.class_id,
                                   c1->data.class_val.object, c2->data.class_val.object);
}


static int class_copy(const idmef_value_type_t *src, void *dst, size_t size)
{
        return idmef_class_copy(src->data.class_val.class_id, src->data.class_val.object, dst);
}


static int class_clone(const idmef_value_type_t *src, idmef_value_type_t *dst, size_t size)
{
        dst->data.class_val.class_id = src->data.class_val.class_id;
        return idmef_class_clone(src->data.class_val.class_id, src->data.class_val.object, &dst->data.class_val.object);
}


static int class_ref(const idmef_value_type_t *src)
{
        return idmef_class_ref(src->data.class_val.class_id, src->data.class_val.object);
}


static void class_destroy(idmef_value_type_t *type)
{
        idmef_class_destroy(type->data.class_val.class_id, type->data.class_val.object);
}



static int list_to_string(const char *cname, idmef_class_id_t class, prelude_list_t *head, prelude_string_t *out,
                          int (*item_to_string_cb)(int class, void *item, prelude_string_t *out))
{
        void *obj;
        int ret, j;
        prelude_list_t *tmp;

        if ( prelude_list_is_empty(head) )
                return 0;

        if ( cname )
                ret = prelude_string_sprintf(out, " %s:", cname);
        else
                ret = prelude_string_cat(out, " ");

        if ( ret < 0 )
                return ret;

        ret = prelude_string_cat(out, "(");
        if ( ret < 0 )
                return ret;

        j = 0;
        prelude_list_for_each(head, tmp) {
                if ( j++ > 0 ) {
                        ret = prelude_string_cat(out, ", ");
                        if ( ret < 0 )
                                return ret;
                }

                if ( ! cname )
                        obj = idmef_linked_object_get_object(tmp);
                else
                        obj = prelude_linked_object_get_object(tmp);

                ret = item_to_string_cb(class, obj, out);
                if ( ret < 0 )
                        return ret;
        }

        return prelude_string_cat(out, ")");
}



static int listed_str_to_string(idmef_class_id_t parent_class, void *parent, prelude_string_t *out)
{
        return prelude_string_cat(out, prelude_string_get_string(parent));
}


static int class_to_string(idmef_class_id_t parent_class, void *parent, prelude_string_t *out)
{
        int ret, i;
        void *childptr;
        idmef_class_id_t childclass;
        idmef_value_type_id_t vtype;
        const char *pname = idmef_class_get_name(parent_class);

        ret = prelude_string_sprintf(out, "<IDMEF%c%s",  toupper(*pname), pname + 1);
        if ( ret < 0 )
                return ret;

        for ( i = 0; ; i++ ) {
                ret = idmef_class_get_child(parent, parent_class, i, &childptr);
                if ( ret < 0 )
                        break;

                if ( ! childptr )
                        continue;

                vtype = idmef_class_get_child_value_type(parent_class, i);
                childclass = idmef_class_get_child_class(parent_class, i);

                if ( vtype != IDMEF_VALUE_TYPE_CLASS ) {
                        const char *cname = idmef_class_get_child_name(parent_class, i);

                        if ( ! idmef_class_is_child_list(parent_class, i) ) {
                                ret = prelude_string_sprintf(out, " %s:", cname);
                                if ( ret < 0 ) {
                                        idmef_value_destroy(childptr);
                                        return ret;
                                }

                                ret = idmef_value_to_string(childptr, out);
                                idmef_value_destroy(childptr);
                                if ( ret < 0 )
                                        return ret;
                        }

                        else {
                                ret = list_to_string(cname, childclass, childptr, out, listed_str_to_string);
                                if ( ret < 0 )
                                        return ret;
                        }

                        continue;
                }

                if ( ! idmef_class_is_child_list(parent_class, i) ) {
                        ret = prelude_string_cat(out, " ");
                        if ( ret < 0 )
                                return ret;

                        ret = class_to_string(childclass, childptr, out);
                        if ( ret < 0 )
                                return ret;
                }

                else {
                        ret = list_to_string(NULL, childclass, childptr, out, class_to_string);
                        if ( ret < 0 )
                                return ret;
                }
        }

        return prelude_string_cat(out, ">");
}


static int class_write(const idmef_value_type_t *src, prelude_string_t *out)
{
        return class_to_string(src->data.class_val.class_id, src->data.class_val.object, out);
}


static const idmef_value_type_operation_t ops_tbl[] = {
        { "unknown", 0, 0, NULL, NULL, NULL, NULL, NULL, NULL, NULL                     },
        { "int8", sizeof(int8_t), INTEGER_OPERATOR, generic_copy,
          generic_clone, NULL, NULL, generic_compare, int8_read, int8_write             },
        { "uint8", sizeof(uint8_t), INTEGER_OPERATOR, generic_copy,
          generic_clone, NULL, NULL, generic_compare, uint8_read, uint8_write           },
        { "int16", sizeof(int16_t), INTEGER_OPERATOR, generic_copy,
          generic_clone, NULL, NULL, generic_compare, int16_read, int16_write           },
        { "uint16", sizeof(uint16_t), INTEGER_OPERATOR, generic_copy,
          generic_clone, NULL, NULL, generic_compare, uint16_read, uint16_write         },
        { "int32", sizeof(int32_t), INTEGER_OPERATOR, generic_copy,
          generic_clone, NULL, NULL, generic_compare, int32_read, int32_write           },
        { "uint32", sizeof(uint32_t), INTEGER_OPERATOR, generic_copy,
          generic_clone, NULL, NULL, generic_compare, uint32_read, uint32_write         },
        { "int64", sizeof(int64_t), INTEGER_OPERATOR, generic_copy,
          generic_clone, NULL, NULL, generic_compare, int64_read, int64_write           },
        { "uint64", sizeof(uint64_t), INTEGER_OPERATOR, generic_copy,
          generic_clone, NULL, NULL, generic_compare, uint64_read, uint64_write         },
        { "float", sizeof(float), INTEGER_OPERATOR, generic_copy,
          generic_clone, NULL, NULL, generic_compare, float_read, float_write           },
        { "double", sizeof(double), INTEGER_OPERATOR, generic_copy,
          generic_clone, NULL, NULL, generic_compare, double_read, double_write         },
        { "string", 0, STRING_OPERATOR, string_copy, string_clone,
          string_ref, string_destroy, string_compare, string_read, string_write         },
        { "time", 0, TIME_OPERATOR, time_copy, time_clone,
          time_ref, time_destroy, time_compare, time_read, time_write                   },
        { "data", 0, DATA_OPERATOR, data_copy, data_clone,
          data_ref, data_destroy, data_compare, data_read, data_write                   },
        { "enum", sizeof(idmef_value_type_enum_t), ENUM_OPERATOR, enum_copy,
          generic_clone, NULL, NULL, enum_compare, enum_read, enum_write,               },
        { "list", 0, 0, NULL, NULL, NULL, NULL, NULL, NULL, NULL                        },
        { "class", 0, CLASS_OPERATOR, class_copy, class_clone,
          class_ref, class_destroy, class_compare, NULL, class_write                    },
};



static int is_type_valid(idmef_value_type_id_t type)
{
        if ( type < 0 || (size_t) type >= (sizeof(ops_tbl) / sizeof(*ops_tbl)) )
                return prelude_error_verbose(PRELUDE_ERROR_IDMEF_VALUE_TYPE_UNKNOWN, "Unknown IDMEF type id: '%d'", type);

        return 0;
}



const char *idmef_value_type_to_string(idmef_value_type_id_t type)
{
        int ret;

        ret = is_type_valid(type);
        if ( ret < 0 )
                return NULL;

        return ops_tbl[type].name;
}



int idmef_value_type_clone(const idmef_value_type_t *src, idmef_value_type_t *dst)
{
        int ret;

        assert(dst->id == src->id);

        ret = is_type_valid(dst->id);
        if ( ret < 0 )
                return ret;

        if ( ! ops_tbl[dst->id].clone )
                return prelude_error_verbose(PRELUDE_ERROR_IDMEF_VALUE_TYPE_CLONE_UNAVAILABLE,
                                             "Object type '%s' does not support clone operation",
                                             idmef_value_type_to_string(dst->id));

        return ops_tbl[dst->id].clone(src, dst, ops_tbl[dst->id].len);
}




int idmef_value_type_copy(const idmef_value_type_t *src, void *dst)
{
        int ret;

        ret = is_type_valid(src->id);
        if ( ret < 0 )
                return ret;

        if ( ! ops_tbl[src->id].copy )
                return prelude_error_verbose(PRELUDE_ERROR_IDMEF_VALUE_TYPE_COPY_UNAVAILABLE,
                                             "Object type '%s' does not support copy operation",
                                             idmef_value_type_to_string(src->id));

        return ops_tbl[src->id].copy(src, dst, ops_tbl[src->id].len);
}



int idmef_value_type_ref(const idmef_value_type_t *vt)
{
        int ret;

        ret = is_type_valid(vt->id);
        if ( ret < 0 )
                return ret;

        if ( ! ops_tbl[vt->id].ref )
                return prelude_error_verbose(PRELUDE_ERROR_IDMEF_VALUE_TYPE_REF_UNAVAILABLE,
                                             "Object type '%s' does not support ref operation",
                                             idmef_value_type_to_string(vt->id));

        return ops_tbl[vt->id].ref(vt);
}


int idmef_value_type_compare(const idmef_value_type_t *type1,
                             const idmef_value_type_t *type2,
                             idmef_criterion_operator_t op)
{
        int ret;
        idmef_value_type_id_t tid;

        prelude_return_val_if_fail(type1 || type2, prelude_error(PRELUDE_ERROR_ASSERTION));
        tid = (type1) ? type1->id : type2->id;

        ret = is_type_valid(tid);
        if ( ret < 0 )
                return ret;

        if ( type1 && type2 && type1->id != type2->id ) {
                if ( type1->id != IDMEF_VALUE_TYPE_ENUM && type2->id != IDMEF_VALUE_TYPE_STRING )
                        return prelude_error(PRELUDE_ERROR_IDMEF_VALUE_TYPE_COMPARE_MISMATCH);
        }

        if ( ! (op & ops_tbl[tid].operator) )
                return prelude_error_verbose(PRELUDE_ERROR_IDMEF_VALUE_TYPE_COMPARE_UNAVAILABLE,
                                             "Object type '%s' does not support operator '%s'",
                                             idmef_value_type_to_string(tid), idmef_criterion_operator_to_string(op));

        if ( ! ops_tbl[tid].compare )
                return prelude_error_verbose(PRELUDE_ERROR_IDMEF_VALUE_TYPE_COMPARE_UNAVAILABLE,
                                             "Object type '%s' does not support compare operation",
                                             idmef_value_type_to_string(tid));

        ret = ops_tbl[tid].compare(type1, type2, ops_tbl[tid].len, op & ~IDMEF_CRITERION_OPERATOR_NOT);
        if ( ret < 0 ) /* not an error -> no match */
                ret = 1;

        if ( op & IDMEF_CRITERION_OPERATOR_NOT )
                return (ret == 0) ? 1 : 0;
        else
                return ret;
}




int idmef_value_type_read(idmef_value_type_t *dst, const char *buf)
{
        int ret;

        ret = is_type_valid(dst->id);
        if ( ret < 0 )
                return ret;

        if ( ! ops_tbl[dst->id].read )
                return prelude_error_verbose(PRELUDE_ERROR_IDMEF_VALUE_TYPE_READ_UNAVAILABLE,
                                             "Object type '%s' does not support read operation",
                                             idmef_value_type_to_string(dst->id));

        ret = ops_tbl[dst->id].read(dst, buf);
        return (ret < 0) ? ret : 0;
}




int idmef_value_type_write(const idmef_value_type_t *src, prelude_string_t *out)
{
        int ret;

        ret = is_type_valid(src->id);
        if ( ret < 0 )
                return ret;

        if ( ! ops_tbl[src->id].write )
                return prelude_error_verbose(PRELUDE_ERROR_IDMEF_VALUE_TYPE_WRITE_UNAVAILABLE,
                                             "Object type '%s' does not support write operation",
                                             idmef_value_type_to_string(src->id));

        return ops_tbl[src->id].write(src, out);
}



void idmef_value_type_destroy(idmef_value_type_t *type)
{
        int ret;

        ret = is_type_valid(type->id);
        if ( ret < 0 )
                return;

        if ( ! ops_tbl[type->id].destroy )
                return;

        ops_tbl[type->id].destroy(type);
}




int idmef_value_type_check_operator(idmef_value_type_id_t type, idmef_criterion_operator_t op)
{
        int ret;

        ret = is_type_valid(type);
        if ( ret < 0 )
                return ret;

        if ( (~ops_tbl[type].operator & op) == 0 )
                return 0;

        return prelude_error_verbose(PRELUDE_ERROR_IDMEF_CRITERION_UNSUPPORTED_OPERATOR,
                                     "Object type '%s' does not support operator '%s'",
                                     idmef_value_type_to_string(type), idmef_criterion_operator_to_string(op));
}



int idmef_value_type_get_applicable_operators(idmef_value_type_id_t type, idmef_criterion_operator_t *result)
{
        int ret;

        ret = is_type_valid(type);
        if ( ret < 0 )
                return ret;

        *result = ops_tbl[type].operator;

        return 0;
}
