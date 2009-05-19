/*****
*
* Copyright (C) 2003-2006,2007,2008 PreludeIDS Technologies. All Rights Reserved.
* Author: Nicolas Delon <nicolas.delon@prelude-ids.com>
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

#include "config.h"

#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <assert.h>

#include "prelude-list.h"
#include "prelude-log.h"
#include "prelude-inttypes.h"
#include "prelude-string.h"
#include "prelude-error.h"
#include "common.h"

#include "idmef.h"
#include "idmef-value-type.h"

#define CHUNK_SIZE 16
#define FLOAT_TOLERANCE 0.0001


#ifndef MAX
# define MAX(x, y) ((x) > (y) ? (x) : (y))
#endif

#ifndef ABS
# define ABS(x) (((x) < 0) ? -(x) : (x))
#endif


#define idmef_value_new_decl(mtype, vname, vtype)                    \
int idmef_value_new_ ## vname (idmef_value_t **value, vtype val) {   \
        int ret;                                                     \
                                                                     \
        ret = idmef_value_create(value, IDMEF_VALUE_TYPE_ ## mtype); \
        if ( ret < 0 )                                               \
                return ret;                                          \
                                                                     \
        (*value)->type.data. vname ## _val = val;                    \
                                                                     \
        return 0;                                                    \
}


#define idmef_value_set_decl(mtype, vname, vtype)                    \
int idmef_value_set_ ## vname (idmef_value_t *value, vtype val)      \
{                                                                    \
        prelude_return_val_if_fail(value, prelude_error(PRELUDE_ERROR_ASSERTION)); \
                                                                     \
        if ( value->own_data )                                       \
                idmef_value_type_destroy(&value->type);              \
                                                                     \
        value->type.id = IDMEF_VALUE_TYPE_ ## mtype;                 \
        value->type.data. vname ## _val = val;                       \
        value->own_data = TRUE;                                      \
                                                                     \
        return 0;                                                    \
}

#define idmef_value_get_decl(mtype, vname, vtype)                \
vtype idmef_value_get_ ## vname (const idmef_value_t *val)       \
{                                                                \
        prelude_return_val_if_fail(val, (vtype) 0);              \
                                                                 \
        if ( val->type.id != IDMEF_VALUE_TYPE_ ## mtype )        \
                return (vtype) 0;                                \
                                                                 \
        return val->type.data. vname ## _val;                    \
}


#define idmef_value_decl(mtype, vname, vtype)     \
        idmef_value_new_decl(mtype, vname, vtype) \
        idmef_value_get_decl(mtype, vname, vtype) \
        idmef_value_set_decl(mtype, vname, vtype)



#define CASTCHK(ntype, fval, src, dst)                                             \
  if ( ! (src == dst && (src < 1) == (dst < 1)) )                                  \
                return prelude_error_verbose(PRELUDE_ERROR_GENERIC,                \
                        "Value '%" fval "' is incompatible with output type '%s'", \
                        src, idmef_value_type_to_string(ntype))



#define FLOATCHK(ntype, fval, src, dst)                                            \
        if ( reldif(src, dst) > FLOAT_TOLERANCE )                                  \
                return prelude_error_verbose(PRELUDE_ERROR_GENERIC,                \
                        "Value '%" fval "' is incompatible with output type '%s'", \
                        src, idmef_value_type_to_string(ntype))


#define VALUE_CAST_CHECK(v, itype_t, itype, iformat, ntype) do {      \
        itype_t src = idmef_value_get_ ##itype(value);                \
                                                                      \
        if ( ntype == IDMEF_VALUE_TYPE_INT8 ) {                       \
                CASTCHK(ntype, iformat, src, (int8_t) src);           \
                idmef_value_set_int8(value, src);                     \
                                                                      \
        } else if ( ntype == IDMEF_VALUE_TYPE_UINT8 ) {               \
                CASTCHK(ntype, iformat, src, (uint8_t) src);          \
                idmef_value_set_uint8(value, src);                    \
                                                                      \
        } else if ( ntype == IDMEF_VALUE_TYPE_INT16 ) {               \
                CASTCHK(ntype, iformat, src, (int16_t) src);          \
                idmef_value_set_int16(value, src);                    \
                                                                      \
        } else if ( ntype == IDMEF_VALUE_TYPE_UINT16 ) {              \
                CASTCHK(ntype, iformat, src, (uint16_t) src);         \
                idmef_value_set_uint16(value, src);                   \
                                                                      \
        } else if ( ntype == IDMEF_VALUE_TYPE_INT32 ) {               \
                CASTCHK(ntype, iformat, src, (int32_t) src);          \
                idmef_value_set_int32(value, src);                    \
                                                                      \
        } else if ( ntype == IDMEF_VALUE_TYPE_UINT32 ) {              \
                CASTCHK(ntype, iformat, src, (uint32_t) src);         \
                idmef_value_set_uint32(value, src);                   \
                                                                      \
        } else if ( ntype == IDMEF_VALUE_TYPE_INT64 ) {               \
                CASTCHK(ntype, iformat, src, (int64_t) src);          \
                idmef_value_set_int64(value, src);                    \
                                                                      \
        } else if ( ntype == IDMEF_VALUE_TYPE_UINT64 ) {              \
                CASTCHK(ntype, iformat, src, (uint64_t) src);         \
                idmef_value_set_uint64(value, src);                   \
                                                                      \
        } else if ( ntype == IDMEF_VALUE_TYPE_FLOAT ) {               \
                FLOATCHK(ntype, iformat, src, (float) src);           \
                idmef_value_set_float(value, src);                    \
                                                                      \
        } else if ( ntype == IDMEF_VALUE_TYPE_DOUBLE ) {              \
                FLOATCHK(ntype, iformat, src, (double) src);          \
                idmef_value_set_double(value, src);                   \
                                                                      \
        } else return prelude_error_verbose(PRELUDE_ERROR_GENERIC,    \
                                          "Unable to handle output type '%s' for integer cast", \
                                           idmef_value_type_to_string(ntype));                  \
} while(0)



typedef struct compare {
        unsigned int match;
        idmef_value_t *val2;
        idmef_criterion_operator_t operator;
} compare_t;



struct idmef_value {
        int list_elems;
        int list_max;
        int refcount;
        prelude_bool_t own_data;
        idmef_value_t **list;
        idmef_value_type_t type;
};


/*
 * Returns the relative difference of two real numbers: 0.0 if they are
 * exactly the same, otherwise, the ratio of the difference to the
 * larger of the two.
 */
