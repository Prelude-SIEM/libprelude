/*****
*
* Copyright (C) 2002 Yoann Vandoorselaere <yoann@prelude-ids.org>
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

#ifndef _LIBPRELUDE_PRELUDE_PATH_H
#define _LIBPRELUDE_PRELUDE_PATH_H


#include <inttypes.h>

void prelude_get_auth_filename(char *buf, size_t size);

void prelude_get_ssl_cert_filename(char *buf, size_t size);

void prelude_get_ssl_key_filename(char *buf, size_t size);

void prelude_get_backup_filename(char *buf, size_t size);

uid_t prelude_get_program_userid(void);

void prelude_set_program_name(const char *sname);

void prelude_set_program_userid(uid_t uid);

void prelude_get_socket_filename(char *buf, size_t size, uint16_t port); 

const char *prelude_get_sensor_name(void);

#endif /* _LIBPRELUDE_PRELUDE_PATH_H */
