/*****
*
* Copyright (C) 2001 Yoann Vandoorselaere <yoann@mandrakesoft.com>
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
#define SENSORS_AUTH_FILE CONFIG_DIR"/prelude-sensors.auth"
#define MANAGER_AUTH_FILE CONFIG_DIR"/prelude-manager.auth"


int prelude_auth_create_account(const char *filename);

int prelude_auth_send(prelude_io_t *fd, const char *addr);

int prelude_auth_recv(prelude_io_t *fd, const char *addr);
