/*****
*
* Copyright (C) 2004, 2005 PreludeIDS Technologies. All Rights Reserved.
* Author: Yoann Vandoorselaere <yoann.v@prelude-ids.com>
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

#ifndef _LIBPRELUDE_IDMEF_ADDITIONAL_DATA_H
#define _LIBPRELUDE_IDMEF_ADDITIONAL_DATA_H

#ifdef __cplusplus
 extern "C" {
#endif
         
/*
 * basic type
 */
int idmef_additional_data_new_real(idmef_additional_data_t **ret, float data);
int idmef_additional_data_new_byte(idmef_additional_data_t **ret, uint8_t byte);
int idmef_additional_data_new_integer(idmef_additional_data_t **ret, uint32_t data);
int idmef_additional_data_new_boolean(idmef_additional_data_t **ret, prelude_bool_t data);
int idmef_additional_data_new_character(idmef_additional_data_t **ret, char data);

void idmef_additional_data_set_real(idmef_additional_data_t *ptr, float data);
void idmef_additional_data_set_byte(idmef_additional_data_t *ptr, uint8_t byte);
void idmef_additional_data_set_integer(idmef_additional_data_t *ptr, uint32_t data);
void idmef_additional_data_set_boolean(idmef_additional_data_t *ptr, prelude_bool_t data);
void idmef_additional_data_set_character(idmef_additional_data_t *ptr, char data);


int idmef_additional_data_new_ptr_ref_fast(idmef_additional_data_t **nd,
                                           idmef_additional_data_type_t type,
                                           const unsigned char *data, size_t len);

int idmef_additional_data_new_ptr_dup_fast(idmef_additional_data_t **nd,
                                           idmef_additional_data_type_t type,
                                           const unsigned char *data, size_t len);

int idmef_additional_data_new_ptr_nodup_fast(idmef_additional_data_t **nd,
                                             idmef_additional_data_type_t type,
                                             unsigned char *data, size_t len);

int idmef_additional_data_set_ptr_ref_fast(idmef_additional_data_t *ad,
                                            idmef_additional_data_type_t type, const unsigned char *data, size_t len);

int idmef_additional_data_set_ptr_dup_fast(idmef_additional_data_t *ad,
                                            idmef_additional_data_type_t type, const unsigned char *data, size_t len);

int idmef_additional_data_set_ptr_nodup_fast(idmef_additional_data_t *ad,
                                            idmef_additional_data_type_t type, unsigned char *data, size_t len);


/*
 * string.
 */

int idmef_additional_data_set_str_dup_fast(idmef_additional_data_t *data, idmef_additional_data_type_t type,
                                            const char *str, size_t len);

int idmef_additional_data_new_str_dup_fast(idmef_additional_data_t **nd,
                                           idmef_additional_data_type_t type, const char *str, size_t len);

#define idmef_additional_data_new_string_ref_fast(data, len) \
        idmef_additional_data_new_ptr_ref_fast(IDMEF_ADDITIONAL_DATA_TYPE_STRING, data, len + 1)

#define idmef_additional_data_set_string_ref_fast(ad, data, len) \
        idmef_additional_data_set_ptr_ref_fast(ad, IDMEF_ADDITIONAL_DATA_TYPE_STRING, data, len + 1)

#define idmef_additional_data_new_string_dup_fast(data, len) \
        idmef_additional_data_new_str_dup_fast(IDMEF_ADDITIONAL_DATA_TYPE_STRING, data, len)

#define idmef_additional_data_set_string_dup_fast(ad, data, len) \
        idmef_additional_data_set_str_dup_fast(ad, IDMEF_ADDITIONAL_DATA_TYPE_STRING, data, len)

#define idmef_additional_data_new_string_nodup_fast(data, len) \
        idmef_additional_data_new_ptr_nodup_fast(IDMEF_ADDITIONAL_DATA_TYPE_STRING, data, len + 1)

#define idmef_additional_data_set_string_nodup_fast(ad, data, len) \
        idmef_additional_data_set_ptr_nodup_fast(ad, IDMEF_ADDITIONAL_DATA_TYPE_STRING, data, len + 1)

