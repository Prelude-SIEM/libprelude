/*****
*
* Copyright (C) 2001-2006,2007,2008 PreludeIDS Technologies. All Rights Reserved.
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
#include "libmissing.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <assert.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#if TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# if HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif

#define PRELUDE_ERROR_SOURCE_DEFAULT PRELUDE_ERROR_SOURCE_MESSAGE
#include "prelude-error.h"

#include "common.h"
#include "prelude-log.h"
#include "prelude-inttypes.h"
#include "prelude-extract.h"
#include "prelude-io.h"
#include "prelude-linked-object.h"
#include "prelude-async.h"
#include "prelude-msg.h"


#define MSGBUF_SIZE 8192
#define PRELUDE_MSG_VERSION 1
#define PRELUDE_MSG_HDR_SIZE 16
#define MINIMUM_FRAGMENT_DATA_SIZE 1



typedef struct {
        uint8_t version;
        uint8_t tag;
        uint8_t priority;
        uint8_t is_fragment;
        uint32_t datalen;
        uint32_t tv_sec;
        uint32_t tv_usec;
} prelude_msg_hdr_t;




struct prelude_msg {
        PRELUDE_ASYNC_OBJECT;

        int refcount;

        uint32_t read_index;
        uint32_t header_index;
        uint32_t write_index;
        uint32_t fd_write_index;

        prelude_msg_hdr_t hdr;
        unsigned char hdrbuf[PRELUDE_MSG_HDR_SIZE];
        unsigned char *payload;

        void *send_msg_data;
        int (*flush_msg_cb)(prelude_msg_t **msg, void *data);
};


/*
 * Anything bigger than this will be discarded.
 */
static const size_t max_message_size = 100 * 1024 * 1024;



static int call_alloc_cb(prelude_msg_t **msg)
{
        int ret;

        ret = (*msg)->flush_msg_cb(msg, (*msg)->send_msg_data);
        if ( ret < 0 )
                return ret;

        /*
         * Within the callback, the caller have the choise to use
         * a newly allocated message or to flush the current message and
         * reuse it.
         *
         * We have to reset write_index and is_fragment header member
         * in order to properly address the message buffer. And in order
         * for not all message to not look like fragment.
         */
        (*msg)->header_index = 0;
        (*msg)->write_index = PRELUDE_MSG_HDR_SIZE;
        (*msg)->hdr.is_fragment = 0;

        return 0;
}



static void write_message_header(prelude_msg_t *msg)
{
        uint32_t dlen;
        struct timeval tv;
        uint32_t hdr_offset = msg->header_index;

        dlen = htonl(msg->write_index - msg->header_index - PRELUDE_MSG_HDR_SIZE);

        msg->payload[hdr_offset++] = PRELUDE_MSG_VERSION;
        msg->payload[hdr_offset++] = msg->hdr.tag;
        msg->payload[hdr_offset++] = msg->hdr.priority;
        msg->payload[hdr_offset++] = msg->hdr.is_fragment;

        gettimeofday(&tv, NULL);

        msg->hdr.tv_sec = htonl(tv.tv_sec);
        msg->hdr.tv_usec = htonl(tv.tv_usec);

        memcpy(&msg->payload[hdr_offset], &dlen, sizeof(dlen));
        memcpy(&msg->payload[hdr_offset + 4], &msg->hdr.tv_sec, sizeof(msg->hdr.tv_sec));
        memcpy(&msg->payload[hdr_offset + 8], &msg->hdr.tv_usec, sizeof(msg->hdr.tv_usec));
}




