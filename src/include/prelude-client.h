/*****
*
* Copyright (C) 2001, 2002 Yoann Vandoorselaere <yoann@mandrakesoft.com>
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


#define PRELUDE_CLIENT_TYPE_OTHER   0
#define PRELUDE_CLIENT_TYPE_SENSOR  1
#define PRELUDE_CLIENT_TYPE_MANAGER 2

void prelude_client_set_type(prelude_client_t *client, int type);


/*
 * this one is located in client-ident.c
 */
uint64_t prelude_client_get_analyzerid(void);

#endif /* _LIBPRELUDE_PRELUDE_CLIENT_H */
