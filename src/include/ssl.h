/*****
*
* Copyright (C) 1998 - 2000 Yoann Vandoorselaere <yoann@mandrakesoft.com>
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

#include <openssl/ssl.h>

/*
 * Place where the Manager certificate is saved.
 */
#define MANAGERS_CERT CONFIG_DIR"/managers.cert"
#define SENSORS_CERT CONFIG_DIR"/sensors.cert"

#define MANAGER_KEY CONFIG_DIR"/manager.key"
#define SENSORS_KEY CONFIG_DIR"/sensors.key"



int ssl_init_client(void);

SSL *ssl_connect_server(int socket);
