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

#include <stdio.h>
#include <string.h>

#include "idmef-data.h"
#include "idmef-string.h"


idmef_string_t *idmef_string_new(void) 
{
        return idmef_data_new();
}



idmef_string_t *idmef_string_new_ref(const char *str)
{
        return idmef_data_new_ref(str, strlen(str) + 1);
}



idmef_string_t *idmef_string_new_dup(const char *str)
{
        return idmef_data_new_dup(str, strlen(str) + 1);
}



idmef_string_t *idmef_string_new_nodup(char *str)
{
        return idmef_data_new_nodup(str, strlen(str) + 1);
}



void idmef_string_destroy(idmef_string_t *string)
{
        idmef_data_destroy(string);
}



void idmef_string_destroy_internal(idmef_string_t *string)
{
        idmef_data_destroy_internal(string);
}



idmef_string_t *idmef_string_new_dup_fast(const char *str, size_t len)
{
        return idmef_data_new_dup(str, len);
}



idmef_string_t *idmef_string_new_nodup_fast(char *str, size_t len)
{
        return idmef_data_new_nodup(str, len);
}



idmef_string_t *idmef_string_new_ref_fast(const char *str, int len)
{
        return idmef_data_new_ref(str, len);
}



int idmef_string_set_dup_fast(idmef_string_t *string, const char *str, size_t len)
{
        return idmef_data_set_dup(string, str, len);
}



int idmef_string_set_dup(idmef_string_t *string, const char *str)
{
        return idmef_data_set_dup(string, str, strlen(str) + 1);
}



int idmef_string_set_nodup_fast(idmef_string_t *string, char *str, size_t len)
{
        return idmef_data_set_nodup(string, str, len);
}



int idmef_string_set_nodup(idmef_string_t *string, char *str)
{
        return idmef_data_set_nodup(string, str, strlen(str) + 1);
}




int idmef_string_set_ref_fast(idmef_string_t *string, const char *str, size_t len)
{
        return idmef_data_set_ref(string, str, len);
}



int idmef_string_set_ref(idmef_string_t *string, const char *str)
{
        return idmef_data_set_ref(string, str, strlen(str) + 1);
}



int idmef_string_copy_ref(idmef_string_t *dst, const idmef_string_t *src)
{
        return idmef_data_copy_ref(dst, src);
}



int idmef_string_copy_dup(idmef_string_t *dst, const idmef_string_t *src)
{
        return idmef_data_copy_dup(dst, src);
}



idmef_string_t *idmef_string_clone(const idmef_string_t *src)
{
        return idmef_data_clone(src);
}



size_t idmef_string_get_len(idmef_string_t *string)
{
        return idmef_data_get_len(string);
}



char *idmef_string_get_string(const idmef_string_t *string)
{
        return idmef_data_get_data(string);
}
