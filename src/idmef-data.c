/*****
*
* Copyright (C) 2003-2005,2006,2007 PreludeIDS Technologies. All Rights Reserved.
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

#include "libmissing.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include "prelude-error.h"
#include "prelude-inttypes.h"
#include "prelude-string.h"
#include "common.h"
#include "idmef-data.h"


/*
 * Data structure may be free'd
 */
#define IDMEF_DATA_OWN_STRUCTURE  0x1

/*
 * Data content may be free'd
 */
#define IDMEF_DATA_OWN_DATA       0x2


#define IDMEF_DATA_DECL(idmef_data_type, c_type, name)                \
int idmef_data_new_ ## name(idmef_data_t **nd, c_type val)            \
{                                                                     \
        int ret;                                                      \
                                                                      \
        ret = idmef_data_new(nd);                                     \
        if ( ret < 0 )                                                \
                return ret;                                           \
                                                                      \
        idmef_data_set_ ## name(*nd, val);                            \
                                                                      \
        return ret;                                                   \
}                                                                     \
                                                                      \
void idmef_data_set_ ## name(idmef_data_t *ptr, c_type val)           \
{                                                                     \
        prelude_return_if_fail(ptr);                                  \
        idmef_data_destroy_internal(ptr);                             \
        ptr->type = idmef_data_type;                                  \
        ptr->len = sizeof(val);                                       \
        ptr->data.name ## _data = val;                                \
}                                                                     \
                                                                      \
c_type idmef_data_get_ ## name(const idmef_data_t *ptr)               \
{                                                                     \
        return ptr->data.name ## _data;                               \
}


IDMEF_DATA_DECL(IDMEF_DATA_TYPE_CHAR, char, char)
IDMEF_DATA_DECL(IDMEF_DATA_TYPE_BYTE, uint8_t, byte)
IDMEF_DATA_DECL(IDMEF_DATA_TYPE_UINT32, uint32_t, uint32)
IDMEF_DATA_DECL(IDMEF_DATA_TYPE_UINT64, uint64_t, uint64)
IDMEF_DATA_DECL(IDMEF_DATA_TYPE_FLOAT, float, float)


int idmef_data_new(idmef_data_t **data)
{
        *data = calloc(1, sizeof(**data));
        if ( ! *data )
                return prelude_error_from_errno(errno);

        (*data)->refcount = 1;
        (*data)->flags |= IDMEF_DATA_OWN_STRUCTURE;

        return 0;
}



idmef_data_t *idmef_data_ref(idmef_data_t *data)
{
        prelude_return_val_if_fail(data, NULL);

        data->refcount++;
        return data;
}



int idmef_data_set_ptr_ref_fast(idmef_data_t *data, idmef_data_type_t type, const void *ptr, size_t len)
{
        prelude_return_val_if_fail(data, prelude_error(PRELUDE_ERROR_ASSERTION));
        prelude_return_val_if_fail(ptr, prelude_error(PRELUDE_ERROR_ASSERTION));

        idmef_data_destroy_internal(data);

        data->type = type;
        data->data.ro_data = ptr;
        data->len = len;

        return 0;
}



int idmef_data_set_ptr_dup_fast(idmef_data_t *data, idmef_data_type_t type, const void *ptr, size_t len)
{
        void *new;

        prelude_return_val_if_fail(data, prelude_error(PRELUDE_ERROR_ASSERTION));
        prelude_return_val_if_fail(ptr, prelude_error(PRELUDE_ERROR_ASSERTION));

        idmef_data_destroy_internal(data);

        new = malloc(len);
        if ( ! new )
                return -1;

        memcpy(new, ptr, len);

        data->type = type;
        data->data.rw_data = new;
        data->len = len;
        data->flags |= IDMEF_DATA_OWN_DATA;

        return 0;
}



