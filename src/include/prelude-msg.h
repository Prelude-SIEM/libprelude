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

#ifndef _LIBPRELUDE_PRELUDE_MESSAGE_H
#define _LIBPRELUDE_PRELUDE_MESSAGE_H

#include "prelude-io.h"


#ifdef __cplusplus
 extern "C" {
#endif

typedef struct prelude_msg prelude_msg_t;


typedef enum {
        PRELUDE_MSG_PRIORITY_NONE = 0,
        PRELUDE_MSG_PRIORITY_LOW  = 1,
        PRELUDE_MSG_PRIORITY_MID  = 2,
        PRELUDE_MSG_PRIORITY_HIGH = 3
} prelude_msg_priority_t;


int prelude_msg_read(prelude_msg_t **msg, prelude_io_t *pio);

int prelude_msg_forward(prelude_msg_t *msg, prelude_io_t *dst, prelude_io_t *src);

int prelude_msg_get(prelude_msg_t *msg, uint8_t *tag, uint32_t *len, void **buf);



/*
 * Write function.
 */
void prelude_msg_recycle(prelude_msg_t *msg);

void prelude_msg_mark_end(prelude_msg_t *msg);

int prelude_msg_dynamic_new(prelude_msg_t **ret, int (*flush_msg_cb)(prelude_msg_t **msg, void *data), void *data);

int prelude_msg_new(prelude_msg_t **ret, size_t msgcount, size_t msglen, uint8_t tag, prelude_msg_priority_t priority);

int prelude_msg_set(prelude_msg_t *msg, uint8_t tag, uint32_t len, const void *data);

int prelude_msg_write(prelude_msg_t *msg, prelude_io_t *dst);



/*
 *
 */
void prelude_msg_set_tag(prelude_msg_t *msg, uint8_t tag);

void prelude_msg_set_priority(prelude_msg_t *msg, prelude_msg_priority_t priority);

uint8_t prelude_msg_get_tag(prelude_msg_t *msg);

prelude_msg_priority_t prelude_msg_get_priority(prelude_msg_t *msg);

uint32_t prelude_msg_get_len(prelude_msg_t *msg);

uint32_t prelude_msg_get_datalen(prelude_msg_t *msg);

const unsigned char *prelude_msg_get_message_data(prelude_msg_t *msg);

struct timeval *prelude_msg_get_time(prelude_msg_t *msg, struct timeval *tv);

int prelude_msg_is_empty(prelude_msg_t *msg);

int prelude_msg_is_fragment(prelude_msg_t *msg);

void prelude_msg_destroy(prelude_msg_t *msg);

void prelude_msg_set_callback(prelude_msg_t *msg, int (*flush_msg_cb)(prelude_msg_t **msg, void *data));

void prelude_msg_set_data(prelude_msg_t *msg, void *data);

prelude_msg_t *prelude_msg_ref(prelude_msg_t *msg);

#ifdef __cplusplus
 }
#endif

#endif /* _LIBPRELUDE_PRELUDE_MESSAGE_H */
