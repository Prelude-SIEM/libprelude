/*****
*
* Copyright (C) 2002,2003,2004,2005 PreludeIDS Technologies. All Rights Reserved.
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

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "prelude-log.h"
#include "prelude-list.h"
#include "prelude-inttypes.h"
#include "prelude-linked-object.h"
#include "prelude-async.h"
#include "prelude-io.h"
#include "prelude-msg.h"
#include "prelude-msgbuf.h"
#include "prelude-error.h"


struct prelude_msgbuf {
        int flags;
        void *data;
        prelude_msg_t *msg;
        int (*send_msg)(prelude_msgbuf_t *msgbuf, prelude_msg_t *msg);
};


static int default_send_msg_cb(prelude_msg_t **msg, void *data);


static int do_send_msg(prelude_msgbuf_t *msgbuf, prelude_msg_t *msg) 
{
        int ret;

        ret = msgbuf->send_msg(msgbuf, msg);
        if ( ret < 0 && prelude_error_get_code(ret) == PRELUDE_ERROR_EAGAIN )
                return ret;
        
        prelude_msg_recycle(msg);
        prelude_msg_set_priority(msg, PRELUDE_MSG_PRIORITY_NONE);
        
        return ret;
}



static int do_send_msg_async(prelude_msgbuf_t *msgbuf, prelude_msg_t *msg) 
{
        int ret;

        ret = msgbuf->send_msg(msgbuf, msg);
        if ( ret < 0 && prelude_error_get_code(ret) == PRELUDE_ERROR_EAGAIN )
                return ret;
        
        ret = prelude_msg_dynamic_new(&msgbuf->msg, default_send_msg_cb, msgbuf);
        if ( ret < 0 )
                return ret;

        return 0;
}



static int default_send_msg_cb(prelude_msg_t **msg, void *data)
{
        int ret;
        prelude_msgbuf_t *msgbuf = data;

        if ( msgbuf->flags & PRELUDE_MSGBUF_FLAGS_ASYNC )
                ret = do_send_msg_async(msgbuf, *msg);
        else
                ret = do_send_msg(msgbuf, *msg);

        *msg = msgbuf->msg;
        
        return ret;
}



/**
 * prelude_msgbuf_set:
 * @msgbuf: Pointer on a #prelude_msgbuf_t object to store the data to.
 * @tag: 8 bits unsigned integer describing the kind of data.
 * @len: len of the data chunk.
 * @data: Pointer to the data.
 *
 * prelude_msgbuf_set() append @len bytes of data from the @data buffer
 * to the @msgbuf object representing a message. The data is tagged with @tag.
 *
 * Returns: 0 on success, a negative value if an error occured.
 */
int prelude_msgbuf_set(prelude_msgbuf_t *msgbuf, uint8_t tag, uint32_t len, const void *data)
{
        return prelude_msg_set(msgbuf->msg, tag, len, data);
}




/**
 * prelude_msgbuf_new:
 * @msgbuf: Pointer where to store the created #prelude_msgbuf_t object.
 *
 * Create a new #prelude_msgbuf_t object and store it into @msgbuf.
 * You can then write data to @msgbuf using the prelude_msgbuf_set() function.
 *
 * When the message buffer is full, the message will be flushed using the
 * user provided callback.
 *
 * Returns: 0 on success, or a negative value if an error occured.
 */
int prelude_msgbuf_new(prelude_msgbuf_t **msgbuf)
{
        int ret;

        *msgbuf = calloc(1, sizeof(**msgbuf));
        if ( ! *msgbuf )
                return prelude_error_from_errno(errno);
        
        ret = prelude_msg_dynamic_new(&(*msgbuf)->msg, default_send_msg_cb, *msgbuf);     
        if ( ret < 0 )
                return ret;
        
        return 0;
}



/**
 * prelude_msgbuf_get_msg:
 * @msgbuf: Pointer on a #prelude_msgbuf_t object.
 *
 * Returns: This function return the current message associated with
 * the message buffer.
 */
prelude_msg_t *prelude_msgbuf_get_msg(prelude_msgbuf_t *msgbuf)
{
        return msgbuf->msg;
}



/**
 * prelude_msgbuf_mark_end:
 * @msgbuf: Pointer on #prelude_msgbuf_t object.
 *
 * This function should be called to tell the msgbuf subsystem
 * that you finished writing your message.
 */
void prelude_msgbuf_mark_end(prelude_msgbuf_t *msgbuf) 
{
        prelude_msg_mark_end(msgbuf->msg);
        
        /*
         * FIXME:
         * only flush the message if we're not under an alert burst.
         */
        default_send_msg_cb(&msgbuf->msg, msgbuf);
}




/**
 * prelude_msgbuf_destroy:
 * @msgbuf: Pointer on a #prelude_msgbuf_t object.
 *
 * Destroy @msgbuf, all data remaining will be flushed.
 */
void prelude_msgbuf_destroy(prelude_msgbuf_t *msgbuf) 
{        
        if ( msgbuf->msg && ! prelude_msg_is_empty(msgbuf->msg) )
                default_send_msg_cb(&msgbuf->msg, msgbuf);

        if ( msgbuf->msg )
                prelude_msg_destroy(msgbuf->msg);

        free(msgbuf);
}




/**
 * prelude_msgbuf_set_callback:
 * @msgbuf: Pointer on a #prelude_msgbuf_t object.
 * @send_msg: Pointer to a function for sending a message.
 *
 * Associate an application specific callback to this @msgbuf.
 */
void prelude_msgbuf_set_callback(prelude_msgbuf_t *msgbuf,
                                 int (*send_msg)(prelude_msgbuf_t *msgbuf, prelude_msg_t *msg))
{
        msgbuf->send_msg = send_msg;
}




void prelude_msgbuf_set_data(prelude_msgbuf_t *msgbuf, void *data) 
{
        msgbuf->data = data;
}



void *prelude_msgbuf_get_data(prelude_msgbuf_t *msgbuf)
{
        return msgbuf->data;
}


void prelude_msgbuf_set_flags(prelude_msgbuf_t *msgbuf, prelude_msgbuf_flags_t flags)
{        
        msgbuf->flags = flags;
}



prelude_msgbuf_flags_t prelude_msgbuf_get_flags(prelude_msgbuf_t *msgbuf)
{
        return msgbuf->flags;
}




