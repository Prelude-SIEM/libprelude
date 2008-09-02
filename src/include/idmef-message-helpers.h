/*****
*
* Copyright (C) 2008 PreludeIDS Technologies. All Rights Reserved.
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


int idmef_message_set_value(idmef_message_t *message, const char *path, idmef_value_t *value);

int idmef_message_get_value(idmef_message_t *message, const char *path, idmef_value_t **value);

int idmef_message_set_string(idmef_message_t *message, const char *path, const char *value);

int idmef_message_get_string(idmef_message_t *message, const char *path, char **result);

int idmef_message_set_number(idmef_message_t *message, const char *path, double number);

int idmef_message_get_number(idmef_message_t *message, const char *path, double *result);

int idmef_message_set_data(idmef_message_t *message, const char *path, const unsigned char *data, size_t size);

int idmef_message_get_data(idmef_message_t *message, const char *path, unsigned char **data, size_t *size);