static int set_data(prelude_msg_t **m, const void *buf, size_t size)
{
        int ret;
        size_t remaining;
        prelude_msg_t *msg = *m;

        remaining = (msg->hdr.datalen - msg->write_index);
        assert(msg->flush_msg_cb != NULL || remaining >= size);

        if ( size > remaining ) {

                /*
                 * there is not enough free buffer space to store the whole
                 * data in the message. Store what we can, and call the message
                 * flushing function, which'll emit the current buffer
                 * and allocate a new one.
                 */
                memcpy(msg->payload + msg->write_index, buf, remaining);

                size -= remaining;
                msg->write_index += remaining;
                buf = (const uint8_t *) buf + remaining;

                /*
                 * this is a fragment of message.
                 */
                msg->hdr.is_fragment = 1;

                /*
                 * the caller might destroy the message after this and re-allocate
                 * one... or use synchronous send and reuse the same message.
                 */
                ret = call_alloc_cb(m);
                if ( ret < 0 )
                        return ret;

                return set_data(m, buf, size);
        }

        memcpy(msg->payload + msg->write_index, buf, size);
        msg->write_index += size;

        return 0;
}




inline static int read_message_data(unsigned char *dst, size_t *size, prelude_io_t *fd)
{
        ssize_t ret;
        size_t count = *size;

        *size = 0;

        /*
         * Read the whole header.
         */
        do {
                ret = prelude_io_read(fd, dst + *size, count - *size);
                if ( ret < 0 )
                        return ret;

                *size += ret;

        } while ( *size != count );

        return 0;
}





inline static int slice_message_header(prelude_msg_t *msg, unsigned char *hdrbuf)
{
        uint32_t tmp;

        if ( ! msg->hdr.datalen ) {
                /*
                 * tag and priority are set on first fragment only.
                 */
                msg->hdr.version = hdrbuf[0];
                msg->hdr.tag = hdrbuf[1];
                msg->hdr.tv_sec = prelude_extract_uint32(hdrbuf + 8);
                msg->hdr.tv_usec = prelude_extract_uint32(hdrbuf + 12);
        }

        /*
         * message priority might be set in any fragment.
         */
        if ( msg->hdr.priority == PRELUDE_MSG_PRIORITY_NONE )
                msg->hdr.priority = hdrbuf[2];

        msg->hdr.is_fragment = hdrbuf[3];
        tmp = prelude_extract_uint32(hdrbuf + 4);

        if ( (msg->hdr.datalen + tmp) <= msg->hdr.datalen )
                return prelude_error(PRELUDE_ERROR_INVAL_LENGTH);

        if ( (msg->hdr.datalen + tmp) >= max_message_size )
                return prelude_error_verbose(PRELUDE_ERROR_INVAL_LENGTH, "maximum message size exceeded: %" PRELUDE_PRIu64 " > %" PRELUDE_PRIu64,
                                             (uint64_t) (msg->hdr.datalen + tmp), (uint64_t) max_message_size);

        msg->hdr.datalen += tmp;

        return 0;
}




static int read_message_header(prelude_msg_t **msgptr, prelude_io_t *fd)
{
        int ret;
        size_t count;
        uint32_t old_dlen;
        prelude_msg_t *msg = *msgptr;
        unsigned char *hdrptr = &msg->hdrbuf[msg->header_index];

        count = PRELUDE_MSG_HDR_SIZE - msg->header_index;

        ret = read_message_data(hdrptr, &count, fd);
        msg->header_index += count;

        if ( ret < 0 )
                return ret;

        /*
         * we have a full header. Move it from our buffer
         * into a real header structure.
         */
        old_dlen = msg->hdr.datalen;
        ret = slice_message_header(msg, msg->hdrbuf);
        if ( ret < 0 )
                return ret;

        /*
         * sanity check. An attacker could arrange to make datalen
         * wrap arround by specifying an odd dlen in a fragment header.
         */
        if ( (msg->hdr.datalen + PRELUDE_MSG_HDR_SIZE) <= old_dlen )
                return prelude_error(PRELUDE_ERROR_INVAL_LENGTH);

        /*
         * Check protocol version.
         */
        if ( msg->hdr.version != PRELUDE_MSG_VERSION )
                return prelude_error_verbose(PRELUDE_ERROR_PROTOCOL_VERSION, "invalid protocol version '%d' (expected %d)",
                                             msg->hdr.version, PRELUDE_MSG_VERSION);

        msg->write_index = msg->hdr.datalen + PRELUDE_MSG_HDR_SIZE;

        /*
         * allocate our data buffer. We also want our buffer to be able to contain an
         * header so that it can be eventually sent...
         */

        msg = _prelude_realloc(msg, sizeof(*msg) + PRELUDE_MSG_HDR_SIZE + msg->hdr.datalen);
        if ( ! msg )
                return prelude_error_from_errno(errno);

        *msgptr = msg;
        msg->payload = ((unsigned char *) msg) + sizeof(*msg);

        return 0;
}




