/*****
*
* Copyright (C) 2001, 2002, 2003 Yoann Vandoorselaere <yoann@prelude-ids.org>
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

#include "config.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/poll.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <sys/un.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <signal.h>
#include <gnutls/gnutls.h>

#define PRELUDE_ERROR_SOURCE_DEFAULT PRELUDE_ERROR_SOURCE_CONNECTION
#include "prelude-error.h"

#include "common.h"
#include "libmissing.h"
#include "prelude-inttypes.h"
#include "prelude-client.h"
#include "prelude-log.h"
#include "prelude-io.h"
#include "prelude-inet.h"
#include "prelude-msg.h"
#include "prelude-message-id.h"
#include "prelude-client.h"
#include "prelude-option.h"
#include "client-ident.h"
#include "prelude-list.h"
#include "prelude-linked-object.h"

#include "tls-auth.h"


/*
 * Path to the Prelude Unix socket.
 */
#define UNIX_SOCKET "/tmp/.prelude-unix"



struct prelude_connection {
        PRELUDE_LINKED_OBJECT;
        
        char *saddr;
        uint16_t sport;

        char *daddr;
        uint16_t dport;
        
        socklen_t sa_len;
        struct sockaddr *sa;
        
        /*
         * This pointer point to the object suitable
         * for writing to the Manager.
         */
        prelude_io_t *fd;

        uint8_t state;
        uint64_t peer_analyzerid;

        void *data;
};



static int declare_ident(prelude_io_t *fd, uint64_t analyzerid) 
{
        int ret;
        uint64_t nident;
        prelude_msg_t *msg;
        
        ret = prelude_msg_new(&msg, 1, sizeof(uint64_t), PRELUDE_MSG_ID, 0);
        if ( ret < 0 )
                return ret;

        nident = prelude_hton64(analyzerid);
        
        /*
         * send message
         */
        prelude_msg_set(msg, PRELUDE_MSG_ID_DECLARE, sizeof(uint64_t), &nident);
        ret = prelude_msg_write(msg, fd);
        prelude_msg_destroy(msg);
        
        return ret;
}



static int connection_write_msgbuf(prelude_msgbuf_t *msgbuf, prelude_msg_t *msg) 
{        
        return prelude_connection_send(prelude_msgbuf_get_data(msgbuf), msg);
}



static void auth_error(prelude_connection_t *cnx, prelude_client_profile_t *cp) 
{        
        log(LOG_INFO, "\nTLS authentication failed. Please run :\n"
            "prelude-adduser register %s %s --uid %d --gid %d\n"
            "program on the sensor host to create an account for this sensor.\n\n",
            prelude_client_profile_get_name(cp), cnx->daddr,
            prelude_client_profile_get_uid(cp), prelude_client_profile_get_gid(cp));
}




/*
 * Check if the tcp connection has been closed by peer
 * i.e if peer has sent a FIN tcp segment.
 *
 * It is important to call this function before writing on
 * a tcp socket, otherwise the write will succeed despite
 * the remote socket has been closed and next write will lead
 * to a broken pipe
 */
static int is_tcp_connection_still_established(prelude_io_t *pio)
{
        int pending, ret;
	struct pollfd pfd;
        
	pfd.events = POLLIN;
	pfd.fd = prelude_io_get_fd(pio);

	ret = poll(&pfd, 1, 0);
	if ( ret < 0 )
                return prelude_error_from_errno(errno);

	if ( ret == 0 )
		return 0;

	if ( pfd.revents & POLLERR || pfd.revents & POLLHUP )
                return prelude_error_from_errno(EPIPE);

	if ( ! (pfd.revents & POLLIN) )
		return 0;

	/*
	 * Get the number of bytes to read
	 */
        pending = prelude_io_pending(pio);        
	if ( pending <= 0 )
		return prelude_error_from_errno(EPIPE);

	return 0;
}




/*
 * Connect to the specified address in a generic manner
 * (can be Unix or Inet ).
 */
