/*****
*
* Copyright (C) 2001,2002,2003,2004,2005 PreludeIDS Technologies. All Rights Reserved.
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
#include "prelude-inttypes.h"
#include "prelude-client.h"
#include "prelude-log.h"
#include "prelude-io.h"
#include "prelude-msg.h"
#include "prelude-message-id.h"
#include "prelude-client.h"
#include "prelude-option.h"
#include "prelude-list.h"
#include "prelude-linked-object.h"

#include "tls-auth.h"


#define PRELUDE_CONNECTION_OWN_FD 0x02


/*
 * Default port to connect to.
 */
#define DEFAULT_PORT 5554

/*
 * Path to the default Unix socket.
 */
#define UNIX_SOCKET "/tmp/.prelude-unix"



struct prelude_connection {
        PRELUDE_LINKED_OBJECT;
        
        char *saddr;
        unsigned int sport;

        char *daddr;
        unsigned int dport;
        
        socklen_t salen;
        struct sockaddr *sa;
        
        prelude_io_t *fd;
        uint64_t peer_analyzerid;
        prelude_connection_permission_t permission;
        
        void *data;

        prelude_connection_state_t state;
};



static int connection_write_msgbuf(prelude_msgbuf_t *msgbuf, prelude_msg_t *msg) 
{        
        return prelude_connection_send(prelude_msgbuf_get_data(msgbuf), msg);
}



