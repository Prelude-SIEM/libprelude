/*
 *  Copyright (C) 2000 Yoann Vandoorselaere.
 *
 *  This program is free software; you can redistribute it and/or modify 
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *  Authors: Yoann Vandoorselaere <yoann@mandrakesoft.com>
 *
 */
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <sys/poll.h>
#include <netinet/in.h>

#include "common.h"
#include "socket-op.h"



static const char *get_event(struct pollfd *pfd) 
{
        if ( pfd->revents & POLLERR )
                return "error";

        else if ( pfd->revents & POLLHUP )
                return "hung up";

        else if ( pfd->revents & POLLNVAL )
                return "invalid";

        return NULL;
}


/*
 * Return 1 when data availlable.
 * Return 0 on timeout.
 * Return -1 on error.
 */
static int wait_data(struct pollfd *pfd, short events, int timeout) 
{
        int ret;

        pfd->events = events;
        
        while ( 1 ) {
                ret = poll(pfd, 1, timeout);                
                if ( ret < 0 ) {
                        if ( errno == EINTR )
                                continue;
                        return ret;
                }

                else if ( ret == 0 )
                        return ret;

                if ( ! (pfd->revents & pfd->events)  ) {
                        log(LOG_ERR, "poll : An invalid event (%s) occured.\n", get_event(pfd));
                        return -1;
                }

                break;
        }
        return 1;
}



static int do_socket_read(struct pollfd *pfd, int fd, void *buf,
                          size_t count, read_func_t *myread) 
{
        int ret, len = 0;
        
        do {
                ret = wait_data(pfd, POLLIN, 3000);
                if ( ret <= 0 )
                        return ret;
                
                ret = myread(fd, (unsigned char *)buf + len, count - len);
                if ( ret < 0 ) {
                        if ( errno == EINTR )
                                continue;

                        return ret;
                }

                else if ( ret == 0 )
                        return ret;
                
                len += ret;
                                        
        } while ( len != count );

        return len;
}




/*
 * socket_read:
 * @fd: The file descriptor to read from.
 * @buf: The buffer where data should be stored.
 * @myread: A pointer on the real "read" function to use (SSL_read/read).
 *
 * This function will wait for data to be availlable, then
 * try to read as many bytes as specified.
 * If there is not that much bytes to read after the first block of data
 * became availlable, it will timeout after 3 second waiting for data.
 * 
 * Returns: The number of bytes read (which will always be count),
 * 0 on timeout (waiting for data), -1 on error.
 */
ssize_t socket_read(int fd, void *buf, size_t count, read_func_t *myread) 
{
        int ret;
        struct pollfd pfd;

        pfd.fd = fd;
        pfd.revents = 0;
        
        ret = wait_data(&pfd, POLLIN, -1);
        if ( ret <= 0 ) 
                return ret;

        return do_socket_read(&pfd, fd, buf, count, myread);
}





/*
 * socket_read:
 * @fd: The file descriptor to read from.
 * @buf: The buffer where data should be stored.
 * @myread: A pointer on the real "read" function to use (SSL_read/read).
 *
 * This function will try to read as many bytes as specified.
 * If there is not that much bytes to read it will timeout after 3 second
 * waiting for data.
 * 
 * Returns: The number of bytes read (which will always be count),
 * 0 on timeout (waiting for data), -1 on error.
 */
ssize_t socket_read_nowait(int fd, void *buf, size_t count, read_func_t *myread) 
{
        struct pollfd pfd;

        pfd.fd = fd;
        pfd.revents = 0;
        
        return do_socket_read(&pfd, fd, buf, count, myread);
}





/*
 * socket_write:
 * @fd: The file descriptor to write to.
 * @buf: The buffer where data should be writen from.
 * @myread: A pointer on the real "write" function to use (SSL_write/write).
 *
 * This function will try to write as many bytes as specified.
 * 
 * Returns: The number of bytes written (which will always be count),
 * 0 on timeout (waiting for possible IO), -1 on error.
 */
ssize_t socket_write(int fd, void *buf, size_t count, write_func_t *mywrite) 
{
        int ret, len = 0;
        
        do {                
                ret = mywrite(fd, (unsigned char *)buf + len, count - len);
                if ( ret < 0 ) {
                        if ( errno == EINTR )
                                continue;

                        return ret;
                }

                else if ( ret == 0 )
                        return ret;

                len += ret;

        } while ( len != count );

        return len;
}



/*
 * socket_read_delimited:
 * @fd: The file descriptor to write to.
 * @buf: A pointer on the address of the buffer where data should be written to.
 * @myread: read function to use.
 *
 * Used in conjunction with socket_write_delimited(), this
 * function permit to read the exact number of bytes written
 * by the socket_write_delimited() function call. The buffer is dynamically
 * allocated.
 * 
 * Returns: The number of bytes read,
 * 0 on timeout (3 seconds), -1 on error.
 */
ssize_t socket_read_delimited(int fd, void **buf, read_func_t *myread) 
{
        int ret, len;

        ret = socket_read_nowait(fd, &len, sizeof(int), read);
        if ( ret < 0 ) {
                log(LOG_ERR, "couldn't read next packet size.\n");
                return -1;
        }

        if ( ret == 0 ) {
                log(LOG_ERR, "couldn't read next packet size (timeout).\n");
                return -1;    
        }
        
        len = ntohl(len);

        *buf = malloc(len);
        if ( ! *buf ) {
                log(LOG_ERR, "couldn't allocate %d bytes.\n", len);
                return -1;
        }
        
        ret = socket_read_nowait(fd, *buf, len, myread);
        if ( ret < 0 ) {
                log(LOG_ERR, "couldn't read packet content.\n");
                return -1;
        }

        if ( ret == 0 ) {
                log(LOG_ERR, "couldn't read packet content (timeout).\n");
                return -1;
        }
        
        return len;
}



/*
 * socket_write_delimited:
 * @fd: The file descriptor to write to.
 * @buf: The buffer where data should be written to.
 * @count: Size of the buffer.
 * @mywrite: write function to use.
 *
 * This function write the size of the data that will be written
 * on the socket in a portable way before writting the data itself.
 * The other end can then read the exact ammount of data written
 * using the socket_read_delimited() function call.
 * 
 * Returns: The number of bytes written,
 * 0 on timeout (3 seconds), -1 on error.
 */
ssize_t socket_write_delimited(int sock, char *buf, size_t count, write_func_t *mywrite) 
{
	int ret, len;
        
        len = htonl(count);
        
        ret = socket_write(sock, &len, sizeof(int), mywrite);
        if ( ret < 0 ) {
                log(LOG_ERR, "couldn't write %d bytes.\n", sizeof(int));
                return -1;
        }

        if ( ret == 0 ) {
                log(LOG_ERR, "couldn't write %d bytes (timeout).\n", sizeof(int));
                return -1;
        }

        ret = socket_write(sock, buf, count, mywrite);
        if ( ret < 0 ) {
                log(LOG_ERR, "couldn't write %d bytes.\n", count);
                return -1;
        }

        if ( ret == 0 ) {
                 log(LOG_ERR, "couldn't write %d bytes (timeout).\n", count);
                 return -1;
        }

        return count;
}






















