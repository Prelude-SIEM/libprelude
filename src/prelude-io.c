/*****
*
* Copyright (C) 2001-2004 PreludeIDS Technologies. All Rights Reserved.
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
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/poll.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <termios.h>
#include <gnutls/gnutls.h>

#include "prelude-inttypes.h"

#ifdef HAVE_SYS_FILIO_H
 #include <sys/filio.h>
#endif


#include "prelude-log.h"
#include "prelude-io.h"
#include "common.h"


#define PRELUDE_ERROR_SOURCE_DEFAULT PRELUDE_ERROR_SOURCE_IO
#include "prelude-error.h"


#define CHUNK_SIZE 1024

#define MIN(x, y) ((x) < (y) ? (x) : (y))


struct prelude_io {

        int fd;
        void *fd_ptr;
        
        size_t size;
        size_t rindex;
        
        int (*close)(prelude_io_t *pio);
        ssize_t (*read)(prelude_io_t *pio, void *buf, size_t count);
        ssize_t (*write)(prelude_io_t *pio, const void *buf, size_t count);
        ssize_t (*pending)(prelude_io_t *pio);
};



/*
 * Buffer IO functions.
 */
static ssize_t buffer_read(prelude_io_t *pio, void *buf, size_t count)
{
        if ( pio->size - pio->rindex == 0 )
                return 0;

        count = MIN(count, pio->size - pio->rindex);

        memcpy(buf, (unsigned char *) pio->fd_ptr + pio->rindex, count);
        pio->rindex += count;
        
        return count;
}



static ssize_t buffer_write(prelude_io_t *pio, const void *buf, size_t count)
{
        unsigned char *new;
        
        if ( pio->size + count <= pio->size )
                return -1;
        
        new = prelude_realloc(pio->fd_ptr, pio->size + count);
        if ( ! new )
                return prelude_error_from_errno(errno);
        
        memcpy(new + pio->size, buf, count);

        pio->fd_ptr = new;
        pio->size += count;
        
        return count;
}



static int buffer_close(prelude_io_t *pio)
{
        if ( pio->fd_ptr ) {
                free(pio->fd_ptr);
                pio->fd_ptr = NULL;
                pio->size = pio->rindex = 0;
        }

        return 0;
}



static ssize_t buffer_pending(prelude_io_t *pio)
{
        return pio->size - pio->rindex;
}




/*
 * System IO functions.
 */
static ssize_t sys_read(prelude_io_t *pio, void *buf, size_t count) 
{
        ssize_t ret;

        do {
                ret = read(pio->fd, buf, count);        
        } while ( ret < 0 && errno == EINTR );

        if ( ret == 0 )
                return prelude_error(PRELUDE_ERROR_EOF);
        
        if ( ret < 0 )
                return prelude_error_from_errno(errno);

        return ret;
}



static ssize_t sys_write(prelude_io_t *pio, const void *buf, size_t count) 
{
        ssize_t ret;
        
        do {
                ret = write(pio->fd, buf, count);
        } while ( ret < 0 && errno == EINTR );

        if ( ret < 0 )
                return prelude_error_from_errno(errno);
        
        return ret;
}



static int sys_close(prelude_io_t *pio) 
{
        int ret;

        do {
                ret = close(pio->fd);
        } while ( ret < 0 && errno == EINTR );

        return (ret >= 0) ? ret : prelude_error_from_errno(errno);
}



static ssize_t sys_pending(prelude_io_t *pio) 
{
        ssize_t ret;
        
        if ( ioctl(pio->fd, FIONREAD, &ret) < 0 )
                return prelude_error_from_errno(errno);

        return ret;
}




/*
 * Buffered IO functions.
 */
static ssize_t file_read(prelude_io_t *pio, void *buf, size_t count) 
{
        size_t ret;
        
        ret = fread(buf, count, 1, pio->fd_ptr);
        if ( ret <= 0 )
                return ret;

        /*
         * fread return the number of *item* read.
         */
        return count;
}



static ssize_t file_write(prelude_io_t *pio, const void *buf, size_t count) 
{
        size_t ret;
        
        ret = fwrite(buf, count, 1, pio->fd_ptr);
        if ( ret <= 0 )
                return ret;

        /*
         * fwrite return the number of *item* written.
         */
        return count;
}



static int file_close(prelude_io_t *pio) 
{
        return fclose(pio->fd_ptr);
}



static ssize_t file_pending(prelude_io_t *pio)
{
        return -1;
}



/*
 * TLS IO functions
 */
