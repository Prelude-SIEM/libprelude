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


typedef struct prelude_client prelude_client_t;

void prelude_client_destroy(prelude_client_t *client);

prelude_client_t *prelude_client_new(const char *addr, uint16_t port);

int prelude_client_send_msg(prelude_client_t *client, prelude_msg_t *msg);

int prelude_client_connect(prelude_client_t *client);

ssize_t prelude_client_forward(prelude_client_t *client, prelude_io_t *src, size_t count);

void prelude_client_get_address(prelude_client_t *client, char **addr, uint16_t *port);

