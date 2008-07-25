/*****
*
* Copyright (C) 2001, 2002, 2003, 2004, 2005 PreludeIDS Technologies. All Rights Reserved.
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

#ifndef _LIBPRELUDE_PRELUDE_CONNECTION_H
#define _LIBPRELUDE_PRELUDE_CONNECTION_H

#ifdef __cplusplus
 extern "C" {
#endif

typedef enum {
        PRELUDE_CONNECTION_PERMISSION_IDMEF_READ      = 0x01, /* client might read received IDMEF message */
        PRELUDE_CONNECTION_PERMISSION_ADMIN_READ      = 0x02, /* client might read received ADMIN message */
        PRELUDE_CONNECTION_PERMISSION_IDMEF_WRITE     = 0x04, /* client might send IDMEF message          */
        PRELUDE_CONNECTION_PERMISSION_ADMIN_WRITE     = 0x08  /* client might issue OPTION request        */
} prelude_connection_permission_t;

typedef enum {
        PRELUDE_CONNECTION_STATE_ESTABLISHED     = 0x01
} prelude_connection_state_t;


typedef struct prelude_connection prelude_connection_t;


#include "prelude-msg.h"
#include "prelude-msgbuf.h"
#include "prelude-string.h"
#include "prelude-client-profile.h"
#include "idmef.h"


void prelude_connection_destroy(prelude_connection_t *conn);

prelude_connection_t *prelude_connection_ref(prelude_connection_t *conn);

int prelude_connection_send(prelude_connection_t *cnx, prelude_msg_t *msg);

int prelude_connection_recv(prelude_connection_t *cnx, prelude_msg_t **outmsg);

int prelude_connection_recv_idmef(prelude_connection_t *con, idmef_message_t **idmef);

int prelude_connection_connect(prelude_connection_t *cnx,
                               prelude_client_profile_t *profile,
                               prelude_connection_permission_t permission);

ssize_t prelude_connection_forward(prelude_connection_t *cnx, prelude_io_t *src, size_t count);

const char *prelude_connection_get_local_addr(prelude_connection_t *cnx);

unsigned int prelude_connection_get_local_port(prelude_connection_t *cnx);

const char *prelude_connection_get_peer_addr(prelude_connection_t *cnx);

unsigned int prelude_connection_get_peer_port(prelude_connection_t *cnx);

prelude_bool_t prelude_connection_is_alive(prelude_connection_t *cnx);

prelude_io_t *prelude_connection_get_fd(prelude_connection_t *cnx);

int prelude_connection_close(prelude_connection_t *cnx);

void prelude_connection_set_fd_ref(prelude_connection_t *cnx, prelude_io_t *fd);

void prelude_connection_set_fd_nodup(prelude_connection_t *cnx, prelude_io_t *fd);

void prelude_connection_set_state(prelude_connection_t *cnx, prelude_connection_state_t state);

prelude_connection_state_t prelude_connection_get_state(prelude_connection_t *cnx);

void prelude_connection_set_data(prelude_connection_t *cnx, void *data);

void *prelude_connection_get_data(prelude_connection_t *cnx);

const char *prelude_connection_get_default_socket_filename(void);

prelude_connection_permission_t prelude_connection_get_permission(prelude_connection_t *conn);

uint64_t prelude_connection_get_peer_analyzerid(prelude_connection_t *cnx);

void prelude_connection_set_peer_analyzerid(prelude_connection_t *cnx, uint64_t analyzerid);

#include "prelude-client.h"

int prelude_connection_new(prelude_connection_t **ret, const char *addr);

int prelude_connection_new_msgbuf(prelude_connection_t *connection, prelude_msgbuf_t **msgbuf);

int prelude_connection_permission_to_string(prelude_connection_permission_t perm, prelude_string_t *out);

int prelude_connection_permission_new_from_string(prelude_connection_permission_t *out, const char *buf);

prelude_connection_t *prelude_connection_ref(prelude_connection_t *conn);

#ifdef __cplusplus
 }
#endif

#endif /* _LIBPRELUDE_PRELUDE_CONNECTION_H */