static double reldif(double a, double b)
{
        double c = ABS(a);
        double d = ABS(b);

        d = MAX(c, d);

        return d == 0.0 ? 0.0 : ABS(a - b) / d;
}


static int string_isdigit(const char *s)
{
        while ( *s ) {
                if ( ! isdigit((int) *s) )
                        return -1;
                s++;
        }

        return 0;
}


static int idmef_value_create(idmef_value_t **ret, idmef_value_type_id_t type_id)
{
        *ret = calloc(1, sizeof(**ret));
        if ( ! *ret )
                return prelude_error_from_errno(errno);

        (*ret)->refcount = 1;
        (*ret)->own_data = TRUE;
        (*ret)->type.id = type_id;

        return 0;
}



idmef_value_decl(INT8, int8, int8_t)
idmef_value_decl(UINT8, uint8, uint8_t)
idmef_value_decl(INT16, int16, int16_t)
idmef_value_decl(UINT16, uint16, uint16_t)
idmef_value_decl(INT32, int32, int32_t)
idmef_value_decl(UINT32, uint32, uint32_t)
idmef_value_decl(INT64, int64, int64_t)
idmef_value_decl(UINT64, uint64, uint64_t)
idmef_value_decl(FLOAT, float, float)
idmef_value_decl(DOUBLE, double, double)
idmef_value_decl(STRING, string, prelude_string_t *)
idmef_value_decl(DATA, data, idmef_data_t *)
idmef_value_decl(TIME, time, idmef_time_t *)



int idmef_value_get_enum(const idmef_value_t *value)
{
        prelude_return_val_if_fail(value, prelude_error(PRELUDE_ERROR_ASSERTION));

        if ( value->type.id != IDMEF_VALUE_TYPE_ENUM )
                return 0;

        return value->type.data.enum_val.value;
}


int idmef_value_set_class(idmef_value_t *value, idmef_class_id_t class, void *object)
{
        prelude_return_val_if_fail(value, prelude_error(PRELUDE_ERROR_ASSERTION));
        prelude_return_val_if_fail(object, prelude_error(PRELUDE_ERROR_ASSERTION));

        if ( value->own_data )
                idmef_value_type_destroy(&value->type);

        value->own_data = TRUE;
        value->type.data.class_val.object = object;
        value->type.data.class_val.class_id = class;

        return 0;
}