static int read_message_content(prelude_msg_t **m, prelude_io_t *fd)
{
        int ret;
        size_t count;
        prelude_msg_t *msg = *m;

        count = (msg->hdr.datalen + PRELUDE_MSG_HDR_SIZE) - msg->read_index;

        ret = read_message_data(&msg->payload[msg->read_index], &count, fd);
        msg->read_index += count;

        if ( ret != 0 )
                /*
                 * there is still data to be read.
                 */
                return ret;

        if ( msg->hdr.is_fragment ) {
                /*
                 * We just finished reading one fragment (not the last one).
                 * Next bytes will be another message header. So reset header_index
                 * to trigger header read on next prelude_msg_read() call.
                 */
                msg->header_index = 0;
                return prelude_msg_read(m, fd);
        }


        /*
         * we now have a full message.
         *
         * reset is_fragment to 0, so that if the message is written in the future,
         * it won't be marked as fragmented (we already defragmented it).
         *
         * Reset header_index to 0 because we would compute an invalid datalen
         * if this message is to be written in the future.
         *
         * Set read index to the point where data start, so that our buffer is correctly
         * addressed.
         */
        msg->hdr.is_fragment = 0;
        msg->header_index = 0;
        msg->read_index = PRELUDE_MSG_HDR_SIZE;

        return ret;
}






/**
 * prelude_msg_read:
 * @msg: Pointer on a #prelude_msg_t object address.
 * @pio: Pointer on a #prelude_io_t object.
 *
 * Read a message on @pio into @msg. If @msg is NULL, it is
 * allocated. This function will never block.
 *
 * Returns: 0 if reading the message is complete, or a #prelude_error_t
 * error if an error occured. Take particular attention to #PRELUDE_ERROR_EAGAIN
 * and PRELUDE_ERROR_EOF.
 */
int prelude_msg_read(prelude_msg_t **msg, prelude_io_t *pio)
{
        int ret = 0;

        /*
         * *msg is NULL,
         * this mean the caller want to work on a new message.
         */
        if ( ! *msg ) {
                *msg = malloc(sizeof(prelude_msg_t));
                if ( ! *msg )
                        return prelude_error_from_errno(errno);

                (*msg)->refcount = 1;
                (*msg)->hdr.datalen = 0;
                (*msg)->hdr.priority = PRELUDE_MSG_PRIORITY_NONE;

                (*msg)->payload = NULL;
                (*msg)->write_index = 0;
                (*msg)->header_index = 0;
                (*msg)->fd_write_index = 0;
                (*msg)->read_index = PRELUDE_MSG_HDR_SIZE;
        }

        /*
         * We didn't finished reading the message header yet.
         */
        if ( (*msg)->header_index != PRELUDE_MSG_HDR_SIZE )
                ret = read_message_header(msg, pio);

        /*
         * Notice that ret is initialized to 0 so that we will read
         * the message if this function is called and we already have
         * the header.
         *
         * In case read_message_header return PRELUDE_ERROR_EAGAIN,
         * we don't want to try to read the rest of the message right now,
         * as it is unlikely we can read something.
         *
         * In case it return 0, there is some chance there are other data
         * waiting to be read.
         */
        if ( (*msg)->payload && ret == 0 )
                ret = read_message_content(msg, pio);

        /*
         * check for msg as well since this function can be called recursively.
         */
        if ( *msg && ret < 0 && prelude_error_get_code(ret) != PRELUDE_ERROR_EAGAIN ) {
                prelude_msg_destroy(*msg);
                /*
                 * reset message to NULL, because the caller might not take
                 * care of the return value enough (and may call us again with an
                 * undefined *msg address.
                 */
                *msg = NULL;
        }

        return ret;
}