static int tls_check_error(prelude_io_t *pio, int ret)
{
        const char *alert;
        
        if ( ret == GNUTLS_E_INTERRUPTED )
                return 0;

        else if ( ret == GNUTLS_E_AGAIN )
                return prelude_error(PRELUDE_ERROR_EAGAIN);
        
        else if ( ret == GNUTLS_E_WARNING_ALERT_RECEIVED ) {
                alert = gnutls_alert_get_name(gnutls_alert_get(pio->fd_ptr));
                prelude_log(PRELUDE_LOG_WARN, "- TLS: received warning alert: %s.\n", alert);
                return 0;
        }
        
        else if ( ret == GNUTLS_E_FATAL_ALERT_RECEIVED ) {
                alert = gnutls_alert_get_name(gnutls_alert_get(pio->fd_ptr));
                prelude_log(PRELUDE_LOG_ERR, "- TLS: received fatal alert: %s.\n", alert);
        }

        return prelude_error(PRELUDE_ERROR_TLS);
}



static ssize_t tls_read(prelude_io_t *pio, void *buf, size_t count) 
{
        ssize_t ret;
        
        do {
                ret = gnutls_record_recv(pio->fd_ptr, buf, count);

        } while ( ret < 0 && (ret = tls_check_error(pio, ret)) == 0 );

        if ( ret == 0 )
                return prelude_error(PRELUDE_ERROR_EOF);
        
        return ret;
}



static ssize_t tls_write(prelude_io_t *pio, const void *buf, size_t count) 
{
        ssize_t ret;

        do {        
                ret = gnutls_record_send(pio->fd_ptr, buf, count);
                                
        } while ( ret < 0 && (ret = tls_check_error(pio, ret)) == 0 );
        
        return ret;
}



static int tls_close(prelude_io_t *pio) 
{
        int ret;

        do {
                ret = gnutls_bye(pio->fd_ptr, GNUTLS_SHUT_RDWR);
        } while ( ret < 0 && (ret = tls_check_error(pio, ret)) == 0 );

        if ( ret < 0 && prelude_error_get_code(ret) == PRELUDE_ERROR_EAGAIN )
                return ret;
        
        gnutls_deinit(pio->fd_ptr);
        
        return sys_close(pio);
}



static ssize_t tls_pending(prelude_io_t *pio)
{
        ssize_t ret;
        
        ret = gnutls_record_check_pending(pio->fd_ptr);
        if ( ret > 0 )
                return ret;
        
        ret = sys_pending(pio);
        if ( ret > 0 )
                return ret;
        
        return 0;
}



/*
 * Forward data from one fd to another using copy.
 */
static ssize_t copy_forward(prelude_io_t *dst, prelude_io_t *src, size_t count) 
{
        int ret;
        size_t scount;
        unsigned char buf[8192];

        scount = count;
        
        while ( count ) {
                
                ret = (count < sizeof(buf)) ? count : sizeof(buf);
                
                ret = prelude_io_read(src, buf, ret);
                if ( ret <= 0 ) 
                        return -1;

                count -= ret;
                
                ret = prelude_io_write(dst, buf, ret);
                if ( ret < 0 ) 
                        return -1;
        }
        
        return scount;
}





/**
 * prelude_io_forward:
 * @src: Pointer to a #prelude_io_t object.
 * @dst: Pointer to a #prelude_io_t object.
 * @count: Number of byte to forward from @src to @dst.
 *
 * prelude_io_forward() attempts to transfer up to @count bytes from
 * the file descriptor identified by @src into the file descriptor
 * identified by @dst.
 *
 * Returns: If the transfer was successful, the number of bytes written
 * to @dst is returned.  On error, -1 is returned, and errno is set appropriately.
 */
ssize_t prelude_io_forward(prelude_io_t *dst, prelude_io_t *src, size_t count) 
{
        return copy_forward(dst, src, count);
}




/**
 * prelude_io_read:
 * @pio: Pointer to a #prelude_io_t object.
 * @buf: Pointer to the buffer to store data into.
 * @count: Number of bytes to read.
 *
 * prelude_io_read() attempts to read up to @count bytes from the
 * file descriptor identified by @pio into the buffer starting at @buf.
 *
 * If @count is zero, prelude_io_read() returns zero and has no other
 * results. If @count is greater than SSIZE_MAX, the result is unspecified.
 *
 * The case where the read function would be interrupted by a signal is
 * handled internally. So you don't have to check for EINTR.
 *
 * Returns: On success, the number of bytes read is returned (zero
 * indicates end of file). It is not an error if this number is smaller
 * than the number of bytes requested; this may happen for example because
 * fewer bytes are actually available right now or because read() was
 * interrupted by a signal.
 *
 * On error, -1 is returned, and errno is set appropriately. In this
 * case it is left unspecified whether the file position (if any) changes.
 */
ssize_t prelude_io_read(prelude_io_t *pio, void *buf, size_t count) 
{
        return pio->read(pio, buf, count);
}