int idmef_value_new_class(idmef_value_t **value, idmef_class_id_t class, void *object)
{
        int ret;

        prelude_return_val_if_fail(object, prelude_error(PRELUDE_ERROR_ASSERTION));

        ret = idmef_value_create(value, IDMEF_VALUE_TYPE_CLASS);
        if ( ret < 0 )
                return ret;

        (*value)->type.data.class_val.object = object;
        (*value)->type.data.class_val.class_id = class;

        return ret;
}


int idmef_value_set_enum_from_numeric(idmef_value_t *value, idmef_class_id_t class, int val)
{
        prelude_return_val_if_fail(value, prelude_error(PRELUDE_ERROR_ASSERTION));

        if ( value->own_data )
                idmef_value_type_destroy(&value->type);

        value->own_data = TRUE;
        value->type.id = IDMEF_VALUE_TYPE_ENUM;
        value->type.data.enum_val.value = val;
        value->type.data.enum_val.class_id = class;

        return 0;
}


int idmef_value_set_enum_from_string(idmef_value_t *value, idmef_class_id_t class, const char *buf)
{
        int ret;

        prelude_return_val_if_fail(value, prelude_error(PRELUDE_ERROR_ASSERTION));
        prelude_return_val_if_fail(buf, prelude_error(PRELUDE_ERROR_ASSERTION));

        ret = idmef_class_enum_to_numeric(class, buf);
        if ( ret < 0 )
                return ret;

        return idmef_value_set_enum_from_numeric(value, class, ret);
}


int idmef_value_set_enum(idmef_value_t *value, idmef_class_id_t class, const char *buf)
{
        int ret;

        prelude_return_val_if_fail(value, prelude_error(PRELUDE_ERROR_ASSERTION));
        prelude_return_val_if_fail(buf, prelude_error(PRELUDE_ERROR_ASSERTION));

        if ( string_isdigit(buf) == 0 )
                ret = idmef_value_set_enum_from_numeric(value, class, atoi(buf));
        else
                ret = idmef_value_set_enum_from_string(value, class, buf);

        return ret;
}


int idmef_value_new_enum_from_numeric(idmef_value_t **value, idmef_class_id_t class, int val)
{
        int ret;

        ret = idmef_value_create(value, IDMEF_VALUE_TYPE_ENUM);
        if ( ret < 0 )
                return ret;

        (*value)->type.data.enum_val.value = val;
        (*value)->type.data.enum_val.class_id = class;

        return ret;
}


int idmef_value_new_enum_from_string(idmef_value_t **value, idmef_class_id_t class, const char *buf)
{
        int ret;

        prelude_return_val_if_fail(buf, prelude_error(PRELUDE_ERROR_ASSERTION));

        ret = idmef_class_enum_to_numeric(class, buf);
        if ( ret < 0 )
                return ret;

        return idmef_value_new_enum_from_numeric(value, class, ret);
}



int idmef_value_new_enum(idmef_value_t **value, idmef_class_id_t class, const char *buf)
{
        int ret;

        prelude_return_val_if_fail(buf, prelude_error(PRELUDE_ERROR_ASSERTION));

        if ( string_isdigit(buf) == 0 )
                ret = idmef_value_new_enum_from_numeric(value, class, atoi(buf));
        else
                ret = idmef_value_new_enum_from_string(value, class, buf);

        return ret;
}


int idmef_value_new_list(idmef_value_t **value)
{
        int ret;

        ret = idmef_value_create(value, IDMEF_VALUE_TYPE_LIST);
        if ( ret < 0 )
                return ret;

        (*value)->list = malloc(CHUNK_SIZE * sizeof(idmef_value_t *));
        if ( ! (*value)->list ) {
                free(*value);
                    return prelude_error_from_errno(errno);
        }

        (*value)->list_elems = 0;
        (*value)->list_max = CHUNK_SIZE - 1;

        return 0;
}



int idmef_value_list_add(idmef_value_t *list, idmef_value_t *item)
{
        prelude_return_val_if_fail(list, prelude_error(PRELUDE_ERROR_ASSERTION));
        prelude_return_val_if_fail(item, prelude_error(PRELUDE_ERROR_ASSERTION));

        if ( list->list_elems == list->list_max ) {

                list->list = realloc(list->list, (list->list_max + 1 + CHUNK_SIZE) * sizeof(idmef_value_t *));
                if ( ! list->list )
                        return prelude_error_from_errno(errno);

                list->list_max += CHUNK_SIZE;
        }

        list->list[list->list_elems++] = item;

        return 0;
}



