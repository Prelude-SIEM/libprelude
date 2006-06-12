/*****
*
* Copyright (C) 2004, 2005 PreludeIDS Technologies. All Rights Reserved.
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


#define _IDMEF_ADDITIONAL_DATA_DECL(name) \
int idmef_additional_data_new_ ## name ## _ref_fast(idmef_additional_data_t **ad, const char *data, size_t len); \
int idmef_additional_data_new_ ## name ## _ref(idmef_additional_data_t **ad, const char *data); \
int idmef_additional_data_set_ ## name ## _ref_fast(idmef_additional_data_t *ad, const char *data, size_t len); \
int idmef_additional_data_set_ ## name ## _ref(idmef_additional_data_t *ad, const char *data); \
int idmef_additional_data_new_ ## name ## _dup_fast(idmef_additional_data_t **ad, const char *data, size_t len); \
int idmef_additional_data_new_ ## name ## _dup(idmef_additional_data_t **ad, const char *data); \
int idmef_additional_data_set_ ## name ## _dup_fast(idmef_additional_data_t *ad, const char *data, size_t len); \
int idmef_additional_data_set_ ## name ## _dup(idmef_additional_data_t *ad, const char *data); \
int idmef_additional_data_new_ ## name ## _nodup_fast(idmef_additional_data_t **ad, char *data, size_t len); \
int idmef_additional_data_new_ ## name ## _nodup(idmef_additional_data_t **ad, char *data); \
int idmef_additional_data_set_ ## name ## _nodup_fast(idmef_additional_data_t *ad, char *data, size_t len); \
int idmef_additional_data_set_ ## name ## _nodup(idmef_additional_data_t *ad, char *data);

_IDMEF_ADDITIONAL_DATA_DECL(string)
_IDMEF_ADDITIONAL_DATA_DECL(ntpstamp)
_IDMEF_ADDITIONAL_DATA_DECL(date_time)
_IDMEF_ADDITIONAL_DATA_DECL(portlist)
_IDMEF_ADDITIONAL_DATA_DECL(xml)

int idmef_additional_data_new_byte_string_ref(idmef_additional_data_t **ad, const unsigned char *data, size_t len);
int idmef_additional_data_set_byte_string_ref(idmef_additional_data_t *ad, const unsigned char *data, size_t len);
int idmef_additional_data_new_byte_string_dup(idmef_additional_data_t **ad, const unsigned char *data, size_t len);
int idmef_additional_data_set_byte_string_dup(idmef_additional_data_t *ad, const unsigned char *data, size_t len);
int idmef_additional_data_new_byte_string_nodup(idmef_additional_data_t **ad, unsigned char *data, size_t len);
int idmef_additional_data_set_byte_string_nodup(idmef_additional_data_t *ad, unsigned char *data, size_t len);
         

         
/*
 * copy / clone / destroy
 */
int idmef_additional_data_copy_ref(idmef_additional_data_t *src, idmef_additional_data_t *dst);
int idmef_additional_data_copy_dup(idmef_additional_data_t *src, idmef_additional_data_t *dst);

/*
 * Accessors
 */

float idmef_additional_data_get_real(idmef_additional_data_t *data);
uint32_t idmef_additional_data_get_integer(idmef_additional_data_t *data);
prelude_bool_t idmef_additional_data_get_boolean(idmef_additional_data_t *data);
char idmef_additional_data_get_character(idmef_additional_data_t *data);
uint8_t idmef_additional_data_get_byte(idmef_additional_data_t *data);

size_t idmef_additional_data_get_len(idmef_additional_data_t *data);

prelude_bool_t idmef_additional_data_is_empty(idmef_additional_data_t *data);

int idmef_additional_data_data_to_string(idmef_additional_data_t *ad, prelude_string_t *out);

#ifdef __cplusplus
 }
#endif
         
#endif /* _LIBPRELUDE_IDMEF_DATA_H */
