/*****
*
* Copyright (C) 2001, 2002 Yoann Vandoorselaere <yoann@prelude-ids.org>
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

#ifndef _LIBPRELUDE_PRELUDE_CONNECTION_H
#define _LIBPRELUDE_PRELUDE_CONNECTION_H

#include "prelude-msg.h"


#define PRELUDE_CONNECTION_ESTABLISHED    0x01
#define PRELUDE_CONNECTION_OWN_FD         0x02


typedef struct prelude_connection prelude_connection_t;

void prelude_connection_destroy(prelude_connection_t *cnx);

int prelude_connection_send_msg(prelude_connection_t *cnx, prelude_msg_t *msg);

int prelude_connection_connect(prelude_connection_t *cnx);

ssize_t prelude_connection_forward(prelude_connection_t *cnx, prelude_io_t *src, size_t count);

const char *prelude_connection_get_saddr(prelude_connection_t *cnx);

const char *prelude_connection_get_daddr(prelude_connection_t *cnx);

uint16_t prelude_connection_get_sport(prelude_connection_t *cnx);

uint16_t prelude_connection_get_dport(prelude_connection_t *cnx);

int prelude_connection_is_alive(prelude_connection_t *cnx);

prelude_io_t *prelude_connection_get_fd(prelude_connection_t *cnx);

void prelude_connection_close(prelude_connection_t *cnx);

void prelude_connection_set_fd(prelude_connection_t *cnx, prelude_io_t *fd);

void prelude_connection_set_state(prelude_connection_t *cnx, int state);

int prelude_connection_get_state(prelude_connection_t *cnx);

void prelude_connection_get_socket_filename(char *buf, size_t size, uint16_t port);


#include "prelude-client.h"

prelude_client_t *prelude_connection_get_client(prelude_connection_t *cnx);

prelude_connection_t *prelude_connection_new(prelude_client_t *client, const char *addr, uint16_t port);

#endif /* _LIBPRELUDE_PRELUDE_CONNECTION_H */