#define idmef_additional_data_new_string_ref(data) \
        idmef_additional_data_new_string_ref_fast(data, strlen(data))

#define idmef_additional_data_set_string_ref(ad, data) \
        idmef_additional_data_set_string_ref_fast(ad, data, strlen(data))

#define idmef_additional_data_new_string_dup(data) \
        idmef_additional_data_new_string_dup_fast(data, strlen(data))

#define idmef_additional_data_set_string_dup(data) \
        idmef_additional_data_set_string_dup_fast(data, strlen(data))

#define idmef_additional_data_new_string_nodup(data) \
        idmef_additional_data_new_string_nodup_fast(data, strlen(data))

#define idmef_additional_data_set_string_nodup(data) \
        idmef_additional_data_set_string_nodup_fast(data, strlen(data))

/*
 * ntpstamp
 */
#define idmef_additional_data_new_ntpstamp_ref_fast(data, len) \
        idmef_additional_data_new_ptr_ref_fast(IDMEF_ADDITIONAL_DATA_TYPE_NTPSTAMP, data, len + 1)

#define idmef_additional_data_set_ntpstamp_ref_fast(ad, data, len) \
        idmef_additional_data_set_ptr_ref_fast(ad, IDMEF_ADDITIONAL_DATA_TYPE_NTPSTAMP, data, len + 1)

#define idmef_additional_data_new_ntpstamp_dup_fast(data, len) \
        idmef_additional_data_new_str_dup_fast(IDMEF_ADDITIONAL_DATA_TYPE_NTPSTAMP, data, len)

#define idmef_additional_data_set_ntpstamp_dup_fast(ad, data, len) \
        idmef_additional_data_set_str_dup_fast(ad, IDMEF_ADDITIONAL_DATA_TYPE_NTPSTAMP, data, len)

#define idmef_additional_data_new_ntpstamp_nodup_fast(data, len) \
        idmef_additional_data_new_ptr_nodup_fast(IDMEF_ADDITIONAL_DATA_TYPE_NTPSTAMP, data, len + 1)

#define idmef_additional_data_set_ntpstamp_nodup_fast(ad, data, len) \
        idmef_additional_data_set_ptr_nodup_fast(ad, IDMEF_ADDITIONAL_DATA_TYPE_NTPSTAMP, data, len + 1)

#define idmef_additional_data_new_ntpstamp_ref(data) \
        idmef_additional_data_new_ntpstamp_ref_fast(data, strlen(data))

#define idmef_additional_data_set_ntpstamp_ref(ad, data) \
        idmef_additional_data_set_ntpstamp_ref_fast(ad, data, strlen(data))

#define idmef_additional_data_new_ntpstamp_dup(data) \
        idmef_additional_data_new_ntpstamp_dup_fast(data, strlen(data))

#define idmef_additional_data_set_ntpstamp_dup(data) \
        idmef_additional_data_set_ntpstamp_dup_fast(data, strlen(data))

#define idmef_additional_data_new_ntpstamp_nodup(data) \
        idmef_additional_data_new_ntpstamp_nodup_fast(data, strlen(data))

#define idmef_additional_data_set_ntpstamp_nodup(data) \
        idmef_additional_data_set_ntpstamp_nodup_fast(data, strlen(data))


/*
 * byte string
 */
#define idmef_additional_data_new_byte_string_ref(data, len) \
        idmef_additional_data_new_ptr_ref_fast(IDMEF_ADDITIONAL_DATA_TYPE_BYTE_STRING, data, len)

#define idmef_additional_data_set_byte_string_ref(ad, data, len) \
        idmef_additional_data_set_ptr_ref_fast(ad, IDMEF_ADDITIONAL_DATA_TYPE_BYTE_STRING, data, len)

#define idmef_additional_data_new_byte_string_dup(data, len) \
        idmef_additional_data_new_ptr_dup_fast(IDMEF_ADDITIONAL_DATA_TYPE_BYTE_STRING, data, len)

#define idmef_additional_data_set_byte_string_dup(ad, data, len) \
        idmef_additional_data_set_ptr_dup_fast(ad, IDMEF_ADDITIONAL_DATA_TYPE_BYTE_STRING, data, len)