static void auth_error(prelude_connection_t *cnx, prelude_connection_permission_t reqperms,
                       prelude_client_profile_t *cp) 
{
        char *tmp;
        prelude_string_t *out;

        prelude_string_new(&out);
        prelude_connection_permission_to_string(reqperms, out);
                
        tmp = strrchr(cnx->daddr, ':');
        if ( tmp )
                *tmp = '\0';
        
        prelude_log(PRELUDE_LOG_WARN, "\nTLS authentication failed. Please run :\n"
                    "prelude-adduser register %s \"%s\" %s --uid %d --gid %d\n"
                    "program on the sensor host to create an account for this sensor.\n\n",
                    prelude_client_profile_get_name(cp), prelude_string_get_string(out), cnx->daddr,
                    prelude_client_profile_get_uid(cp), prelude_client_profile_get_gid(cp));
        
        prelude_string_destroy(out);
        if ( tmp )
                *tmp = ':';
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
static int generic_connect(struct sockaddr *sa, socklen_t salen)
{
        int ret, sock, flags = FD_CLOEXEC;
        
        sock = socket(sa->sa_family, SOCK_STREAM, 0);
	if ( sock < 0 ) 
		return prelude_error_from_errno(errno);


        ret = fcntl(sock, F_SETFD, &flags);
        if ( ret < 0 ) {
                close(sock);
                return prelude_error_from_errno(errno);
        }
                
        ret = fcntl(sock, F_SETOWN, getpid());
        if ( ret < 0 ) {
                close(sock);
                return prelude_error_from_errno(errno);
        }

        ret = connect(sock, sa, salen);
	if ( ret < 0 ) {
                close(sock);
                return prelude_error_from_errno(errno);
	}
        
	return sock;
}





static int handle_authentication(prelude_connection_t *cnx,
                                 prelude_connection_permission_t reqperms, prelude_client_profile_t *cp, int crypt)
{
        int ret;
        prelude_string_t *gbuf, *wbuf;
        
        ret = tls_auth_connection(cp, cnx->fd, crypt, &cnx->peer_analyzerid, &cnx->permission);
        if ( ret < 0 ) {
                auth_error(cnx, reqperms, cp);
                return ret;
        }

        if ( (cnx->permission & reqperms) != reqperms ) {
                ret = prelude_string_new(&gbuf);
                if ( ret < 0 )
                        goto err;
                
                ret = prelude_string_new(&wbuf);
                if ( ret < 0 ) {
                        prelude_string_destroy(gbuf);
                        goto err;
                }
                                
                prelude_connection_permission_to_string(cnx->permission, gbuf);
                prelude_connection_permission_to_string(reqperms, wbuf);

                prelude_log(PRELUDE_LOG_WARN,
                            "- Insufficient credentials: got \"%s\" but at least \"%s\" required.\n",
                            prelude_string_get_string(gbuf), prelude_string_get_string(wbuf));

                prelude_string_destroy(gbuf);
                prelude_string_destroy(wbuf);

        err:
                auth_error(cnx, reqperms, cp);
                return prelude_error(PRELUDE_ERROR_INSUFFICIENT_CREDENTIALS);
        }

        prelude_log(PRELUDE_LOG_INFO, "- TLS authentication succeed with Prelude Manager.\n");

        return 0;
}





static int start_inet_connection(prelude_connection_t *cnx,
                                 prelude_connection_permission_t reqperms, prelude_client_profile_t *profile) 
{
        socklen_t len;
        int sock, ret;
        struct sockaddr_in addr;
        
        sock = generic_connect(cnx->sa, cnx->salen);
        if ( sock < 0 )
                return sock;

        prelude_io_set_sys_io(cnx->fd, sock);
        
        ret = handle_authentication(cnx, reqperms, profile, 1);
        if ( ret < 0 ) {
                prelude_io_close(cnx->fd);
                return ret;
        }
        
        /*
         * Get information about the connection,
         * because the sensor might want to know source addr/port used.
         */        
        len = sizeof(addr);

        ret = getsockname(sock, (struct sockaddr *) &addr, &len);
        if ( ret < 0 ) 
                prelude_log(PRELUDE_LOG_ERR, "couldn't get connection informations.\n");
        else {
                cnx->saddr = strdup(inet_ntoa(addr.sin_addr));
                cnx->sport = ntohs(addr.sin_port);
        }
        
        return ret;
}




static int start_unix_connection(prelude_connection_t *cnx,
                                 prelude_connection_permission_t reqperms, prelude_client_profile_t *profile) 
{
        int ret, sock;
        
        sock = generic_connect(cnx->sa, cnx->salen);
        if ( sock < 0 )
                return sock;
        
        prelude_io_set_sys_io(cnx->fd, sock);
        
        ret = handle_authentication(cnx, reqperms, profile, 0);
        if ( ret < 0 )
                prelude_io_close(cnx->fd);
                
        return ret;
}




static int do_connect(prelude_connection_t *cnx,
                      prelude_connection_permission_t reqperms, prelude_client_profile_t *profile) 
{
        int ret;
        
        if ( cnx->sa->sa_family == AF_UNIX ) {
                prelude_log(PRELUDE_LOG_INFO, "- Connecting to %s (UNIX) prelude Manager server.\n",
                            ((struct sockaddr_un *) cnx->sa)->sun_path);
		ret = start_unix_connection(cnx, reqperms, profile);
        } else {
                prelude_log(PRELUDE_LOG_INFO, "- Connecting to %s prelude Manager server.\n", cnx->daddr);
                ret = start_inet_connection(cnx, reqperms, profile);
        }
        
        return ret;
}




static int close_connection_fd(prelude_connection_t *cnx) 
{
        int ret;
        
        if ( ! (cnx->state & PRELUDE_CONNECTION_STATE_ESTABLISHED) )
                return -1;
        
        ret = prelude_io_close(cnx->fd);
        if ( ret < 0 )
                return ret;
        
        if ( cnx->saddr ) {
                free(cnx->saddr);
                cnx->saddr = NULL;
        }
        
        cnx->state &= ~PRELUDE_CONNECTION_STATE_ESTABLISHED;

        return 0;
}



static int close_connection_fd_block(prelude_connection_t *cnx)
{
        int ret;
        
        do {
                ret = close_connection_fd(cnx);
        } while ( ret < 0 && prelude_error_get_code(ret) == PRELUDE_ERROR_EAGAIN );

        return ret;
}



static void destroy_connection_fd(prelude_connection_t *cnx)
{
        close_connection_fd_block(cnx);
        
        if ( cnx->state & PRELUDE_CONNECTION_OWN_FD )
                prelude_io_destroy(cnx->fd);
}



static prelude_bool_t is_unix_addr(prelude_connection_t *cnx, const char *addr)
{
        int ret;
        const char *ptr;
        
        ret = strncmp(addr, "unix", 4);
        if ( ret != 0 )
                return FALSE;
        
        ptr = strchr(addr, ':');
        if ( ptr && *(ptr + 1) )
                cnx->daddr = strdup(ptr + 1);
        else
                cnx->daddr = strdup(UNIX_SOCKET);
        
        return TRUE;
}



static int do_getaddrinfo(prelude_connection_t *cnx, struct addrinfo **ai, const char *addr_string)
{
        int ret;
        struct addrinfo hints;
        char buf[1024], *addr;
        unsigned int port = DEFAULT_PORT;

        ret = prelude_parse_address(addr_string, &addr, &port);
        if ( ret < 0 )
                return ret;
        
        memset(&hints, 0, sizeof(hints));
        snprintf(buf, sizeof(buf), "%u", port);

#ifdef AI_ADDRCONFIG
        hints.ai_flags = AI_ADDRCONFIG;
#endif
        
        hints.ai_family = PF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_protocol = IPPROTO_TCP;
        
        ret = getaddrinfo(addr, buf, &hints, ai);
        if ( ret != 0 ) {
                prelude_log(PRELUDE_LOG_WARN, "could not resolve %s: %s.\n",
                            addr, (ret == EAI_SYSTEM) ? strerror(errno) : gai_strerror(ret));
                return prelude_error(PRELUDE_ERROR_CANT_RESOLVE);
        }
        
        snprintf(buf, sizeof(buf), "%s:%d", addr, port);
        free(addr);
        
        cnx->daddr = strdup(buf);
        
        return 0;
}



static int resolve_addr(prelude_connection_t *cnx, const char *addr) 
{
        struct addrinfo *ai;
        struct sockaddr_un *un;
        int ret, ai_family, ai_addrlen;
        
        if ( ! addr || is_unix_addr(cnx, addr) ) {
                ai_family = AF_UNIX;
                ai_addrlen = sizeof(*un);
        }

        else {
                ret = do_getaddrinfo(cnx, &ai, addr);
                if ( ret < 0 )
                        return ret;

                ai_family = ai->ai_family;
                ai_addrlen = ai->ai_addrlen;
        }

        cnx->sa = malloc(ai_addrlen);
        if ( ! cnx->sa ) {
                freeaddrinfo(ai);
                return prelude_error_from_errno(errno);
        }

        cnx->salen = ai_addrlen;
        cnx->sa->sa_family = ai_family;
                
        if ( ai_family != AF_UNIX ) {
                memcpy(cnx->sa, ai->ai_addr, ai->ai_addrlen);
                freeaddrinfo(ai);
        }

        else {
                un = (struct sockaddr_un *) cnx->sa;
                strncpy(un->sun_path, cnx->daddr, sizeof(un->sun_path));
        }
        
        return 0;
}



/**
 * prelude_connection_destroy:
 * @conn: Pointer to a #prelude_connection_t object.
 *
 * Destroy the connection referenced by @conn.
 *
 * In case the connection is still alive, it is closed in a blocking
 * manner. Use prelude_connection_close() if you want to close the
 * connection in a non blocking manner prior prelude_connection_destroy().
 */
void prelude_connection_destroy(prelude_connection_t *conn) 
{        
        destroy_connection_fd(conn);
        
        free(conn->daddr);
        free(conn->sa);
        free(conn);
}




int prelude_connection_new(prelude_connection_t **out, const char *addr)
{
        int ret;
        prelude_connection_t *new;
        
        signal(SIGPIPE, SIG_IGN);

        new = calloc(1, sizeof(*new));
        if ( ! new )
                return prelude_error_from_errno(errno);
        
        ret = prelude_io_new(&new->fd);
        if ( ret < 0 ) {
                free(new);
                return ret;
        }

        if ( addr ) {
                ret = resolve_addr(new, addr);                
                if ( ret < 0 ) {
                        prelude_io_destroy(new->fd);
                        free(new);
                        return prelude_error(PRELUDE_ERROR_CANT_RESOLVE);
                }
        }
        
        new->state = PRELUDE_CONNECTION_OWN_FD;
        
        *out = new;
        
        return 0;
}



void prelude_connection_set_fd_ref(prelude_connection_t *cnx, prelude_io_t *fd) 
{        
        destroy_connection_fd(cnx);        
        cnx->fd = fd;
        cnx->state &= ~PRELUDE_CONNECTION_OWN_FD;
}



void prelude_connection_set_fd_nodup(prelude_connection_t *cnx, prelude_io_t *fd) 
{        
        destroy_connection_fd(cnx);        
        cnx->fd = fd;
        cnx->state |= PRELUDE_CONNECTION_OWN_FD;
}



int prelude_connection_connect(prelude_connection_t *conn,
                               prelude_client_profile_t *profile,
                               prelude_connection_permission_t permission)
{
        int ret;
        prelude_msg_t *msg;
        
        close_connection_fd_block(conn);
        
        ret = do_connect(conn, permission, profile);
        if ( ret < 0 ) 
                return ret;
        
        ret = prelude_msg_new(&msg, 1, sizeof(uint8_t), PRELUDE_MSG_CONNECTION_CAPABILITY, 0);
        if ( ret < 0 )
                goto err;

        prelude_msg_set(msg, permission, 0, NULL);
        ret = prelude_msg_write(msg, conn->fd);
        prelude_msg_destroy(msg);

        if ( ret < 0 )
                goto err;

        conn->state |= PRELUDE_CONNECTION_STATE_ESTABLISHED;
        
        return ret;
        
 err:
        close_connection_fd_block(conn);
        
        return ret;
}




int prelude_connection_close(prelude_connection_t *cnx) 
{
        return close_connection_fd(cnx);
}




int prelude_connection_send(prelude_connection_t *cnx, prelude_msg_t *msg) 
{
        ssize_t ret;
                
        if ( ! (cnx->state & PRELUDE_CONNECTION_STATE_ESTABLISHED) )
                return -1;

        ret = prelude_msg_write(msg, cnx->fd);
        if ( ret < 0 )                
                return ret;

        ret = is_tcp_connection_still_established(cnx->fd);
        if ( ret < 0 )
                return ret;
        
        return ret;
}



int prelude_connection_recv(prelude_connection_t *cnx, prelude_msg_t **msg)
{
        int ret;
        uint8_t tag;
        
        if ( ! (cnx->state & PRELUDE_CONNECTION_STATE_ESTABLISHED) )
                return -1;

        ret = prelude_msg_read(msg, cnx->fd);        
        if ( ret < 0 )
                return ret;

        tag = prelude_msg_get_tag(*msg);
        if ( tag == PRELUDE_MSG_IDMEF && !(cnx->permission & PRELUDE_CONNECTION_PERMISSION_IDMEF_READ) ) {
                prelude_log(PRELUDE_LOG_WARN, "insufficiant credentials for receiving IDMEF message.\n");
                return prelude_error(PRELUDE_ERROR_INSUFFICIENT_CREDENTIALS);
        }
        
        if ( tag == PRELUDE_MSG_OPTION_REQUEST && !(cnx->permission & PRELUDE_CONNECTION_PERMISSION_ADMIN_READ) ) {
                prelude_log(PRELUDE_LOG_WARN, "insufficiant credentials for receiving IDMEF message.\n");
                return prelude_error(PRELUDE_ERROR_INSUFFICIENT_CREDENTIALS);
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
 * prelude_connection_get_local_addr:
 * @cnx: Pointer to a #prelude_connection_t object.
 *
 * Returns: the local address used to connect.
 */
const char *prelude_connection_get_local_addr(prelude_connection_t *cnx) 
{
        return cnx->saddr;
}




/**
 * prelude_connection_get_local_port:
 * @cnx: Pointer to a #prelude_connection_t object.
 *
 * Returns: the local port used to connect.
 */
unsigned int prelude_connection_get_local_port(prelude_connection_t *cnx) 
{
        return cnx->sport;
}




/**
 * prelude_connection_is_alive:
 * @cnx: Pointer to a #prelude_connection_t object.
 *
 * Returns: 0 if the connection associated with @cnx is alive, -1 otherwise.
 */
prelude_bool_t prelude_connection_is_alive(prelude_connection_t *cnx) 
{
        return (cnx->state & PRELUDE_CONNECTION_STATE_ESTABLISHED) ? TRUE : FALSE;
}



void prelude_connection_set_state(prelude_connection_t *cnx, prelude_connection_state_t state) 
{
        cnx->state = state;
}




prelude_connection_state_t prelude_connection_get_state(prelude_connection_t *cnx)
{
        return cnx->state;
}




ssize_t prelude_connection_forward(prelude_connection_t *cnx, prelude_io_t *src, size_t count) 
{
        ssize_t ret;

        if ( ! (cnx->state & PRELUDE_CONNECTION_STATE_ESTABLISHED) )
                return -1;
        
        ret = prelude_io_forward(cnx->fd, src, count);
        if ( ret < 0 )
                return ret;
        
        ret = is_tcp_connection_still_established(cnx->fd);
        if ( ret < 0 )
                return ret;
        
        return 0;
}



const char *prelude_connection_get_default_socket_filename(void) 
{
        return UNIX_SOCKET;
}



unsigned int prelude_connection_get_peer_port(prelude_connection_t *cnx)
{
        return cnx->dport;
}


const char *prelude_connection_get_peer_addr(prelude_connection_t *cnx)
{
        return cnx->daddr;
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
                return ret;

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




int prelude_connection_permission_new_from_string(prelude_connection_permission_t *out, const char *permission)
{
        int i, c;
        const char *tptr;
        char buf[1024], *tmp;
        const struct {
                const char *name;
                prelude_connection_permission_t val_read;
                prelude_connection_permission_t val_write;
        } tbl[] = {
                { "idmef", PRELUDE_CONNECTION_PERMISSION_IDMEF_READ, PRELUDE_CONNECTION_PERMISSION_IDMEF_WRITE },
                { "admin", PRELUDE_CONNECTION_PERMISSION_ADMIN_READ, PRELUDE_CONNECTION_PERMISSION_ADMIN_WRITE },
                { NULL, 0 },
        };
        
        *out = 0;
        strncpy(buf, permission, sizeof(buf));

        tmp = buf;
        while ( (tptr = strsep(&tmp, ":")) ) {

                while ( *tptr == ' ' ) tptr++;
                if ( ! *tptr )
                        continue;
                                
                for ( i = 0; tbl[i].name; i++ ) {
                        if ( strcmp(tbl[i].name, tptr) != 0 )
                                continue;

                        break;
                }

                if ( ! tbl[i].name )
                        return prelude_error(PRELUDE_ERROR_UNKNOWN_PERMISSION_TYPE);
                
                while ( *tmp == ' ' )
                        tmp++;
                                        
                while ( (c = *tmp++) ) {
                        
                        if ( c == 'r' )
                                *out |= tbl[i].val_read;
                        
                        else if ( c == 'w' )
                                *out |= tbl[i].val_write;
                        
                        else if ( c == ' ' )
                                break;
                        
                        else return prelude_error(PRELUDE_ERROR_UNKNOWN_PERMISSION_BIT);
                }
        }

        return 0;
}



int prelude_connection_permission_to_string(prelude_connection_permission_t permission, prelude_string_t *out)
{
        int i, ret = 0;
        const struct {
                const char *name;
                prelude_connection_permission_t val_read;
                prelude_connection_permission_t val_write;
        } tbl[] = {
                { "idmef", PRELUDE_CONNECTION_PERMISSION_IDMEF_READ, PRELUDE_CONNECTION_PERMISSION_IDMEF_WRITE },
                { "admin", PRELUDE_CONNECTION_PERMISSION_ADMIN_READ, PRELUDE_CONNECTION_PERMISSION_ADMIN_WRITE },
        };

        for ( i = 0; i < (sizeof(tbl) / sizeof(*tbl)); i++ ) {
                
                if ( ! (permission & (tbl[i].val_read|tbl[i].val_write)) )
                        continue;
                
                ret = prelude_string_sprintf(out, "%s%s:", (! prelude_string_is_empty(out)) ? " " : "", tbl[i].name);
                if ( ret < 0 )
                        return ret;
                                
                if ( (permission & tbl[i].val_read) == tbl[i].val_read )
                        prelude_string_cat(out, "r");
                
                if ( (permission & tbl[i].val_write) == tbl[i].val_write )
                        prelude_string_cat(out, "w");
        }
        
        return 0;
}



prelude_connection_permission_t prelude_connection_get_permission(prelude_connection_t *conn)
{
        return conn->permission;
}