/**
 * prelude_msg_get:
 * @msg: Pointer on a #prelude_msg_t object representing the message to get data from.
 * @tag: Pointer on a 8 bits unsigned integer to store the message tag.
 * @len: Pointer on a 32 bits unsigned integer to store the message len to.
 * @buf: Address of a pointer to store the buffer starting address.
 *
 * prelude_msg_get() read the next data chunk contained in the message.
 * @tag is updated to contain the kind of data the chunk contain.
 * @len is updated to contain the len of the data chunk.
 * @buf is updated to point on the data chunk.
 *
 * Returns: 0 on success, or a #prelude_error_t value on error.
 */
int prelude_msg_get(prelude_msg_t *msg, uint8_t *tag, uint32_t *len, void **buf)
{
        if ( msg->read_index == (msg->hdr.datalen + PRELUDE_MSG_HDR_SIZE) ) {
                /*
                 * no more sub - messages in the buffer.
                 */
                return prelude_error(PRELUDE_ERROR_EOF);
        }

        /*
         * bound check our buffer,
         * so that we won't overflow if it doesn't contain tag and len.
         */
        if ( msg->read_index > (msg->hdr.datalen + PRELUDE_MSG_HDR_SIZE - 5) )
                return prelude_error(PRELUDE_ERROR_INVAL_LENGTH);

        /*
         * slice wanted data.
         */
        *tag = msg->payload[msg->read_index++];
        *len = prelude_extract_uint32(&msg->payload[msg->read_index]);
        msg->read_index += sizeof(uint32_t);

        if ( ! *len )
                return 0;

        /*
         * bound check again, against specified len.
         */
        if ( (msg->read_index + *len) <= msg->read_index )
                return prelude_error(PRELUDE_ERROR_INVAL_LENGTH);

        if ( (msg->read_index + *len) > (msg->hdr.datalen + PRELUDE_MSG_HDR_SIZE) )
                return prelude_error(PRELUDE_ERROR_INVAL_LENGTH);

        *buf = &msg->payload[msg->read_index];
        msg->read_index += *len;

        return 0;
}




/**
 * prelude_msg_forward:
 * @msg: Pointer on a #prelude_msg_t object containing a message header.
 * @dst: Pointer on a #prelude_io_t object to send message to.
 * @src: Pointer on a #prelude_io_t object to read message from.
 *
 * prelude_msg_forward() read the message corresponding to the @msg object
 * containing the message header previously gathered using prelude_msg_read_header()
 * from the @src object, and transfer it to @dst. The header is also transfered.
 *
 * Returns: 0 on success, or a negative value if an error occured.
 */
int prelude_msg_forward(prelude_msg_t *msg, prelude_io_t *dst, prelude_io_t *src)
{
        ssize_t ret;
        uint32_t dlen = htonl(msg->hdr.datalen);
        unsigned char buf[PRELUDE_MSG_HDR_SIZE];

        buf[0] = msg->hdr.version;
        buf[1] = msg->hdr.tag;
        buf[2] = msg->hdr.priority;
        buf[3] = msg->hdr.is_fragment;

        memcpy(&buf[4], &dlen, sizeof(dlen));

        ret = prelude_io_write(dst, buf, sizeof(buf));
        if ( ret < 0 )
                return ret;

        ret = prelude_io_forward(dst, src, msg->hdr.datalen);
        if ( ret < 0 )
                return ret;

        return 0;
}




/**
 * prelude_msg_write:
 * @msg: Pointer on a #prelude_msg_t object containing the message.
 * @dst: Pointer on a #prelude_io_t object to send the message to.
 *
 * prelude_msg_write() write the message corresponding to the @msg
 * object to @dst. The message should have been created using the
 * prelude_msg_new() and prelude_msg_set() functions.
 *
 * Returns: 0 on success, or a negative value if an error occured.
 */