/**
 * prelude_io_read_wait:
 * @pio: Pointer to a #prelude_io_t object.
 * @buf: Pointer to the buffer to store data into.
 * @count: Number of bytes to read.
 *
 * prelude_io_read_wait() attempts to read up to @count bytes from the
 * file descriptor identified by @pio into the buffer starting at @buf.
 *
 * If @count is zero, prelude_io_read() returns zero and has no other
 * results. If @count is greater than SSIZE_MAX, the result is unspecified.
 *
 * The case where the read function would be interrupted by a signal is
 * handled internally. So you don't have to check for EINTR.
 *
 * prelude_io_read_wait() always return the number of bytes requested.
 * Be carefull that this function is blocking.
 *
 * Returns: On success, the number of bytes read is returned (zero
 * indicates end of file).
 *
 * On error, -1 is returned, and errno is set appropriately. In this
 * case it is left unspecified whether the file position (if any) changes.
 */
ssize_t prelude_io_read_wait(prelude_io_t *pio, void *buf, size_t count) 
{
        struct pollfd pfd;
        ssize_t n = 0, ret;
        unsigned char *in = buf;

        pfd.fd = prelude_io_get_fd(pio);
        pfd.events = POLLIN;
        
        do {
                ret = poll(&pfd, 1, -1);                
                if ( ret <= 0 )
                        return -1;

                if ( ! (pfd.revents & POLLIN) )
                     return -1;
                
                ret = prelude_io_read(pio, &in[n], count - n);
                if ( ret <= 0 )
                        return ret;

                n += ret;
                
        } while ( n != count );
        
        return n;
}


                       
/**
 * prelude_io_read_delimited:
 * @pio: Pointer to a #prelude_io_t object.
 * @buf: Pointer to the address of a buffer to store address of data into.
 *
 * prelude_io_read_delimited() read message written by prelude_write_delimited().
 * Theses messages are sents along with the len of the message.
 *
 * Uppon return the @buf argument is updated to point on a newly allocated
 * buffer containing the data read. The @count argument is set to the number of
 * bytes the message was containing.
 *
 * The case where the read function would be interrupted by a signal is
 * handled internally. So you don't have to check for EINTR.
 *
 * Returns: On success, the number of bytes read is returned (zero
 * indicates end of file). 
 *
 * On error, -1 is returned, and errno is set appropriately. In this
 * case it is left unspecified whether the file position (if any) changes.
 */
ssize_t prelude_io_read_delimited(prelude_io_t *pio, unsigned char **buf) 
{
        ssize_t ret;
        size_t count;
        uint16_t msglen;
        
        ret = prelude_io_read_wait(pio, &msglen, sizeof(msglen));
        if ( ret <= 0 ) {
                prelude_log(PRELUDE_LOG_ERR, "couldn't read len message of %d bytes.\n", sizeof(msglen));
                return ret;
        }

        count = ntohs(msglen);

        *buf = malloc(count);
        if ( ! *buf )
                return prelude_error_from_errno(errno);
        
        ret = prelude_io_read_wait(pio, *buf, count);
        if ( ret < 0 )
                return prelude_error_from_errno(errno);

        if ( ret == 0 )
                return prelude_error(PRELUDE_ERROR_EOF);
        
        return count;
}




/**
 * prelude_io_write:
 * @pio: Pointer to a #prelude_io_t object.
 * @buf: Pointer to the buffer to write data from.
 * @count: Number of bytes to write.
 *
 * prelude_io_write() writes up to @count bytes to the file descriptor
 * identified by @pio from the buffer starting at @buf. POSIX requires
 * that a read() which can be proved to occur after a write() has returned
 * returns the new data. Note that not all file systems are POSIX conforming.
 *
 * The case where the write() function would be interrupted by a signal is
 * handled internally. So you don't have to check for EINTR.
 *
 * Returns: On success, the number of bytes written are returned (zero
 * indicates nothing was written). On error, -1 is returned, and errno
 * is set appropriately. If @count is zero and the file descriptor refers
 * to a regular file, 0 will be returned without causing any other effect.
 * For a special file, the results are not portable.
 */
ssize_t prelude_io_write(prelude_io_t *pio, const void *buf, size_t count) 
{
        return pio->write(pio, buf, count);
}



/**
 * prelude_io_write_delimited:
 * @pio: Pointer to a #prelude_io_t object.
 * @buf: Pointer to the buffer to write data from.
 * @count: Number of bytes to write.
 *
 * prelude_io_write_delimited() writes up to @count bytes to the file descriptor
 * identified by @pio from the buffer starting at @buf. POSIX requires
 * that a read() which can be proved to occur after a write() has returned
 * returns the new data. Note that not all file systems are POSIX conforming.
 *
 * prelude_io_write_delimited() also write the len of the data to be sent.
 * which allow prelude_io_read_delimited() to safely know if it got all the
 * data a given write contain.
 *
 * The case where the write() function would be interrupted by a signal is
 * handled internally. So you don't have to check for EINTR.
 *
 * Returns: On success, the number of bytes written are returned (zero
 * indicates nothing was written). On error, -1 is returned, and errno
 * is set appropriately.
 */
