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

#ifndef _LIBPRELUDE_PRELUDE_CLIENT_H
#define _LIBPRELUDE_PRELUDE_CLIENT_H

#include "prelude-message.h"


typedef struct prelude_client prelude_client_t;

void prelude_client_destroy(prelude_client_t *client);

prelude_client_t *prelude_client_new(const char *addr, uint16_t port);

int prelude_client_send_msg(prelude_client_t *client, prelude_msg_t *msg);

int prelude_client_connect(prelude_client_t *client);

ssize_t prelude_client_forward(prelude_client_t *client, prelude_io_t *src, size_t count);

const char *prelude_client_get_saddr(prelude_client_t *client);

const char *prelude_client_get_daddr(prelude_client_t *client);

uint16_t prelude_client_get_sport(prelude_client_t *client);

uint16_t prelude_client_get_dport(prelude_client_t *client);

int prelude_client_is_alive(prelude_client_t *client);

prelude_io_t *prelude_client_get_fd(prelude_client_t *client);

void prelude_client_close(prelude_client_t *client);

void prelude_client_set_fd(prelude_client_t *client, prelude_io_t *fd);

#define PRELUDE_CLIENT_TYPE_OTHER            0
#define PRELUDE_CLIENT_TYPE_SENSOR           1
#define PRELUDE_CLIENT_TYPE_MANAGER_PARENT   2
#define PRELUDE_CLIENT_TYPE_MANAGER_CHILDREN 3
#define PRELUDE_CLIENT_TYPE_ADMIN            4

void prelude_client_set_type(prelude_client_t *client, int type);

int prelude_client_get_type(prelude_client_t *client);


#define PRELUDE_CLIENT_CONNECTED    0x01
#define PRELUDE_CLIENT_OWN_FD       0x02

void prelude_client_set_state(prelude_client_t *client, int state);

int prelude_client_get_state(prelude_client_t *client);

/*
 * this one is located in client-ident.c
 */
uint64_t prelude_client_get_analyzerid(void);

#endif /* _LIBPRELUDE_PRELUDE_CLIENT_H */