prelude_bool_t idmef_value_is_list(const idmef_value_t *list)
{
        prelude_return_val_if_fail(list, FALSE);
        return (list->list) ? TRUE : FALSE;
}



prelude_bool_t idmef_value_list_is_empty(const idmef_value_t *list)
{
        prelude_return_val_if_fail(list, TRUE);
        return (list->list_elems) ? FALSE : TRUE;
}




int idmef_value_new(idmef_value_t **value, idmef_value_type_id_t type, void *ptr)
{
        int ret;

        prelude_return_val_if_fail(ptr, prelude_error(PRELUDE_ERROR_ASSERTION));

        ret = idmef_value_create(value, type);
        if ( ret < 0 )
                return ret;

        (*value)->type.data.data_val = ptr;

        return 0;
}



int idmef_value_new_from_string(idmef_value_t **value, idmef_value_type_id_t type, const char *buf)
{
        int ret;

        prelude_return_val_if_fail(buf, prelude_error(PRELUDE_ERROR_ASSERTION));

        ret = idmef_value_create(value, type);
        if ( ret < 0 )
                return ret;

        ret = idmef_value_type_read(&(*value)->type, buf);
        if ( ret < 0 ) {
                free(*value);
                return ret;
        }

        return 0;
}



int idmef_value_new_from_path(idmef_value_t **value, idmef_path_t *path, const char *buf)
{
        int ret;
        idmef_class_id_t class;
        idmef_value_type_id_t value_type;

        prelude_return_val_if_fail(path, prelude_error(PRELUDE_ERROR_ASSERTION));
        prelude_return_val_if_fail(buf, prelude_error(PRELUDE_ERROR_ASSERTION));

        value_type = idmef_path_get_value_type(path, -1);
        if ( value_type < 0 )
                return value_type;

        if ( value_type != IDMEF_VALUE_TYPE_ENUM )
                ret = idmef_value_new_from_string(value, value_type, buf);
        else {
                class = idmef_path_get_class(path, -1);
                if ( class < 0 )
                        return class;

                ret = idmef_value_new_enum(value, class, buf);
        }

        return ret;
}



static int idmef_value_set_own_data(idmef_value_t *value, prelude_bool_t own_data)
{
        int cnt;

        if ( ! value->list )
                value->own_data = own_data;

        else for ( cnt = 0 ; cnt < value->list_elems; cnt++ )
                idmef_value_set_own_data(value->list[cnt], own_data);

        return 0;
}




int idmef_value_have_own_data(idmef_value_t *value)
{
        prelude_return_val_if_fail(value, prelude_error(PRELUDE_ERROR_ASSERTION));
        return idmef_value_set_own_data(value, TRUE);
}



int idmef_value_dont_have_own_data(idmef_value_t *value)
{
        prelude_return_val_if_fail(value, prelude_error(PRELUDE_ERROR_ASSERTION));
        return idmef_value_set_own_data(value, FALSE);
}



idmef_value_type_id_t idmef_value_get_type(const idmef_value_t *value)
{
        prelude_return_val_if_fail(value, prelude_error(PRELUDE_ERROR_ASSERTION));
        return value->type.id;
}



idmef_class_id_t idmef_value_get_class(const idmef_value_t *value)
{
        prelude_return_val_if_fail(value, prelude_error(PRELUDE_ERROR_ASSERTION));

        if ( value->type.id == IDMEF_VALUE_TYPE_CLASS )
                return value->type.data.class_val.class_id;

        else if ( value->type.id == IDMEF_VALUE_TYPE_ENUM )
                return value->type.data.enum_val.class_id;

        return -1;
}



void *idmef_value_get_object(const idmef_value_t *value)
{
        prelude_return_val_if_fail(value, NULL);
        return (value->type.id == IDMEF_VALUE_TYPE_CLASS) ? value->type.data.class_val.object : NULL;
}


inline static idmef_value_t *value_ro2rw(const idmef_value_t *value)
{
        union {
                idmef_value_t *rw;
                const idmef_value_t *ro;
        } val;

        val.ro = value;

        return val.rw;
}


int idmef_value_iterate(const idmef_value_t *value,
                        int (*callback)(idmef_value_t *ptr, void *extra), void *extra)
{
        int i, ret;

        prelude_return_val_if_fail(value, prelude_error(PRELUDE_ERROR_ASSERTION));
        prelude_return_val_if_fail(callback, prelude_error(PRELUDE_ERROR_ASSERTION));

        if ( ! value->list )
                return callback(value_ro2rw(value), extra);

        for ( i = 0; i < value->list_elems; i++ ) {

                ret = callback(value->list[i], extra);
                if ( ret < 0 )
                        return ret;
        }

        return 0;
}



