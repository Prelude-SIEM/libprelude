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

#ifndef _LIBPRELUDE_IDMEF_DATA_H
#define _LIBPRELUDE_IDMEF_DATA_H

#include "prelude-list.h"


/*
 * Make sure that this ID does not conflict with any in idmef-tree-wrap.h
 */
#define idmef_type_data 2


typedef struct {
        
        int flags;
        size_t len;

        union {
                unsigned char *rw_data;
                const unsigned char *ro_data;
        } data;
        
        prelude_list_t list;
} idmef_data_t;



/**
 * idmef_data_new:
 *
 * Allocate a new idmef_data_t object.
 *
 * Returns: A new idmef_data_t object, or NULL if an error occured.
 */
idmef_data_t *idmef_data_new(void);



/**
 * idmef_data_new_dup:
 * @data: Pointer to the data.
 * @len: Length of the data.
 *
 * Create a new idmef_data_t object, and associate it with a copy of
 * @data. The data pointer will be freed at idmef_data_destroy() time.
 *
 * Returns: A new idmef_data_t object, or NULL if an error occured.
 */
idmef_data_t *idmef_data_new_dup(const unsigned char *data, size_t len);



/**
 * idmef_data_new_nodup:
 * @data: Pointer to the data.
 * @len: Length of the data.
 *
 * Create a new idmef_data_t object, and associate it with @data and
 * @len. The data pointer will be freed at idmef_data_destroy() time.
 *
 * Returns: A new idmef_data_t object, or NULL if an error occured.
 */
idmef_data_t *idmef_data_new_nodup(unsigned char *data, size_t len);



/**
 * idmef_data_new_ref:
 * @data: Pointer to the data.
 * @len: Length of the data.
 *
 * Create a new idmef_data_t object referencing @data. @data won't be
 * freed at idmef_data_destroy() time.
 *
 * Returns: A new idmef_data_t object, or NULL if an error occured.
 */
idmef_data_t *idmef_data_new_ref(const unsigned char *data, size_t len);



/**
 * idmef_data_destroy:
 * @data: Pointer to an #idmef_data_t object.
 *
 * Free @data. The buffer pointed to by @data will be freed if
 * the @data object is marked as _dup or _nodup.
 */
void idmef_data_destroy(idmef_data_t *data);



/**
 * idmef_data_set_nodup:
 * @data: Pointer to an #idmef_data_t object.
 * @buf: Pointer to a buffer containing data.
 * @len: Lenght of the data within the buffer.
 *
 * Associate @buf with @data. Any content already associated with
 * @data will be freed depending on how it was associated.
 *
 * The associated @buf will be freed at idmef_data_destroy() time.
 * 
 * Returns: 0 on success, -1 if an error occurred.
 */
int idmef_data_set_nodup(idmef_data_t *data, unsigned char *buf, size_t len);



/**
 * idmef_data_set_dup:
 * @data: Pointer to an #idmef_data_t object.
 * @buf: Pointer to a buffer containing data.
 * @len: Length of the data within the buffer.
 *
 * Associate a copy of @buf with @data. Any content already associated with
 * @data will be freed depending on how it was associated.
 *
 * The associated @buf will be freed at idmef_data_destroy() time.
 * 
 * Returns: 0 on success, -1 if an error occurred.
 */
int idmef_data_set_dup(idmef_data_t *data, const unsigned char *buf, size_t len);



/**
 * idmef_data_set_ref:
 * @data: Pointer to an #idmef_data_t object.
 * @buf: Pointer to a buffer containing data.
 * @len: Length of the data within the buffer.
 *
 * Associate @buf with @data. Any content already associated with
 * @data will be freed depending on how it was associated.
 *
 * The associated @buf will NOT be freed at idmef_data_destroy() time.
 * 
 * Returns: 0 on success, -1 if an error occurred.
 */
int idmef_data_set_ref(idmef_data_t *data, const unsigned char *buf, size_t len);



/**
 * idmef_data_copy_ref:
 * @dst: Destination #idmef_data_t object.
 * @src: Source #idmef_data_t object.
 *
 * Make @dst reference the same buffer as @src.
 *
 * Returns: 0 on success, -1 if an error occured.
 */
int idmef_data_copy_ref(idmef_data_t *dst, const idmef_data_t *src);



/**
 * idmef_data_copy_dup:
 * @dst: Destination #idmef_data_t object.
 * @src: Source #idmef_data_t object.
 *
 * Copy @src to @dst, including the associated buffer
 * this is an alternative to idmef_data_clone().
 *
 * Returns: 0 on success, -1 if an error occured.
 */
int idmef_data_copy_dup(idmef_data_t *dst, const idmef_data_t *src);




/**
 * idmef_data_clone:
 * @data: Pointer to an #idmef_data_t object.
 *
 * Make up an exact copy of the @data object, and it's content.
 *
 * Returns: A new #idmef_data_t object, or NULL if an error occured.
 */
idmef_data_t *idmef_data_clone(const idmef_data_t *data);



/**
 * idmef_data_get_len:
 * @data: Pointer to an #idmef_data_t object.
 *
 * Returns: the len of data contained within @data object.
 */
size_t idmef_data_get_len(const idmef_data_t *data);



/**
 * idmef_data_get_data:
 * @data: Pointer to an #idmef_data_t object.
 *
 * Returns: the data contained within @data object.
 */
unsigned char *idmef_data_get_data(const idmef_data_t *data);



/**
 * idmef_data_is_empty:
 * @data: Pointer to an #idmef_data_t object.
 *
 * Returns: 1 if the data is empty, 0 otherwise.
 */
int idmef_data_is_empty(const idmef_data_t *data);



/**
 * idmef_data_to_string:
 * @data: Pointer to an #idmef_data_t object.
 * @buf: Buffer to store the formated data into.
 * @size: Size of @buf.
 *
 * Format the data contained within @data to be printable,
 * and store the result in the provided @buf of @size length.
 *
 * Returns: 0 on success, -1 if an error occured.
 */
int idmef_data_to_string(idmef_data_t *data, char *buf, size_t size);



/*
 * Free data contents, don't free the data itself.
 * This function is not meant to be used publicly.
 */
void idmef_data_destroy_internal(idmef_data_t *data);



/**
 * idmef_data_set_constant_ref:
 * @data: Pointer to an #idmef_data_t object.
 * @buf: Pointer to a string buffer.
 *
 * Same as idmef_data_set_ref(), but automatically provide
 * sizeof(buf) as the length argument.
 */
#define idmef_data_set_constant_ref(data, buf)      \
        idmef_data_set_ref(data, buf, sizeof(buf))



/*
 * FIXME: backward compatibility
 */
#define idmef_data idmef_data_get_data
#define idmef_data_len idmef_data_get_len

#endif /* _LIBPRELUDE_IDMEF_DATA_H */