static int generic_connect(struct sockaddr *sa, socklen_t sa_len)
{
        int ret, sock, proto;

        if ( sa->sa_family == AF_UNIX ) 
                proto = 0;
        else
                proto = IPPROTO_TCP;
        
        sock = socket(sa->sa_family, SOCK_STREAM, proto);
	if ( sock < 0 ) 
		return prelude_error_from_errno(errno);
        
        ret = fcntl(sock, F_SETOWN, getpid());
        if ( ret < 0 ) {
                close(sock);
                return prelude_error_from_errno(errno);
        }

        ret = connect(sock, sa, sa_len);
	if ( ret < 0 ) {
                close(sock);
                return prelude_error_from_errno(errno);
	}
        
	return sock;
}





static int handle_authentication(prelude_connection_t *cnx, prelude_client_profile_t *cp, int crypt)
{
        int ret;
        
        ret = tls_auth_connection(cp, cnx->fd, crypt, &cnx->peer_analyzerid);
        if ( ret < 0 ) {
                /*
                 * SSL authentication failed,
                 * tell the user how to setup SSL auth and exit.
                 */
                log(LOG_INFO, "- TLS authentication failed with Prelude Manager.\n");
                auth_error(cnx, cp);
                return -1;
        }

        log(LOG_INFO, "- TLS authentication succeed with Prelude Manager.\n");

        return 0;
}





static int start_inet_connection(prelude_connection_t *cnx, prelude_client_profile_t *profile) 
{
        socklen_t len;
        int sock, ret = -1;
        struct sockaddr_in addr;
        
        sock = generic_connect(cnx->sa, cnx->sa_len);
        if ( sock < 0 )
                return sock;

        /*
         * Get information about the connection,
         * because the sensor might want to know source addr/port used.
         */        
        len = sizeof(addr);

        ret = getsockname(sock, (struct sockaddr *) &addr, &len);
        if ( ret < 0 ) 
                log(LOG_ERR, "couldn't get connection informations.\n");
        else {
                cnx->saddr = strdup(inet_ntoa(addr.sin_addr));
                cnx->sport = ntohs(addr.sin_port);
        }
        
        prelude_io_set_sys_io(cnx->fd, sock);
        
        ret = handle_authentication(cnx, profile, 1);
        if ( ret < 0 )
                close(sock);
        
        return ret;
}




static int start_unix_connection(prelude_connection_t *cnx, prelude_client_profile_t *profile) 
{
        int ret, sock;
        
        sock = generic_connect(cnx->sa, cnx->sa_len);
        if ( sock < 0 )
                return sock;
        
        prelude_io_set_sys_io(cnx->fd, sock);
        
        ret = handle_authentication(cnx, profile, 0);
        if ( ret < 0 )
                close(sock);
                
        return ret;
}




static int do_connect(prelude_connection_t *cnx, prelude_client_profile_t *profile) 
{
        int ret;
        
        if ( cnx->sa->sa_family == AF_UNIX ) {
                log(LOG_INFO, "- Connecting to %s (UNIX) prelude Manager server.\n",
                    ((struct sockaddr_un *) cnx->sa)->sun_path);
		ret = start_unix_connection(cnx, profile);
        } else {
                log(LOG_INFO, "- Connecting to %s port %d prelude Manager server.\n",
                    cnx->daddr, cnx->dport);
                ret = start_inet_connection(cnx, profile);
        }
        
        return ret;
}




static void close_connection_fd(prelude_connection_t *cnx) 
{        
        if ( ! (cnx->state & PRELUDE_CONNECTION_ESTABLISHED) )
                return;

        if ( cnx->saddr )
                free(cnx->saddr);
        
        cnx->state &= ~PRELUDE_CONNECTION_ESTABLISHED;
        prelude_io_close(cnx->fd);
}



static void destroy_connection_fd(prelude_connection_t *cnx)
{
        close_connection_fd(cnx);
        
        if ( cnx->state & PRELUDE_CONNECTION_OWN_FD )
                prelude_io_destroy(cnx->fd);
}