int prelude_msg_write(prelude_msg_t *msg, prelude_io_t *dst)
{
        ssize_t ret;
        uint32_t dlen = msg->write_index;

        /*
         * no need to send... There's no data in this message.
         */
        if ( msg->write_index - PRELUDE_MSG_HDR_SIZE <= 0 )
                return 0;

        /*
         * if the message header index is 0 (write called, without
         * prelude_msg_mark_end() first), mark end of the message
         * cause the caller didn't do it in this case.
         */
        if ( msg->header_index == 0 )
                write_message_header(msg);

        /*
         * in this case, prelude_msg_mark_end() was called.
         */
        else if ( ! msg->hdr.is_fragment )
                dlen -= PRELUDE_MSG_HDR_SIZE;

        ret = prelude_io_write(dst, msg->payload + msg->fd_write_index, dlen - msg->fd_write_index);
        if ( ret < 0 )
                return ret;

        msg->fd_write_index += ret;

        if ( msg->fd_write_index == dlen ) {
                msg->fd_write_index = 0;
                return 0;
        }

        return prelude_error(PRELUDE_ERROR_EAGAIN);
}




/**
 * prelude_msg_recycle:
 * @msg: Pointer on #prelude_msg_t object.
 *
 * Recycle @msg so you can write at it again, even
 * thought it was written.
 */
void prelude_msg_recycle(prelude_msg_t *msg)
{
        msg->header_index = 0;
        msg->write_index = PRELUDE_MSG_HDR_SIZE;
        msg->payload = (unsigned char *) msg + sizeof(*msg);

        if ( msg->read_index )
                msg->read_index = PRELUDE_MSG_HDR_SIZE;
}



/**
 * prelude_msg_mark_end:
 * @msg: Pointer on #prelude_msg_t object.
 *
 * Mark end of message in the @msg buffer, so you can continue
 * adding different message in the same buffer.
 */
void prelude_msg_mark_end(prelude_msg_t *msg)
{
        int ret;

        if ( msg->write_index - msg->header_index - PRELUDE_MSG_HDR_SIZE <= 0 )
                return;

        write_message_header(msg);

        if ( msg->write_index + PRELUDE_MSG_HDR_SIZE + MINIMUM_FRAGMENT_DATA_SIZE > msg->hdr.datalen ) {

                ret = call_alloc_cb(&msg);
                if ( ret < 0 )
                        return;
        } else {
                msg->header_index = msg->write_index;
                msg->write_index += PRELUDE_MSG_HDR_SIZE;
        }
}




/**
 * prelude_msg_set:
 * @msg: Pointer on a #prelude_msg_t object to store the data to.
 * @tag: 8 bits unsigned integer describing the kind of data.
 * @len: len of the data chunk.
 * @data: Pointer to the starting address of the data.
 *
 * prelude_msg_set() append @len bytes of data from the @data buffer
 * to the @msg object representing a message. The data is tagged with @tag.
 *
 * Returns: 0 on success, or a negative value if the remaining space is not
 * available. You might check the return value mostly if using a dynamic message
 * through prelude_msg_dynamic_new()
 */
int prelude_msg_set(prelude_msg_t *msg, uint8_t tag, uint32_t len, const void *data)
{
        int ret;
        uint32_t l;

        l = htonl(len);

        ret = set_data(&msg, &tag, sizeof(tag));
        if ( ret < 0 )
                return ret;

        ret = set_data(&msg, &l, sizeof(l));
        if ( ret < 0 )
                return ret;

        return set_data(&msg, data, len);
}





/**
 * prelude_msg_new:
 * @ret: Pointer where to store the created #prelude_msg_t.
 * @msgcount: Number of chunk of data the created object can accept.
 * @msglen: Maximum number of bytes the object should handle for all the chunks.
 * @tag: A tag identifying the kind of message.
 * @priority: The priority of this message.
 *
 * Allocate a new #prelude_msg_t object and store it into @ret. prelude_msg_set()
 * can then be used to add chunk of data to the message, and prelude_msg_write()
 * to send it.
 *
 * Returns: 0 on success, a negative value if an error occured.
 */