ssize_t prelude_io_write_delimited(prelude_io_t *pio, const void *buf, uint16_t count) 
{
        int ret;
        uint16_t nlen;
        
        nlen = htons(count);
        
        ret = prelude_io_write(pio, &nlen, sizeof(nlen));
        if ( ret <= 0 ) 
                return -1;
        
        ret = prelude_io_write(pio, buf, count);
        if ( ret <= 0 )
                return -1;

        return count;
}




/**
 * prelude_io_close:
 * @pio: Pointer to a #prelude_io_t object.
 *
 * prelude_io_close() closes the file descriptor indentified by @pio,
 *
 * The case where the close() function would be interrupted by a signal is
 * handled internally. So you don't have to check for EINTR.
 *
 * Returns: zero on success, or -1 if an error occurred.
 */
int prelude_io_close(prelude_io_t *pio)
{
        return pio->close(pio);
}




/**
 * prelude_io_new:
 * @ret: Pointer where to store the created #prelude_io_t object.
 *
 * Create a new prelude IO object.
 *
 * Returns: 0 on success, or a negative value if an error occur.
 */
int prelude_io_new(prelude_io_t **ret) 
{
        *ret = malloc(sizeof(**ret));
        if ( ! *ret )
                return prelude_error_from_errno(errno);
        
        return 0;
}



/**
 * prelude_io_set_file_io:
 * @pio: A pointer on the #prelude_io_t object.
 * @fd: File descriptor identifying a file.
 *
 * Setup the @pio object to work with file I/O function.
 * The @pio object is then associated with @fd.
 */
void prelude_io_set_file_io(prelude_io_t *pio, FILE *fdptr) 
{
        pio->fd = fileno(fdptr);
        pio->fd_ptr = fdptr;
        pio->read = file_read;
        pio->write = file_write;
        pio->close = file_close;
        pio->pending = file_pending;
}




/**
 * prelude_io_set_tls_io:
 * @pio: A pointer on the #prelude_io_t object.
 * @tls: Pointer on the TLS structure holding the TLS connection data.
 *
 * Setup the @pio object to work with TLS based I/O function.
 * The @pio object is then associated with @tls.
 */
void prelude_io_set_tls_io(prelude_io_t *pio, void *tls) 
{
        gnutls_transport_ptr ptr;

        ptr = gnutls_transport_get_ptr(tls);
        pio->fd = (int) ptr;
        
        pio->fd_ptr = tls;
        pio->read = tls_read;
        pio->write = tls_write;
        pio->close = tls_close;
        pio->pending = tls_pending;
}




/**
 * prelude_io_set_sys_io:
 * @pio: A pointer on the #prelude_io_t object.
 * @fd: A file descriptor.
 *
 * Setup the @pio object to work with system based I/O function.
 * The @pio object is then associated with @fd.
 */
void prelude_io_set_sys_io(prelude_io_t *pio, int fd) 
{
        pio->fd = fd;
        pio->fd_ptr = NULL;
        pio->read = sys_read;
        pio->write = sys_write;
        pio->close = sys_close;
        pio->pending = sys_pending;
}



int prelude_io_set_buffer_io(prelude_io_t *pio)
{
        pio->fd_ptr = NULL;
        pio->size = pio->rindex = 0;
        
        pio->read = buffer_read;
        pio->write = buffer_write;
        pio->close = buffer_close;
        pio->pending = buffer_pending;
        
        return 0;
}



/**
 * prelude_io_get_fd:
 * @pio: A pointer on a #prelude_io_t object.
 *
 * Returns: The FD associated with this object.
 */
int prelude_io_get_fd(prelude_io_t *pio) 
{
        return pio->fd;
}



/**
 * prelude_io_get_fdptr:
 * @pio: A pointer on a #prelude_io_t object.
 *
 * Returns: Pointer associated with this object (file, tls, buffer, or NULL).
 */
void *prelude_io_get_fdptr(prelude_io_t *pio) 
{
        return pio->fd_ptr;
}



/**
 * prelude_io_destroy:
 * @pio: Pointer to a #prelude_io_t object.
 *
 * Destroy the @pio object.
 */
void prelude_io_destroy(prelude_io_t *pio) 
{
        free(pio);
}




/**
 * prelude_io_pending:
 * @pio: Pointer to a #prelude_io_t object.
 *
 * prelude_io_pending return the number of bytes waiting to
 * be read on an TLS or socket fd.
 *
 * Returns: Number of byte waiting to be read on @pio, or -1
 * if @pio is not of type TLS or socket. 
 */
ssize_t prelude_io_pending(prelude_io_t *pio)
{
        return pio->pending(pio);
}