#define idmef_additional_data_new_byte_string_nodup(data, len) \
        idmef_additional_data_new_ptr_nodup_fast(IDMEF_ADDITIONAL_DATA_TYPE_BYTE_STRING, data, len)

#define idmef_additional_data_set_byte_string_nodup(ad, data, len) \
        idmef_additional_data_set_ptr_nodup_fast(ad, IDMEF_ADDITIONAL_DATA_TYPE_BYTE_STRING, data, len)


/*
 * portlist
 */
#define idmef_additional_data_new_portlist_ref_fast(data, len) \
        idmef_additional_data_new_ptr_ref_fast(IDMEF_ADDITIONAL_DATA_TYPE_PORTLIST, data, len + 1)

#define idmef_additional_data_set_portlist_ref_fast(ad, data, len) \
        idmef_additional_data_set_ptr_ref_fast(ad, IDMEF_ADDITIONAL_DATA_TYPE_PORTLIST, data, len + 1)

#define idmef_additional_data_new_portlist_dup_fast(data, len) \
        idmef_additional_data_new_str_dup_fast(IDMEF_ADDITIONAL_DATA_TYPE_PORTLIST, data, len)

#define idmef_additional_data_set_portlist_dup_fast(ad, data, len) \
        idmef_additional_data_set_str_dup_fast(ad, IDMEF_ADDITIONAL_DATA_TYPE_PORTLIST, data, len)

#define idmef_additional_data_new_portlist_nodup_fast(data, len) \
        idmef_additional_data_new_ptr_nodup_fast(IDMEF_ADDITIONAL_DATA_TYPE_PORTLIST, data, len + 1)

#define idmef_additional_data_set_portlist_nodup_fast(ad, data, len) \
        idmef_additional_data_set_ptr_nodup_fast(ad, IDMEF_ADDITIONAL_DATA_TYPE_PORTLIST, data, len + 1)

#define idmef_additional_data_new_portlist_ref(data) \
        idmef_additional_data_new_portlist_ref_fast(data, strlen(data))

#define idmef_additional_data_set_portlist_ref(ad, data) \
        idmef_additional_data_set_portlist_ref_fast(ad, data, strlen(data))

#define idmef_additional_data_new_portlist_dup(data) \
        idmef_additional_data_new_portlist_dup_fast(data, strlen(data))

#define idmef_additional_data_set_portlist_dup(data) \
        idmef_additional_data_set_portlist_dup_fast(data, strlen(data))

#define idmef_additional_data_new_portlist_nodup(data) \
        idmef_additional_data_new_portlist_nodup_fast(data, strlen(data))

#define idmef_additional_data_set_portlist_nodup(data) \
        idmef_additional_data_set_portlist_nodup_fast(data, strlen(data))

/*
 * datetime
 */
#define idmef_additional_data_new_datetime_ref_fast(data, len) \
        idmef_additional_data_new_ptr_ref_fast(IDMEF_ADDITIONAL_DATA_TYPE_DATE_TIME, data, len + 1)

#define idmef_additional_data_set_datetime_ref_fast(ad, data, len) \
        idmef_additional_data_set_ptr_ref_fast(ad, IDMEF_ADDITIONAL_DATA_TYPE_DATE_TIME, data, len + 1)

#define idmef_additional_data_new_datetime_dup_fast(data, len) \
        idmef_additional_data_new_str_dup_fast(IDMEF_ADDITIONAL_DATA_TYPE_DATE_TIME, data, len)

#define idmef_additional_data_set_datetime_dup_fast(ad, data, len) \
        idmef_additional_data_set_str_dup_fast(ad, IDMEF_ADDITIONAL_DATA_TYPE_DATE_TIME, data, len)

#define idmef_additional_data_new_datetime_nodup_fast(data, len) \
        idmef_additional_data_new_ptr_nodup_fast(IDMEF_ADDITIONAL_DATA_TYPE_DATE_TIME, data, len + 1)

#define idmef_additional_data_set_datetime_nodup_fast(ad, data, len) \
        idmef_additional_data_set_ptr_nodup_fast(ad, IDMEF_ADDITIONAL_DATA_TYPE_DATE_TIME, data, len + 1)