int idmef_data_set_ptr_nodup_fast(idmef_data_t *data, idmef_data_type_t type, void *ptr, size_t len)
{
        prelude_return_val_if_fail(data, prelude_error(PRELUDE_ERROR_ASSERTION));
        prelude_return_val_if_fail(ptr, prelude_error(PRELUDE_ERROR_ASSERTION));

        idmef_data_destroy_internal(data);

        data->type = type;
        data->data.rw_data= ptr;
        data->len = len;
        data->flags |= IDMEF_DATA_OWN_DATA;

        return 0;
}



int idmef_data_new_ptr_ref_fast(idmef_data_t **data, idmef_data_type_t type, const void *ptr, size_t len)
{
        int ret;

        ret = idmef_data_new(data);
        if ( ret < 0 )
                return ret;

        ret = idmef_data_set_ptr_ref_fast(*data, type, ptr, len);
        if ( ret < 0 )
                idmef_data_destroy(*data);

        return ret;
}



int idmef_data_new_ptr_dup_fast(idmef_data_t **data, idmef_data_type_t type, const void *ptr, size_t len)
{
        int ret;

        ret = idmef_data_new(data);
        if ( ret < 0 )
                return ret;

        ret = idmef_data_set_ptr_dup_fast(*data, type, ptr, len);
        if ( ret < 0 )
                idmef_data_destroy(*data);

        return ret;
}



int idmef_data_new_ptr_nodup_fast(idmef_data_t **data, idmef_data_type_t type, void *ptr, size_t len)
{
        int ret;

        ret = idmef_data_new(data);
        if ( ret < 0 )
                return ret;

        ret = idmef_data_set_ptr_nodup_fast(*data, type, ptr, len);
        if ( ret < 0 )
                idmef_data_destroy(*data);

        return ret;
}




/**
 * idmef_data_copy_ref:
 * @src: Source #idmef_data_t object.
 * @dst: Destination #idmef_data_t object.
 *
 * Makes @dst reference the same buffer as @src.
 *
 * Returns: 0 on success, a negative value if an error occured.
 */
int idmef_data_copy_ref(const idmef_data_t *src, idmef_data_t *dst)
{
        prelude_return_val_if_fail(src, prelude_error(PRELUDE_ERROR_ASSERTION));
        prelude_return_val_if_fail(dst, prelude_error(PRELUDE_ERROR_ASSERTION));

        idmef_data_destroy_internal(dst);

        dst->type = src->type;
        dst->len = src->len;
        dst->data = src->data;
        dst->flags &= ~IDMEF_DATA_OWN_DATA;

        return 0;
}




/**
 * idmef_data_copy_dup:
 * @src: Source #idmef_data_t object.
 * @dst: Destination #idmef_data_t object.
 *
 * Copies @src to @dst, including the associated buffer.
 * This is an alternative to idmef_data_clone().
 *
 * Returns: 0 on success, a negative value if an error occured.
 */
int idmef_data_copy_dup(const idmef_data_t *src, idmef_data_t *dst)
{
        prelude_return_val_if_fail(src, prelude_error(PRELUDE_ERROR_ASSERTION));
        prelude_return_val_if_fail(dst, prelude_error(PRELUDE_ERROR_ASSERTION));

        idmef_data_destroy_internal(dst);

        dst->type = src->type;
        dst->flags |= IDMEF_DATA_OWN_DATA;
        dst->len = src->len;

        if ( src->type == IDMEF_DATA_TYPE_CHAR_STRING || src->type == IDMEF_DATA_TYPE_BYTE_STRING ) {
                dst->data.rw_data = malloc(src->len);
                if ( ! dst->data.rw_data )
                        return -1;

                memcpy(dst->data.rw_data, src->data.ro_data, src->len);
        } else {
                dst->data = src->data;
        }

        return 0;
}



int idmef_data_clone(const idmef_data_t *src, idmef_data_t **dst)
{
        int ret;

        prelude_return_val_if_fail(src, prelude_error(PRELUDE_ERROR_ASSERTION));

        ret = idmef_data_new(dst);
        if ( ret < 0 )
                return ret;

        ret = idmef_data_copy_dup(src, *dst);
        if ( ret < 0 )
                idmef_data_destroy(*dst);

        return ret;
}