int idmef_value_iterate_reversed(const idmef_value_t *value,
                                 int (*callback)(idmef_value_t *ptr, void *extra), void *extra)
{
        int i, ret;

        prelude_return_val_if_fail(value, prelude_error(PRELUDE_ERROR_ASSERTION));
        prelude_return_val_if_fail(callback, prelude_error(PRELUDE_ERROR_ASSERTION));

        if ( ! value->list )
                return callback(value_ro2rw(value), extra);

        for ( i = value->list_elems - 1; i >= 0; i-- ) {

                ret = callback(value->list[i], extra);
                if ( ret < 0 )
                        return ret;
        }

        return 0;
}



idmef_value_t *idmef_value_get_nth(const idmef_value_t *val, int n)
{
        prelude_return_val_if_fail(val, NULL);

        if ( ! val->list )
                return (n == 0) ? value_ro2rw(val) : NULL;

        return (n >= 0 && n < val->list_elems) ? val->list[n] : NULL;
}




int idmef_value_get_count(const idmef_value_t *val)
{
        prelude_return_val_if_fail(val, prelude_error(PRELUDE_ERROR_ASSERTION));
        return val->list ? val->list_elems : 1;
}




static int idmef_value_list_clone(const idmef_value_t *val, idmef_value_t **dst)
{
        int cnt, ret;

        ret = idmef_value_create(dst, val->type.id);
        if ( ret < 0 )
                return ret;

        (*dst)->list_elems = val->list_elems;
        (*dst)->list_max = val->list_max;
        (*dst)->list = malloc(((*dst)->list_elems + 1) * sizeof((*dst)->list));

        for ( cnt = 0; cnt < (*dst)->list_elems; cnt++ ) {

                ret = idmef_value_clone(val->list[cnt], &((*dst)->list[cnt]));
                if ( ret < 0 ) {
                        while ( --cnt >= 0 )
                                idmef_value_destroy((*dst)->list[cnt]);
                }

                free((*dst)->list);
                free(*dst);

                return -1;
        }

        return 0;
}



int idmef_value_clone(const idmef_value_t *val, idmef_value_t **dst)
{
        int ret;

        prelude_return_val_if_fail(val, prelude_error(PRELUDE_ERROR_ASSERTION));

        if ( val->list )
                return idmef_value_list_clone(val, dst);

        ret = idmef_value_create(dst, val->type.id);
        if ( ret < 0 )
                return ret;

        ret = idmef_value_type_clone(&val->type, &(*dst)->type);
        if ( ret < 0 )
                free(*dst);

        return ret;
}



idmef_value_t *idmef_value_ref(idmef_value_t *val)
{
        prelude_return_val_if_fail(val, NULL);

        val->refcount++;

        return val;
}



int idmef_value_to_string(const idmef_value_t *val, prelude_string_t *out)
{
        prelude_return_val_if_fail(val, prelude_error(PRELUDE_ERROR_ASSERTION));
        prelude_return_val_if_fail(out, prelude_error(PRELUDE_ERROR_ASSERTION));

        return idmef_value_type_write(&val->type, out);
}



int idmef_value_print(const idmef_value_t *val, prelude_io_t *fd)
{
        int ret;
        prelude_string_t *out;

        prelude_return_val_if_fail(val, prelude_error(PRELUDE_ERROR_ASSERTION));
        prelude_return_val_if_fail(fd, prelude_error(PRELUDE_ERROR_ASSERTION));

        ret = prelude_string_new(&out);
        if ( ret < 0 )
                return ret;

        ret = idmef_value_type_write(&val->type, out);
        if ( ret < 0 ) {
                prelude_string_destroy(out);
                return ret;
        }

        return prelude_io_write(fd, prelude_string_get_string(out), prelude_string_get_len(out));
}



int idmef_value_get(const idmef_value_t *val, void *res)
{
        prelude_return_val_if_fail(val, prelude_error(PRELUDE_ERROR_ASSERTION));
        prelude_return_val_if_fail(res, prelude_error(PRELUDE_ERROR_ASSERTION));

        return idmef_value_type_copy(&val->type, res);
}



