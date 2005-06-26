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

#ifndef _LIBPRELUDE_PRELUDE_MSGBUF_H
#define _LIBPRELUDE_PRELUDE_MSGBUF_H

#ifdef __cplusplus
 extern "C" {
#endif
         
typedef struct prelude_msgbuf prelude_msgbuf_t;

typedef enum {
        PRELUDE_MSGBUF_FLAGS_ASYNC = 0x01
} prelude_msgbuf_flags_t;


#include "prelude-client.h"
#include "prelude-msg.h"


int prelude_msgbuf_new(prelude_msgbuf_t **msgbuf);

void prelude_msgbuf_destroy(prelude_msgbuf_t *msgbuf);

void prelude_msgbuf_mark_end(prelude_msgbuf_t *msgbuf);

int prelude_msgbuf_set(prelude_msgbuf_t *msgbuf, uint8_t tag, uint32_t len, const void *data);

prelude_msg_t *prelude_msgbuf_get_msg(prelude_msgbuf_t *msgbuf);

void prelude_msgbuf_set_callback(prelude_msgbuf_t *msgbuf, int (*send_msg)(prelude_msgbuf_t *msgbuf, prelude_msg_t *msg));

void prelude_msgbuf_set_data(prelude_msgbuf_t *msgbuf, void *data);

void *prelude_msgbuf_get_data(prelude_msgbuf_t *msgbuf);

void prelude_msgbuf_set_flags(prelude_msgbuf_t *msgbuf, prelude_msgbuf_flags_t flags);

prelude_msgbuf_flags_t prelude_msgbuf_get_flags(prelude_msgbuf_t *msgbuf);

#ifdef __cplusplus
 }
#endif
         
#endif /* _LIBPRELUDE_PRELUDE_MSGBUF_H */
