/*****
*
* Copyright (C) 2002 Yoann Vandoorselaere <yoann@mandrakesoft.com>
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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <inttypes.h>

#include "prelude-log.h"
#include "prelude-list.h"
#include "prelude-async.h"
#include "prelude-io.h"
#include "prelude-message.h"
#include "sensor.h"
#include "prelude-message-buffered.h"


struct prelude_message_buffered {
        int async_send;
        prelude_msg_t *msg;
};



static prelude_msg_t *flush_msg_cb(void *data) 
{
        prelude_msgbuf_t *msgbuf = data;

        if ( msgbuf->async_send ) {
                prelude_sensor_send_msg_async(msgbuf->msg);

                msgbuf->msg = prelude_msg_dynamic_new(0, 0, data, flush_msg_cb);
                if ( ! msgbuf->msg ) 
                        log(LOG_ERR, "memory exhausted.\n");
                
        } else {
                prelude_sensor_send_msg(msgbuf->msg);
                prelude_msg_recycle(msgbuf->msg);
        }
        
        return msgbuf->msg;
}



/**
 * prelude_msgbuf_set_header:
 * @msgbuf: Pointer on #prelude_msgbuf_t object.
 * @tag: Tag to associate to the current message within @msgbuf.
 * @priority: Priority to associate to the current message within @msgbuf.
 *
 * Associate @tag and @priority, with the current message being constructed
 * in the @msgbuf message buffer. This function should be called before
 * prelude_msgbuf_mark_end().
 */
void prelude_msgbuf_set_header(prelude_msgbuf_t *msgbuf, uint8_t tag, uint8_t priority) 
{
        prelude_msg_set_tag(msgbuf->msg, tag);
        prelude_msg_set_priority(msgbuf->msg, priority);
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
 */
void prelude_msgbuf_set(prelude_msgbuf_t *msgbuf, uint8_t tag, uint32_t len, const void *data)
{
        prelude_msg_set(msgbuf->msg, tag, len, data);
}




/**
 * prelude_msgbuf_new:
 * @async_send: Tell if the message should be written asynchronously or not.
 *
 * Create a new #prelude_msgbuf_t object. You can then write data to
 * @msgbuf using the prelude_msgbuf_set function.
 *
 * When the message buffer is full, the message will be flushed either
 * asynchronously if @async_send is TRUE, or synchronously otherwise.
 *
 * Returns: a #prelude_msgbuf_t object, or NULL if an error occured.
 */
prelude_msgbuf_t *prelude_msgbuf_new(int async_send) 
{
        prelude_msgbuf_t *msgbuf;

        msgbuf = malloc(sizeof(*msgbuf));
        if ( ! msgbuf ) {
                log(LOG_ERR, "memory exhausted.\n");
                return NULL;
        }
        
        msgbuf->async_send = async_send;

        msgbuf->msg = prelude_msg_dynamic_new(0, 0, msgbuf, flush_msg_cb);
        if ( ! msgbuf->msg )
                return NULL;

        return msgbuf;
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
        flush_msg_cb(msgbuf);
}




/**
 * prelude_msgbuf_close:
 * @msgbuf: Pointer on a #prelude_msgbuf_t object.
 *
 * Close the message associated with @msgbuf,
 * all data remaining will be flushed before closing it.
 */
void prelude_msgbuf_close(prelude_msgbuf_t *msgbuf) 
{
        flush_msg_cb(msgbuf);
        
        if ( ! msgbuf->async_send )
                prelude_msg_destroy(msgbuf->msg);

        free(msgbuf);
}