static int idmef_value_match_internal(idmef_value_t *val1, void *extra)
{
        int ret;
        compare_t *compare = extra;

        if ( idmef_value_is_list(val1) )
                ret = idmef_value_iterate(val1, idmef_value_match_internal, extra);

        else if ( compare->val2 && idmef_value_is_list(compare->val2) ) {
                ret = idmef_value_match(compare->val2, val1, compare->operator);
                if ( ret < 0 )
                        return ret;

                compare->match += ret;
        }

        else {
                ret = idmef_value_type_compare(&val1->type, &compare->val2->type, compare->operator);
                if ( ret == 0 )
                        compare->match++;
        }

        return ret;
}



/**
 * idmef_value_match:
 * @val1: Pointer to a #idmef_value_t object.
 * @val2: Pointer to a #idmef_value_t object.
 * @op: operator to use for matching.
 *
 * Match @val1 and @val2 using @op.
 *
 * Returns: the number of match, 0 for none, a negative value if an error occured.
 */
int idmef_value_match(idmef_value_t *val1, idmef_value_t *val2, idmef_criterion_operator_t op)
{
        int ret;
        compare_t compare;

        prelude_return_val_if_fail(val1, prelude_error(PRELUDE_ERROR_ASSERTION));
        prelude_return_val_if_fail(val2, prelude_error(PRELUDE_ERROR_ASSERTION));

        compare.match = 0;
        compare.val2 = val2;
        compare.operator = op;

        ret = idmef_value_iterate(val1, idmef_value_match_internal, &compare);
        if ( ret < 0 )
                return ret;

        return compare.match;
}



/**
 * idmef_value_check_operator:
 * @value: Pointer to a #idmef_value_t object.
 * @op: Type of operator to check @value for.
 *
 * Check whether @op can apply to @value.
 *
 * Returns: 0 on success, a negative value if an error occured.
 */
int idmef_value_check_operator(const idmef_value_t *value, idmef_criterion_operator_t op)
{
        prelude_return_val_if_fail(value, prelude_error(PRELUDE_ERROR_ASSERTION));
        return idmef_value_type_check_operator(value->type.id, op);
}



/**
 * idmef_value_get_applicable_operators:
 * @value: Pointer to a #idmef_value_t object.
 * @result: Pointer where the result will be stored.
 *
 * Store all operator supported by @value in @result.
 *
 * Returns: 0 on success, a negative value if an error occured.
 */

int idmef_value_get_applicable_operators(const idmef_value_t *value, idmef_criterion_operator_t *result)
{
        prelude_return_val_if_fail(value, prelude_error(PRELUDE_ERROR_ASSERTION));

        return idmef_value_type_get_applicable_operators(value->type.id, result);
}



/**
 * idmef_value_destroy:
 * @val: Pointer to a #idmef_value_t object.
 *
 * Decrement refcount and destroy @value if it reach 0.
 */
void idmef_value_destroy(idmef_value_t *val)
{
        int i;

        prelude_return_if_fail(val);

        if ( --val->refcount )
                return;

        if ( val->list ) {
                for ( i = 0; i < val->list_elems; i++ )
                        idmef_value_destroy(val->list[i]);

                free(val->list);
        }

        /*
         * Actual destructor starts here
         */
        if ( val->own_data )
                idmef_value_type_destroy(&val->type);

        free(val);
}