int prelude_msg_new(prelude_msg_t **ret, size_t msgcount, size_t msglen, uint8_t tag, prelude_msg_priority_t priority)
{
        size_t len;
        prelude_msg_t *msg;

        len = msglen;

        /*
         * 5 bytes of header by chunks :
         * - 1 byte:  tag
         * - 4 bytes: len
         */
        len += msgcount * 5;

        /*
         * For alert header.
         */
        len += PRELUDE_MSG_HDR_SIZE;

        msg = malloc(sizeof(prelude_msg_t) + len);
        if ( ! msg )
                return prelude_error_from_errno(errno);

        msg->payload = (unsigned char *) msg + sizeof(prelude_msg_t);

        msg->refcount = 1;
        msg->header_index = 0;
        msg->hdr.version = PRELUDE_MSG_VERSION;
        msg->hdr.tag = tag;
        msg->hdr.priority = priority;
        msg->hdr.is_fragment = 0;
        msg->hdr.datalen = len;
        msg->read_index = 0;
        msg->write_index = PRELUDE_MSG_HDR_SIZE;
        msg->fd_write_index = 0;
        msg->flush_msg_cb = NULL;

        *ret = msg;

        return 0;
}



/**
 * prelude_msg_dynamic_new:
 * @ret: Pointer where to store the create #prelude_msg_t.
 * @flush_msg_cb: Callback function to call when the buffer need to be flushed.
 * @data: Data to pass to the @flush_msg_cb callback function.
 *
 * Allocate a new #prelude_msg_t object. prelude_msg_set() can then be used to
 * add chunk of data to the message, and prelude_msg_mark_start() to separate
 * different message in the same buffer.
 *
 * This function use memory chunk of static size to store the message in. If
 * the size of the data you want to store is bigger than the actual chunk size,
 * @flush_msg_cb callback will be called for the current message to be flushed,
 * and the returned message will be used in order to store remaining data.
 *
 * Returns: 0 on success, a negative value if an error occured.
 */
int prelude_msg_dynamic_new(prelude_msg_t **ret, int (*flush_msg_cb)(prelude_msg_t **msg, void *data), void *data)
{
        prelude_msg_t *msg;

        msg = malloc(sizeof(prelude_msg_t) + MSGBUF_SIZE);
        if ( ! msg )
                return prelude_error_from_errno(errno);

        msg->refcount = 1;
        msg->hdr.tag = 0;
        msg->hdr.priority = 0;
        msg->hdr.is_fragment = 0;
        msg->hdr.version = PRELUDE_MSG_VERSION;
        msg->hdr.datalen = MSGBUF_SIZE;

        msg->payload = (unsigned char *) msg + sizeof(prelude_msg_t);

        msg->header_index = 0;
        msg->send_msg_data = data;
        msg->read_index = 0;
        msg->flush_msg_cb = flush_msg_cb;
        msg->write_index = PRELUDE_MSG_HDR_SIZE;
        msg->fd_write_index = 0;

        *ret = msg;

        return 0;
}


/**
 * prelude_msg_set_tag:
 * @msg: Pointer on a #prelude_msg_t object.
 * @tag: Tag to associate with @msg.
 *
 * Tag @msg.
 */
void prelude_msg_set_tag(prelude_msg_t *msg, uint8_t tag)
{
        msg->hdr.tag = tag;
}



/**
 * prelude_msg_set_priority:
 * @msg: Pointer on a #prelude_msg_t object.
 * @priority: Priority to associate with @msg.
 *
 * Associate @priority with @msg.
 */
void prelude_msg_set_priority(prelude_msg_t *msg, prelude_msg_priority_t priority)
{
        msg->hdr.priority = priority;
}




/**
 * prelude_msg_get_tag:
 * @msg: Pointer on a #prelude_msg_t object.
 *
 * prelude_msg_get_tag() return the tag contained in the @msg header.
 *
 * Returns: A tag.
 */