const char *idmef_data_get_char_string(const idmef_data_t *data)
{
        prelude_return_val_if_fail(data, NULL);
        return data->data.ro_data;
}



const unsigned char *idmef_data_get_byte_string(const idmef_data_t *data)
{
        prelude_return_val_if_fail(data, NULL);
        return data->data.ro_data;
}



/**
 * idmef_data_get_type
 * @data: Pointer to an #idmef_data_t object.
 *
 * Returns: the type of the embedded data.
 */
idmef_data_type_t idmef_data_get_type(const idmef_data_t *data)
{
        prelude_return_val_if_fail(data, prelude_error(PRELUDE_ERROR_ASSERTION));
        return data->type;
}




/**
 * idmef_data_get_len:
 * @data: Pointer to an #idmef_data_t object.
 *
 * Returns: the length of data contained within @data object.
 */
size_t idmef_data_get_len(const idmef_data_t *data)
{
        prelude_return_val_if_fail(data, 0);
        return data->len;
}




/**
 * idmef_data_get_data:
 * @data: Pointer to an #idmef_data_t object.
 *
 * Returns: the data contained within @data object.
 */
const void *idmef_data_get_data(const idmef_data_t *data)
{
        prelude_return_val_if_fail(data, NULL);

        switch ( data->type ) {
        case IDMEF_DATA_TYPE_UNKNOWN:
                return NULL;

        case IDMEF_DATA_TYPE_CHAR_STRING: case IDMEF_DATA_TYPE_BYTE_STRING:
                return data->data.ro_data;

        default:
                return &data->data;
        }

        return NULL;
}



/**
 * idmef_data_is_empty:
 * @data: Pointer to an #idmef_data_t object.
 *
 * Returns: TRUE if empty, FALSE otherwise.
 */
prelude_bool_t idmef_data_is_empty(const idmef_data_t *data)
{
        prelude_return_val_if_fail(data, TRUE);
        return (data->len == 0) ? TRUE : FALSE;
}



static int bytes_to_string(prelude_string_t *out, const unsigned char *src, size_t size)
{
        char c;
        int ret;
        static const char b64tbl[64] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

        while ( size ) {
                ret = prelude_string_ncat(out, &b64tbl[(src[0] >> 2) & 0x3f], 1);
                if ( ret < 0 )
                        return ret;

                c = b64tbl[((src[0] << 4) + ((--size) ? src[1] >> 4 : 0)) & 0x3f];

                ret = prelude_string_ncat(out, &c, 1);
                if ( ret < 0 )
                        return ret;

                c = (size) ? b64tbl[((src[1] << 2) + ((--size) ? src[2] >> 6 : 0)) & 0x3f] : '=';

                ret = prelude_string_ncat(out, &c, 1);
                if ( ret < 0 )
                        return ret;

                c = (size && size--) ? b64tbl[src[2] & 0x3f] : '=';

                ret = prelude_string_ncat(out, &c, 1);
                if ( ret < 0 )
                        return ret;

                src += 3;
        }

        return 0;
}




/**
 * idmef_data_to_string:
 * @data: Pointer to an #idmef_data_t object.
 * @out: Pointer to a #prelude_string_t to store the formated data into.
 *
 * Formats data contained within @data to be printable,
 * and stores the result in the provided @out buffer.
 *
 * Returns: 0 on success, a negative value if an error occured.
 */