static int cast_to_data(idmef_value_t *input)
{
        int ret;
        idmef_data_t *data;
        idmef_value_type_id_t vtype = idmef_value_get_type(input);

        if ( vtype == IDMEF_VALUE_TYPE_STRING ) {
                prelude_string_t *str = idmef_value_get_string(input);

                ret = idmef_data_new_char_string_dup_fast(&data, prelude_string_get_string(str), prelude_string_get_len(str));
                if ( ret < 0 )
                        return ret;

                return idmef_value_set_data(input, data);
        }

        else if ( vtype == IDMEF_VALUE_TYPE_INT8 ) {
                ret = idmef_data_new_uint32(&data, idmef_value_get_int8(input));
                if ( ret < 0 )
                        return ret;

                return idmef_value_set_data(input, data);
        }

        else if ( vtype == IDMEF_VALUE_TYPE_UINT8 ) {
                ret = idmef_data_new_uint32(&data, idmef_value_get_uint8(input));
                if ( ret < 0 )
                        return ret;

                return idmef_value_set_data(input, data);
        }

        else if ( vtype == IDMEF_VALUE_TYPE_INT16 ) {
                ret = idmef_data_new_uint32(&data, idmef_value_get_int16(input));
                if ( ret < 0 )
                        return ret;

                return idmef_value_set_data(input, data);
        }

        else if ( vtype == IDMEF_VALUE_TYPE_UINT16 ) {
                ret = idmef_data_new_uint32(&data, idmef_value_get_uint16(input));
                if ( ret < 0 )
                        return ret;

                return idmef_value_set_data(input, data);
        }

        else if ( vtype == IDMEF_VALUE_TYPE_INT32 ) {
                ret = idmef_data_new_uint32(&data, idmef_value_get_int32(input));
                if ( ret < 0 )
                        return ret;

                return idmef_value_set_data(input, data);
        }

        else if ( vtype == IDMEF_VALUE_TYPE_UINT32 ) {
                ret = idmef_data_new_uint32(&data, idmef_value_get_uint32(input));
                if ( ret < 0 )
                        return ret;

                return idmef_value_set_data(input, data);
        }

        else if ( vtype == IDMEF_VALUE_TYPE_INT64 ) {
                ret = idmef_data_new_uint64(&data, idmef_value_get_int64(input));
                if ( ret < 0 )
                        return ret;

                return idmef_value_set_data(input, data);
        }

        else if ( vtype == IDMEF_VALUE_TYPE_UINT64 ) {
                ret = idmef_data_new_uint64(&data, idmef_value_get_uint64(input));
                if ( ret < 0 )
                        return ret;

                return idmef_value_set_data(input, data);
        }

        else if ( vtype == IDMEF_VALUE_TYPE_FLOAT ) {
                int64_t v = idmef_value_get_float(input);

                if ( v == idmef_value_get_float(input) )
                        ret = idmef_data_new_uint64(&data, v);
                else
                        ret = idmef_data_new_float(&data, idmef_value_get_float(input));

                if ( ret < 0 )
                        return ret;

                return idmef_value_set_data(input, data);
        }

        else if ( vtype == IDMEF_VALUE_TYPE_DOUBLE ) {
                int64_t v = idmef_value_get_double(input);

                if ( v == idmef_value_get_double(input) )
                        ret = idmef_data_new_uint64(&data, v);
                else
                        ret = idmef_data_new_float(&data, idmef_value_get_double(input));

                if ( ret < 0 )
                        return ret;

                return idmef_value_set_data(input, data);
        }

        return -1;
}


static int cast_to_time(idmef_value_t *value)
{
        int ret = -1;
        idmef_time_t *time;

        if ( idmef_value_get_type(value) == IDMEF_VALUE_TYPE_STRING ) {
                ret = idmef_time_new_from_string(&time, prelude_string_get_string(idmef_value_get_string(value)));
                if ( ret < 0 )
                        return ret;

                idmef_value_set_time(value, time);
        }

        else if ( idmef_value_get_type(value) == IDMEF_VALUE_TYPE_INT32 ) {
                time_t val = idmef_value_get_int32(value);

                ret = idmef_time_new_from_time(&time, &val);
                if ( ret < 0 )
                        return ret;

                idmef_value_set_time(value, time);
        }

        else if ( idmef_value_get_type(value) == IDMEF_VALUE_TYPE_UINT32 ) {
                time_t val = idmef_value_get_uint32(value);

                ret = idmef_time_new_from_time(&time, &val);
                if ( ret < 0 )
                        return ret;

                idmef_value_set_time(value, time);
        }

        else if ( idmef_value_get_type(value) == IDMEF_VALUE_TYPE_INT64 ) {
                time_t val = idmef_value_get_int64(value);

                ret = idmef_time_new_from_time(&time, &val);
                if ( ret < 0 )
                        return ret;

                idmef_value_set_time(value, time);
        }

        else if ( idmef_value_get_type(value) == IDMEF_VALUE_TYPE_UINT64 ) {
                time_t val = idmef_value_get_uint64(value);

                ret = idmef_time_new_from_time(&time, &val);
                if ( ret < 0 )
                        return ret;

                idmef_value_set_time(value, time);
        }

        return ret;
}



static int cast_to_string(idmef_value_t *value)
{
        int ret;
        prelude_string_t *out;

        ret = prelude_string_new(&out);
        if ( ret < 0 )
                return ret;

        ret = idmef_value_type_write(&value->type, out);
        if ( ret < 0 ) {
                prelude_string_destroy(out);
                return ret;
        }

        idmef_value_set_string(value, out);

        return 0;
}