#define idmef_additional_data_new_datetime_ref(data) \
        idmef_additional_data_new_datetime_ref_fast(data, strlen(data))

#define idmef_additional_data_set_datetime_ref(ad, data) \
        idmef_additional_data_set_datetime_ref_fast(ad, data, strlen(data))

#define idmef_additional_data_new_datetime_dup(data) \
        idmef_additional_data_new_datetime_dup_fast(data, strlen(data))

#define idmef_additional_data_set_datetime_dup(data) \
        idmef_additional_data_set_datetime_dup_fast(data, strlen(data))

#define idmef_additional_data_new_datetime_nodup(data) \
        idmef_additional_data_new_datetime_nodup_fast(data, strlen(data))

#define idmef_additional_data_set_datetime_nodup(data) \
        idmef_additional_data_set_datetime_nodup_fast(data, strlen(data))


/*
 * xml
 */
#define idmef_additional_data_new_xml_ref_fast(data, len) \
        idmef_additional_data_new_ptr_ref_fast(IDMEF_ADDITIONAL_DATA_TYPE_XML, data, len + 1)

#define idmef_additional_data_set_xml_ref_fast(ad, data, len) \
        idmef_additional_data_set_ptr_ref_fast(ad, IDMEF_ADDITIONAL_DATA_TYPE_XML, data, len + 1)

#define idmef_additional_data_new_xml_dup_fast(data, len) \
        idmef_additional_data_new_str_dup_fast(IDMEF_ADDITIONAL_DATA_TYPE_XML, data, len)

#define idmef_additional_data_set_xml_dup_fast(ad, data, len) \
        idmef_additional_data_set_str_dup_fast(ad, IDMEF_ADDITIONAL_DATA_TYPE_XML, data, len)

#define idmef_additional_data_new_xml_nodup_fast(data, len) \
        idmef_additional_data_new_ptr_nodup_fast(IDMEF_ADDITIONAL_DATA_TYPE_XML, data, len + 1)

#define idmef_additional_data_set_xml_nodup_fast(ad, data, len) \
        idmef_additional_data_set_ptr_nodup_fast(ad, IDMEF_ADDITIONAL_DATA_TYPE_XML, data, len + 1)

#define idmef_additional_data_new_xml_ref(data) \
        idmef_additional_data_new_xml_ref_fast(data, strlen(data))

#define idmef_additional_data_set_xml_ref(ad, data) \
        idmef_additional_data_set_xml_ref_fast(ad, data, strlen(data))

#define idmef_additional_data_new_xml_dup(data) \
        idmef_additional_data_new_xml_dup_fast(data, strlen(data))

#define idmef_additional_data_set_xml_dup(data) \
        idmef_additional_data_set_xml_dup_fast(data, strlen(data))

#define idmef_additional_data_new_xml_nodup(data) \
        idmef_additional_data_new_xml_nodup_fast(data, strlen(data))

#define idmef_additional_data_set_xml_nodup(data) \
        idmef_additional_data_set_xml_nodup_fast(data, strlen(data))



/*
 * copy / clone / destroy
 */
void idmef_additional_data_destroy_internal(idmef_additional_data_t *data);
int idmef_additional_data_clone(idmef_additional_data_t *src, idmef_additional_data_t **dst);
int idmef_additional_data_copy_ref(idmef_additional_data_t *src, idmef_additional_data_t *dst);
int idmef_additional_data_copy_dup(idmef_additional_data_t *src, idmef_additional_data_t *dst);

/*
 * Accessors
 */

float idmef_additional_data_get_real(idmef_additional_data_t *data);
uint32_t idmef_additional_data_get_integer(idmef_additional_data_t *data);
prelude_bool_t idmef_additional_data_get_boolean(idmef_additional_data_t *data);
char idmef_additional_data_get_character(idmef_additional_data_t *data);

size_t idmef_additional_data_get_len(idmef_additional_data_t *data);

prelude_bool_t idmef_additional_data_is_empty(idmef_additional_data_t *data);

int idmef_additional_data_data_to_string(idmef_additional_data_t *ad, prelude_string_t *out);

#ifdef __cplusplus
 }
#endif
         
#endif /* _LIBPRELUDE_IDMEF_DATA_H */