static int resolve_addr(prelude_connection_t *cnx, const char *addr, uint16_t port) 
{
        int ret;
        prelude_addrinfo_t *ai, hints;
        char service[sizeof("00000")];

        memset(&hints, 0, sizeof(hints));
        
        hints.ai_family = PF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_protocol = IPPROTO_TCP;

        snprintf(service, sizeof(service), "%u", port);
        
        ret = prelude_inet_getaddrinfo(addr, service, &hints, &ai);        
        if ( ret < 0 )
                return prelude_error_from_errno(ret);
        
        ret = prelude_inet_addr_is_loopback(ai->ai_family, prelude_inet_sockaddr_get_inaddr(ai->ai_addr));
        if ( ret == 0 ) {                
                ai->ai_family = AF_UNIX;
                ai->ai_addrlen = sizeof(struct sockaddr_un);
        }

        cnx->sa = malloc(ai->ai_addrlen);
        if ( ! cnx->sa ) {
                prelude_inet_freeaddrinfo(ai);
                return prelude_error_from_errno(errno);
        }

        cnx->sa_len = ai->ai_addrlen;
        cnx->sa->sa_family = ai->ai_family;

        if ( ai->ai_family != AF_UNIX )
                memcpy(cnx->sa, ai->ai_addr, ai->ai_addrlen);
        else {
                struct sockaddr_un *un = (struct sockaddr_un *) cnx->sa;
                prelude_connection_get_socket_filename(un->sun_path, sizeof(un->sun_path), port);
        }
                
        prelude_inet_freeaddrinfo(ai);

        return 0;
}



/*
 * function called back when a connection timer expire.
 */

void prelude_connection_destroy(prelude_connection_t *cnx) 
{        
	free(cnx->sa);
        
        destroy_connection_fd(cnx);
        
        if ( cnx->saddr )
                free(cnx->saddr);
        
        free(cnx->daddr);
        free(cnx);
}




int prelude_connection_new(prelude_connection_t **out, const char *addr, uint16_t port) 
{
        int ret;
        prelude_connection_t *new;
        
        signal(SIGPIPE, SIG_IGN);

        new = malloc(sizeof(*new));
        if ( ! new )
                return prelude_error_from_errno(errno);
        
        ret = prelude_io_new(&new->fd);
        if ( ret < 0 ) {
                free(new);
                return ret;
        }

        if ( strcmp(addr, "unix") != 0 ) {
                ret = resolve_addr(new, addr, port);                
                if ( ret < 0 ) {
                        prelude_io_destroy(new->fd);
                        free(new);
                        return ret;
                }
        }
        
        new->saddr = NULL;
        new->sport = 0;
        new->daddr = strdup(addr);
        new->dport = port;
        new->state = PRELUDE_CONNECTION_OWN_FD;

        *out = new;
        
        return 0;
}




void prelude_connection_set_fd(prelude_connection_t *cnx, prelude_io_t *fd) 
{
        destroy_connection_fd(cnx);        
        cnx->fd = fd;

        /*
         * The caller is responssible for fd closing/destruction.
         * Unless it specify otherwise using prelude_connection_set_state().
         */
        cnx->state &= ~PRELUDE_CONNECTION_OWN_FD;
}




int prelude_connection_connect(prelude_connection_t *conn,
                               prelude_client_profile_t *profile,
                               prelude_connection_capability_t capability)
{
        int ret;
        prelude_msg_t *msg;
        
        close_connection_fd(conn);
                
        ret = do_connect(conn, profile);
        if ( ret < 0 ) 
                return ret;
                
        ret = declare_ident(conn->fd, prelude_client_profile_get_analyzerid(profile));
        if ( ret < 0 ) 
                goto err;
                
        ret = prelude_msg_new(&msg, 1, sizeof(uint8_t), PRELUDE_MSG_CONNECTION_CAPABILITY, 0);
        if ( ret < 0 )
                goto err;
        
        prelude_msg_set(msg, capability, 0, NULL);
        ret = prelude_msg_write(msg, conn->fd);
        prelude_msg_destroy(msg);

        if ( ret < 0 )
                goto err;

        conn->state |= PRELUDE_CONNECTION_ESTABLISHED;
        
        return ret;
        
 err:
        prelude_io_close(conn->fd);
        
        return ret;
}




void prelude_connection_close(prelude_connection_t *cnx) 
{
        close_connection_fd(cnx);
}




int prelude_connection_send(prelude_connection_t *cnx, prelude_msg_t *msg) 
{
        ssize_t ret;
        
        if ( ! (cnx->state & PRELUDE_CONNECTION_ESTABLISHED) )
                return -1;

        ret = prelude_msg_write(msg, cnx->fd);
        if ( ret < 0 ) {
                close_connection_fd(cnx);
                return ret;
        }

        ret = is_tcp_connection_still_established(cnx->fd);
        if ( ret < 0 ) {
                close_connection_fd(cnx);
                return ret;
        }
        
        return ret;
}