static int cast_from_string(idmef_value_t *value, idmef_value_type_id_t ntype)
{
        int ret;
        idmef_value_type_t vt;
        prelude_string_t *str = idmef_value_get_string(value);

        vt.id = ntype;

        ret = idmef_value_type_read(&vt, prelude_string_get_string(str));
        if ( ret < 0 )
                return ret;

        if ( value->own_data )
                idmef_value_type_destroy(&value->type);

        memcpy(&value->type, &vt, sizeof(value->type));

        return 0;
}



int _idmef_value_cast(idmef_value_t *value, idmef_value_type_id_t ntype, idmef_class_id_t id)
{
        idmef_value_type_id_t otype;

        prelude_return_val_if_fail(value, prelude_error(PRELUDE_ERROR_ASSERTION));

        otype = idmef_value_get_type(value);
        prelude_log_debug(3, "converting '%s' to '%s'.\n",
                          idmef_value_type_to_string(otype),
                          idmef_value_type_to_string(ntype));

        if ( ntype == IDMEF_VALUE_TYPE_DATA )
                return cast_to_data(value);

        else if ( ntype == IDMEF_VALUE_TYPE_TIME )
                return cast_to_time(value);

        else if ( ntype == IDMEF_VALUE_TYPE_STRING )
                return cast_to_string(value);

        else if ( ntype == IDMEF_VALUE_TYPE_ENUM && idmef_value_get_type(value) == IDMEF_VALUE_TYPE_STRING ) {
                prelude_string_t *str = idmef_value_get_string(value);
                return idmef_value_set_enum_from_string(value, id, prelude_string_get_string(str));
        }

        else if ( otype == IDMEF_VALUE_TYPE_INT8 )
                VALUE_CAST_CHECK(value, int8_t, int8, PRELUDE_PRId8, ntype);

        else if ( otype == IDMEF_VALUE_TYPE_UINT8 )
                VALUE_CAST_CHECK(value, uint8_t, uint8, PRELUDE_PRIu8, ntype);

        else if ( otype == IDMEF_VALUE_TYPE_INT16 )
                VALUE_CAST_CHECK(value, int16_t, int16, PRELUDE_PRId16, ntype);

        else if ( otype == IDMEF_VALUE_TYPE_UINT16 )
                VALUE_CAST_CHECK(value, uint16_t, uint16, PRELUDE_PRIu16, ntype);

        else if ( otype == IDMEF_VALUE_TYPE_INT32 )
                VALUE_CAST_CHECK(value, int32_t, int32, PRELUDE_PRId32, ntype);

        else if ( otype == IDMEF_VALUE_TYPE_UINT32 )
                VALUE_CAST_CHECK(value, uint32_t, uint32, PRELUDE_PRIu32, ntype);

        else if ( otype == IDMEF_VALUE_TYPE_INT64 )
                VALUE_CAST_CHECK(value, int64_t, int64, PRELUDE_PRId64, ntype);

        else if ( otype == IDMEF_VALUE_TYPE_UINT64 )
                VALUE_CAST_CHECK(value, uint64_t, uint64, PRELUDE_PRIu64, ntype);

        else if ( otype == IDMEF_VALUE_TYPE_FLOAT )
                VALUE_CAST_CHECK(value, float, float, "f", ntype);

        else if ( otype == IDMEF_VALUE_TYPE_DOUBLE )
                VALUE_CAST_CHECK(value, double, double, "f", ntype);

        else if ( otype == IDMEF_VALUE_TYPE_STRING )
                return cast_from_string(value, ntype);

        else return prelude_error_verbose(PRELUDE_ERROR_GENERIC,
                                          "Unable to cast input type '%s' to '%s'",
                                           idmef_value_type_to_string(value->type.id),
                                           idmef_value_type_to_string(ntype));

        return 0;
}



int _idmef_value_copy_internal(const idmef_value_t *val,
                               idmef_value_type_id_t res_type, idmef_class_id_t res_id, void *res)
{
        int ret;

        prelude_return_val_if_fail(val, prelude_error(PRELUDE_ERROR_ASSERTION));
        prelude_return_val_if_fail(res, prelude_error(PRELUDE_ERROR_ASSERTION));

        if ( res_type == val->type.id )
                ret = idmef_value_type_copy(&val->type, res);
        else {
                idmef_value_t copy;

                memcpy(&copy, val, sizeof(copy));
                idmef_value_dont_have_own_data(&copy);

                ret = _idmef_value_cast(&copy, res_type, res_id);
                if ( ret < 0 )
                        return ret;

                ret = idmef_value_type_copy(&copy.type, res);
        }

        return ret;
}