int idmef_data_to_string(const idmef_data_t *data, prelude_string_t *out)
{
        int ret = 0;

        prelude_return_val_if_fail(data, prelude_error(PRELUDE_ERROR_ASSERTION));
        prelude_return_val_if_fail(out, prelude_error(PRELUDE_ERROR_ASSERTION));

        switch ( data->type ) {
        case IDMEF_DATA_TYPE_UNKNOWN:
                return 0;

        case IDMEF_DATA_TYPE_CHAR:
                ret = prelude_string_sprintf(out, "%c", data->data.char_data);
                break;

        case IDMEF_DATA_TYPE_BYTE:
                /*
                 * %hh convertion specifier is not portable.
                 */
                ret = prelude_string_sprintf(out, "%u", (unsigned int) data->data.byte_data);
                break;

        case IDMEF_DATA_TYPE_UINT32:
                ret = prelude_string_sprintf(out, "%u", data->data.uint32_data);
                break;

        case IDMEF_DATA_TYPE_UINT64:
                ret = prelude_string_sprintf(out, "%" PRELUDE_PRIu64, data->data.uint64_data);
                break;

        case IDMEF_DATA_TYPE_FLOAT:
                ret = prelude_string_sprintf(out, "%f", data->data.float_data);
                break;

        case IDMEF_DATA_TYPE_CHAR_STRING:
                ret = prelude_string_sprintf(out, "%s", (const char *) data->data.ro_data);
                break;

        case IDMEF_DATA_TYPE_BYTE_STRING:
                ret = bytes_to_string(out, data->data.ro_data, data->len);
                break;
        }

        return ret;
}



/*
 *  This function cannot be declared static because it is invoked
 *  from idmef-tree-wrap.c
 */
void idmef_data_destroy_internal(idmef_data_t *ptr)
{
        prelude_return_if_fail(ptr);

        if ( (ptr->type == IDMEF_DATA_TYPE_CHAR_STRING || ptr->type == IDMEF_DATA_TYPE_BYTE_STRING) &&
             ptr->flags & IDMEF_DATA_OWN_DATA ) {
                free(ptr->data.rw_data);
                ptr->data.rw_data = NULL;
        }

        /*
         * free() should be done by the caller
         */
}




/**
 * idmef_data_destroy:
 * @data: Pointer to an #idmef_data_t object.
 *
 * Frees @data. The buffer pointed by @data will be freed if
 * the @data object is marked as _dup or _nodup.
 */
void idmef_data_destroy(idmef_data_t *data)
{
        prelude_return_if_fail(data);

        if ( --data->refcount )
                return;

        idmef_data_destroy_internal(data);

        if ( data->flags & IDMEF_DATA_OWN_STRUCTURE )
                free(data);
}


int idmef_data_new_char_string_ref_fast(idmef_data_t **data, const char *ptr, size_t len)
{
        return idmef_data_new_ptr_ref_fast(data, IDMEF_DATA_TYPE_CHAR_STRING, ptr, len + 1);
}

int idmef_data_new_char_string_dup_fast(idmef_data_t **data, const char *ptr, size_t len)
{
        return idmef_data_new_ptr_dup_fast(data, IDMEF_DATA_TYPE_CHAR_STRING, ptr, len + 1);
}

int idmef_data_new_char_string_nodup_fast(idmef_data_t **data, char *ptr, size_t len)
{
        return idmef_data_new_ptr_nodup_fast(data, IDMEF_DATA_TYPE_CHAR_STRING, ptr, len + 1);
}

int idmef_data_set_char_string_ref_fast(idmef_data_t *data, const char *ptr, size_t len)
{
        return idmef_data_set_ptr_ref_fast(data, IDMEF_DATA_TYPE_CHAR_STRING, ptr, len + 1);
}

int idmef_data_set_char_string_dup_fast(idmef_data_t *data, const char *ptr, size_t len)
{
        return idmef_data_set_ptr_dup_fast(data, IDMEF_DATA_TYPE_CHAR_STRING, ptr, len + 1);
}

int idmef_data_set_char_string_nodup_fast(idmef_data_t *data, char *ptr, size_t len)
{
        return idmef_data_set_ptr_nodup_fast(data, IDMEF_DATA_TYPE_CHAR_STRING, ptr, len + 1);
}


int idmef_data_new_char_string_ref(idmef_data_t **data, const char *ptr)
{
        prelude_return_val_if_fail(ptr, prelude_error(PRELUDE_ERROR_ASSERTION));
        return idmef_data_new_char_string_ref_fast(data, ptr, strlen(ptr));
}