uint8_t prelude_msg_get_tag(prelude_msg_t *msg)
{
        return msg->hdr.tag;
}



/**
 * prelude_msg_get_priority:
 * @msg: Pointer on a #prelude_msg_t object.
 *
 * prelude_msg_get_priority() return the priority contained in the @msg header.
 *
 * Returns: A priority.
 */
prelude_msg_priority_t prelude_msg_get_priority(prelude_msg_t *msg)
{
        return msg->hdr.priority;
}




/**
 * prelude_msg_get_datalen:
 * @msg: Pointer on a #prelude_msg_t object.
 *
 * prelude_msg_get_datalen() return the len of the whole message
 * contained in the @msg header.
 *
 * Returns: Len of the message.
 */
uint32_t prelude_msg_get_datalen(prelude_msg_t *msg)
{
        return msg->hdr.datalen;
}



/**
 * prelude_msg_get_len:
 * @msg: Pointer on a #prelude_msg_t object.
 *
 * prelude_msg_get_len() return the currently used
 * len for the @msg message.
 *
 * Returns: Len of the message.
 */
uint32_t prelude_msg_get_len(prelude_msg_t *msg)
{
        return msg->write_index;
}




/**
 * prelude_msg_destroy:
 * @msg: Pointer on a #prelude_msg_t object.
 *
 * prelude_msg_destroy() destroy the #prelude_msg_t object pointed
 * to by @msg. All the ressources for this message are freed.
 */
void prelude_msg_destroy(prelude_msg_t *msg)
{
        if ( --msg->refcount == 0 )
                free(msg);
}




/**
 * prelude_msg_set_callback:
 * @msg: Pointer on a #prelude_msg_t object.
 * @flush_msg_cb: Pointer on a function responssible of sending the message.
 *
 * prelude_msg_set_callback() allow to change the callback used
 * to flush a message created with prelude_msg_dynamic_new().
 */
void prelude_msg_set_callback(prelude_msg_t *msg, int (*flush_msg_cb)(prelude_msg_t **msg, void *data))
{
        msg->flush_msg_cb = flush_msg_cb;
}



/**
 * prelude_msg_set_data:
 * @msg: Pointer on a #prelude_msg_t object.
 * @data: Pointer on the data to associate to this message.
 *
 * prelude_msg_set_data() allow to change the data passed
 * to the message sending callback.
 */
void prelude_msg_set_data(prelude_msg_t *msg, void *data)
{
        msg->send_msg_data = data;
}



/**
 * prelude_msg_is_fragment:
 * @msg: Pointer on a #prelude_msg_t object.
 *
 * Returns: true if @msg only contain a fragment of message.
 */
int prelude_msg_is_fragment(prelude_msg_t *msg)
{
        return msg->hdr.is_fragment ? 1 : 0;
}



/**
 * prelude_msg_is_empty:
 * @msg: Pointer on a #prelude_msg_t object.
 *
 * Returns: true if @msg doesn't contain any data to send.
 */
int prelude_msg_is_empty(prelude_msg_t *msg)
{
        return (msg->write_index - msg->header_index - PRELUDE_MSG_HDR_SIZE <= 0) ? 1 : 0;
}



struct timeval *prelude_msg_get_time(prelude_msg_t *msg, struct timeval *tv)
{
        tv->tv_sec = msg->hdr.tv_sec;
        tv->tv_usec = msg->hdr.tv_usec;

        return tv;
}



prelude_msg_t *prelude_msg_ref(prelude_msg_t *msg)
{
        msg->refcount++;
        return msg;
}



const unsigned char *prelude_msg_get_message_data(prelude_msg_t *msg)
{
        /*
         * if the message header index is 0 (write called, without
         * prelude_msg_mark_end() first), mark end of the message
         * cause the caller didn't do it in this case.
         */
        if ( msg->header_index == 0 )
                write_message_header(msg);

        return msg->payload + msg->fd_write_index;
}