int prelude_connection_recv(prelude_connection_t *cnx, prelude_msg_t **msg)
{
        int ret;
        
        if ( ! (cnx->state & PRELUDE_CONNECTION_ESTABLISHED) )
                return -1;

        ret = prelude_msg_read(msg, cnx->fd);
        if ( ret < 0 ) {
                close_connection_fd(cnx);
                return ret;
        }

        ret = is_tcp_connection_still_established(cnx->fd);
        if ( ret < 0 ) {
                close_connection_fd(cnx);
                return ret;
        }

        return ret;
}




/**
 * prelude_connection_get_fd:
 * @cnx: Pointer to a #prelude_connection_t object.
 *
 * Returns: A pointer to the #prelude_io_t object used for
 * communicating with the peer.
 */
prelude_io_t *prelude_connection_get_fd(prelude_connection_t *cnx) 
{
        return cnx->fd;
}




/**
 * prelude_connection_get_saddr:
 * @cnx: Pointer to a #prelude_connection_t object.
 *
 * Returns: the source address used to connect.
 */
const char *prelude_connection_get_saddr(prelude_connection_t *cnx) 
{
        return cnx->saddr;
}



/**
 * prelude_connection_get_daddr:
 * @cnx: Pointer to a #prelude_connection_t object.
 *
 * Returns: the destination address used to connect.
 */
const char *prelude_connection_get_daddr(prelude_connection_t *cnx) 
{
        return cnx->daddr;
}



/**
 * prelude_connection_get_sport:
 * @cnx: Pointer to a #prelude_connection_t object.
 *
 * Returns: the source port used to connect.
 */
uint16_t prelude_connection_get_sport(prelude_connection_t *cnx) 
{
        return cnx->sport;
}



/**
 * prelude_connection_get_sport:
 * @cnx: Pointer to a #prelude_connection_t object.
 *
 * Returns: the destination port used to connect.
 */
uint16_t prelude_connection_get_dport(prelude_connection_t *cnx) 
{
        return cnx->dport;
}



/**
 * prelude_connection_is_alive:
 * @cnx: Pointer to a #prelude_connection_t object.
 *
 * Returns: 0 if the connection associated with @cnx is alive, -1 otherwise.
 */
int prelude_connection_is_alive(prelude_connection_t *cnx) 
{
        return (cnx->state & PRELUDE_CONNECTION_ESTABLISHED) ? 0 : -1;
}



void prelude_connection_set_state(prelude_connection_t *cnx, int state) 
{
        cnx->state = state;
}




int prelude_connection_get_state(prelude_connection_t *cnx)
{
        return cnx->state;
}




ssize_t prelude_connection_forward(prelude_connection_t *cnx, prelude_io_t *src, size_t count) 
{
        ssize_t ret;

        if ( ! (cnx->state & PRELUDE_CONNECTION_ESTABLISHED) )
                return -1;
        
        ret = prelude_io_forward(cnx->fd, src, count);
        if ( ret < 0 ) {
                log(LOG_ERR, "error forwarding message to Manager.\n");
                close_connection_fd(cnx);
        }

        return ret;
}



void prelude_connection_get_socket_filename(char *buf, size_t size, uint16_t port) 
{
        snprintf(buf, size, "%s-%u", UNIX_SOCKET, port);
}



uint64_t prelude_connection_get_peer_analyzerid(prelude_connection_t *cnx)
{
        return cnx->peer_analyzerid;
}


void prelude_connection_set_peer_analyzerid(prelude_connection_t *cnx, uint64_t analyzerid)
{
        cnx->peer_analyzerid = analyzerid;
}



int prelude_connection_new_msgbuf(prelude_connection_t *connection, prelude_msgbuf_t **msgbuf)
{
        int ret;
        
        ret = prelude_msgbuf_new(msgbuf);
        if ( ret < 0 )
                return -1;

        prelude_msgbuf_set_data(*msgbuf, connection);
        prelude_msgbuf_set_callback(*msgbuf, connection_write_msgbuf);
        
        return 0;
}



void prelude_connection_set_data(prelude_connection_t *conn, void *data)
{
        conn->data = data;
}



void *prelude_connection_get_data(prelude_connection_t *conn)
{
        return conn->data;
}