int idmef_data_new_char_string_dup(idmef_data_t **data, const char *ptr)
{
        prelude_return_val_if_fail(ptr, prelude_error(PRELUDE_ERROR_ASSERTION));
        return idmef_data_new_char_string_dup_fast(data, ptr, strlen(ptr));
}

int idmef_data_new_char_string_nodup(idmef_data_t **data, char *ptr)
{
        prelude_return_val_if_fail(ptr, prelude_error(PRELUDE_ERROR_ASSERTION));
        return idmef_data_new_char_string_nodup_fast(data, ptr, strlen(ptr));
}

int idmef_data_set_char_string_ref(idmef_data_t *data, const char *ptr)
{
        prelude_return_val_if_fail(data, prelude_error(PRELUDE_ERROR_ASSERTION));
        prelude_return_val_if_fail(ptr, prelude_error(PRELUDE_ERROR_ASSERTION));

        return idmef_data_set_char_string_ref_fast(data, ptr, strlen(ptr));
}

int idmef_data_set_char_string_dup(idmef_data_t *data, const char *ptr)
{
        prelude_return_val_if_fail(data, prelude_error(PRELUDE_ERROR_ASSERTION));
        prelude_return_val_if_fail(ptr, prelude_error(PRELUDE_ERROR_ASSERTION));

        return idmef_data_set_char_string_dup_fast(data, ptr, strlen(ptr));
}

int idmef_data_set_char_string_nodup(idmef_data_t *data, char *ptr)
{
        prelude_return_val_if_fail(data, prelude_error(PRELUDE_ERROR_ASSERTION));
        prelude_return_val_if_fail(ptr, prelude_error(PRELUDE_ERROR_ASSERTION));

        return idmef_data_set_char_string_nodup_fast(data, ptr, strlen(ptr));
}


/*
 *
 */
int idmef_data_new_byte_string_ref(idmef_data_t **data, const unsigned char *ptr, size_t len)
{
        return idmef_data_new_ptr_ref_fast(data, IDMEF_DATA_TYPE_BYTE_STRING, ptr, len);
}


int idmef_data_new_byte_string_dup(idmef_data_t **data, const unsigned char *ptr, size_t len)
{
        return idmef_data_new_ptr_dup_fast(data, IDMEF_DATA_TYPE_BYTE_STRING, ptr, len);
}


int idmef_data_new_byte_string_nodup(idmef_data_t **data, unsigned char *ptr, size_t len)
{
        return idmef_data_new_ptr_nodup_fast(data, IDMEF_DATA_TYPE_BYTE_STRING, ptr, len);
}


int idmef_data_set_byte_string_ref(idmef_data_t *data, const unsigned char *ptr, size_t len)
{
        return idmef_data_set_ptr_ref_fast(data, IDMEF_DATA_TYPE_BYTE_STRING, ptr, len);
}


int idmef_data_set_byte_string_dup(idmef_data_t *data, const unsigned char *ptr, size_t len)
{
        return idmef_data_set_ptr_dup_fast(data, IDMEF_DATA_TYPE_BYTE_STRING, ptr, len);
}


int idmef_data_set_byte_string_nodup(idmef_data_t *data, unsigned char *ptr, size_t len)
{
        return idmef_data_set_ptr_nodup_fast(data, IDMEF_DATA_TYPE_BYTE_STRING, ptr, len);
}



int idmef_data_compare(const idmef_data_t *data1, const idmef_data_t *data2)
{
        if ( ! data1 && ! data2 )
                return 0;

        else if ( ! data1 || ! data2 )
                return (data1) ? 1 : -1;

        else if ( data1->len != data2->len )
                return (data1->len > data2->len) ? 1 : -1;

        else if ( data1->type != data2->type )
                return -1;

        if ( data1->type == IDMEF_DATA_TYPE_CHAR_STRING || data1->type == IDMEF_DATA_TYPE_BYTE_STRING )
                return memcmp(data1->data.ro_data, data2->data.ro_data, data1->len);
        else
                return memcmp(&data1->data.char_data, &data2->data.char_data, data1->len);
}
