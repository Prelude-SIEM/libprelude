/*****
*
* Copyright (C) 2001-2005,2006,2007 PreludeIDS Technologies. All Rights Reserved.
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
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <netdb.h>
#include <poll.h>

#include <gnutls/gnutls.h>

#ifdef HAVE_SYS_UN_H
# include <sys/un.h>
#endif

#ifdef HAVE_SYS_IOCTL_H
# include <sys/ioctl.h>
#endif

#ifdef HAVE_NETINET_TCP_H
# include <netinet/tcp.h> /* TCP keepalive stuff */
#endif

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
#define DEFAULT_PORT 4690

/*
 * Path to the default Unix socket.
 */
#define UNIX_SOCKET "/tmp/.prelude-unix"


/*
 * FIXME: we need a high level configuration object allowing
 * to fetch per-client settings easily.
 */
int _prelude_connection_keepalive_time = 0;
int _prelude_connection_keepalive_probes = 0;
int _prelude_connection_keepalive_intvl = 0;



struct prelude_connection {
        PRELUDE_LINKED_OBJECT;

        int refcount;

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
        prelude_msg_t *msg;

        prelude_connection_state_t state;
};



static int connection_write_msgbuf(prelude_msgbuf_t *msgbuf, prelude_msg_t *msg)
{
        return prelude_connection_send(prelude_msgbuf_get_data(msgbuf), msg);
}



static int auth_error(prelude_connection_t *cnx,
                      prelude_connection_permission_t reqperms,
                      prelude_client_profile_t *cp, prelude_error_t error, const char *fmt, ...)
{
        int ret;
        va_list ap;
        char *tmp, buf[1024];
        prelude_string_t *out;

        prelude_string_new(&out);
        prelude_connection_permission_to_string(reqperms, out);

        tmp = strrchr(cnx->daddr, ':');
        if ( tmp )
                *tmp = '\0';

        va_start(ap, fmt);
        vsnprintf(buf, sizeof(buf), fmt, ap);
        va_end(ap);

        ret = prelude_error_verbose_make(prelude_error_get_source(error), prelude_error_get_code(error), "%s.\n\n"
                 "In order to register this sensor, please run:\n"
                 "prelude-admin register %s \"%s\" %s --uid %d --gid %d",
                 buf, prelude_client_profile_get_name(cp), prelude_string_get_string(out),
                 (cnx->sa->sa_family == AF_UNIX) ? "<manager address>" : cnx->daddr,
                 (int) prelude_client_profile_get_uid(cp), (int) prelude_client_profile_get_gid(cp));

        prelude_string_destroy(out);
        if ( tmp )
                *tmp = ':';

        return ret;
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



static void set_single_socket_option(int sock, const char *name, int level, int option, int value)
{
        int ret;

        if ( ! value )
                return;

        if ( option < 0 ) {
                prelude_log(PRELUDE_LOG_ERR, "'%s' socket option is unavailable on this system.\n", name);
                return;
        }

        ret = setsockopt(sock, level, option, (void *) &value, sizeof(value));
        if ( ret < 0 )
                prelude_log(PRELUDE_LOG_ERR, "could not set '%s' socket option: %s.\n", name, strerror(errno));
}



static void set_inet_socket_option(int sock, struct sockaddr *sa)
{
# ifdef TCP_KEEPIDLE
        set_single_socket_option(sock, "tcp-keepalive-time", IPPROTO_TCP, TCP_KEEPIDLE, _prelude_connection_keepalive_time);
# else
        set_single_socket_option(sock, "tcp-keepalive-time", IPPROTO_TCP, -1, _prelude_connection_keepalive_time);
# endif

# ifdef TCP_KEEPINTVL
        set_single_socket_option(sock, "tcp-keepalive-intvl", IPPROTO_TCP, TCP_KEEPINTVL, _prelude_connection_keepalive_intvl);
# else
        set_single_socket_option(sock, "tcp-keepalive-intvl", IPPROTO_TCP, -1, _prelude_connection_keepalive_intvl);
# endif

# ifdef TCP_KEEPCNT
        set_single_socket_option(sock, "tcp-keepalive-probes", IPPROTO_TCP, TCP_KEEPCNT, _prelude_connection_keepalive_probes);
# else
        set_single_socket_option(sock, "tcp-keepalive-probes", IPPROTO_TCP, -1, _prelude_connection_keepalive_probes);
# endif
}



/*
 * Connect to the specified address in a generic manner
 * (can be Unix or Inet ).
 */
static int generic_connect(struct sockaddr *sa, socklen_t salen)
{
        int ret, sock;

        sock = socket(sa->sa_family, SOCK_STREAM, 0);
        if ( sock < 0 )
                return prelude_error_from_errno(errno);

#if !((defined _WIN32 || defined __WIN32__) && !defined __CYGWIN__)
        fcntl(sock, F_SETFD, fcntl(sock, F_GETFD) | FD_CLOEXEC);

        ret = fcntl(sock, F_SETOWN, getpid());
        if ( ret < 0 ) {
                close(sock);
                return prelude_error_from_errno(errno);
        }
#endif

        set_single_socket_option(sock, "SO_KEEPALIVE", SOL_SOCKET, SO_KEEPALIVE, 1);
        if ( sa->sa_family != AF_UNIX )
                set_inet_socket_option(sock, sa);

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
        if ( ret < 0 )
                return auth_error(cnx, reqperms, cp, ret, "%s", prelude_strerror(ret));

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

                ret = auth_error(cnx, reqperms, cp, prelude_error(PRELUDE_ERROR_PROFILE),
                                 "Insufficient credentials: got '%s' but at least '%s' required",
                                 prelude_string_get_string(gbuf), prelude_string_get_string(wbuf));

                prelude_string_destroy(gbuf);
                prelude_string_destroy(wbuf);

        err:
                return ret;
        }

        prelude_log(PRELUDE_LOG_INFO, "TLS authentication succeed with Prelude Manager.\n");

        return 0;
}





static int start_inet_connection(prelude_connection_t *cnx,
                                 prelude_connection_permission_t reqperms, prelude_client_profile_t *profile)
{
        socklen_t len;
        int sock, ret, tmp;
        union {
                struct sockaddr sa;
#ifdef HAVE_IPV6
                struct sockaddr_in6 addr;
# define ADDR_PORT(x) (x).sin6_port
#else
                struct sockaddr_in addr;
# define ADDR_PORT(x) (x).sin_port
#endif
        } addr;

        sock = generic_connect(cnx->sa, cnx->salen);
        if ( sock < 0 )
                return sock;

        prelude_io_set_sys_io(cnx->fd, sock);

        ret = handle_authentication(cnx, reqperms, profile, 1);
        if ( ret < 0 ) {
                do {
                        tmp = prelude_io_close(cnx->fd);
                } while ( tmp < 0 && ! prelude_io_is_error_fatal(cnx->fd, tmp) );

                return ret;
        }

        /*
         * Get information about the connection,
         * because the sensor might want to know source addr/port used.
         */
        len = sizeof(addr.addr);

        ret = getsockname(sock, &addr.sa, &len);
        if ( ret < 0 )
                ret = prelude_error_verbose(PRELUDE_ERROR_SYSTEM_ERROR, "getsockname failed: %s", strerror(errno));
        else {
                char buf[512];

                if ( inet_ntop(addr.sa.sa_family, prelude_sockaddr_get_inaddr(&addr.sa), buf, sizeof(buf)) )
                        cnx->saddr = strdup(buf);
                else
                        cnx->saddr = NULL;

                cnx->sport = ntohs(ADDR_PORT(addr.addr));
        }

        return ret;
}



#if !((defined _WIN32 || defined __WIN32__) && !defined __CYGWIN__)
static int start_unix_connection(prelude_connection_t *cnx,
                                 prelude_connection_permission_t reqperms, prelude_client_profile_t *profile)
{
        int ret, sock, tmp;

        sock = generic_connect(cnx->sa, cnx->salen);
        if ( sock < 0 )
                return sock;

        prelude_io_set_sys_io(cnx->fd, sock);

        ret = handle_authentication(cnx, reqperms, profile, 0);
        if ( ret < 0 )
                do {
                        tmp = prelude_io_close(cnx->fd);
                } while ( tmp < 0 && ! prelude_io_is_error_fatal(cnx->fd, tmp) );

        return ret;
}
#endif



static int do_connect(prelude_connection_t *cnx,
                      prelude_connection_permission_t reqperms, prelude_client_profile_t *profile)
{
        int ret = 0;

        if ( cnx->sa->sa_family != AF_UNIX ) {
                prelude_log(PRELUDE_LOG_INFO, "Connecting to %s prelude Manager server.\n", cnx->daddr);
                ret = start_inet_connection(cnx, reqperms, profile);
        }

#if !((defined _WIN32 || defined __WIN32__) && !defined __CYGWIN__)
        else {
                prelude_log(PRELUDE_LOG_INFO, "Connecting to %s (UNIX) prelude Manager server.\n",
                            ((struct sockaddr_un *) cnx->sa)->sun_path);
                ret = start_unix_connection(cnx, reqperms, profile);
        }
#endif

        return ret;
}




static int close_connection_fd(prelude_connection_t *cnx)
{
        int ret;

        if ( ! (cnx->state & PRELUDE_CONNECTION_STATE_ESTABLISHED) )
                return -1;

        ret = prelude_io_close(cnx->fd);
        if ( ret >= 0 || prelude_io_is_error_fatal(cnx->fd, ret) ) {
                if ( cnx->saddr ) {
                        free(cnx->saddr);
                        cnx->saddr = NULL;
                }

                cnx->state &= ~PRELUDE_CONNECTION_STATE_ESTABLISHED;
        }

        return ret;
}



static int close_connection_fd_block(prelude_connection_t *cnx)
{
        int ret;

        if ( ! (cnx->state & PRELUDE_CONNECTION_STATE_ESTABLISHED) )
                return -1;

        do {
                ret = close_connection_fd(cnx);
        } while ( ret < 0 && ! prelude_io_is_error_fatal(cnx->fd, ret) );

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
                ret = prelude_error_verbose(PRELUDE_ERROR_CANT_RESOLVE, "could not resolve '%s': %s",
                                            addr, (ret == EAI_SYSTEM) ? strerror(errno) : gai_strerror(ret));
                free(addr);
                return ret;
        }

        snprintf(buf, sizeof(buf), "%s:%d", addr, port);
        free(addr);

        cnx->daddr = strdup(buf);

        return 0;
}



static int resolve_addr(prelude_connection_t *cnx, const char *addr)
{
        struct addrinfo *ai = NULL;
        int ret, ai_family, ai_addrlen;
#if !((defined _WIN32 || defined __WIN32__) && !defined __CYGWIN__)
        struct sockaddr_un *un;
#endif

        if ( is_unix_addr(cnx, addr) ) {
#if (defined _WIN32 || defined __WIN32__) && !defined __CYGWIN__
                return prelude_error_verbose(PRELUDE_ERROR_GENERIC, "UNIX socket are not supported under this environment");
#else
                ai_family = AF_UNIX;
                ai_addrlen = sizeof(*un);
#endif
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
                if ( ai )
                        freeaddrinfo(ai);

                return prelude_error_from_errno(errno);
        }

        cnx->salen = ai_addrlen;
        cnx->sa->sa_family = ai_family;

        if ( ai_family != AF_UNIX ) {
                memcpy(cnx->sa, ai->ai_addr, ai->ai_addrlen);
                freeaddrinfo(ai);
        }

#if !((defined _WIN32 || defined __WIN32__) && !defined __CYGWIN__)
        else {
                un = (struct sockaddr_un *) cnx->sa;
                strncpy(un->sun_path, cnx->daddr, sizeof(un->sun_path));
        }
#endif

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
        prelude_return_if_fail(conn);

        if ( --conn->refcount > 0 )
                return;

        destroy_connection_fd(conn);

        free(conn->daddr);
        free(conn->sa);
        free(conn);
}




int prelude_connection_new(prelude_connection_t **out, const char *addr)
{
        int ret;
        prelude_connection_t *new;

        prelude_return_val_if_fail(addr, prelude_error(PRELUDE_ERROR_ASSERTION));

#if !((defined _WIN32 || defined __WIN32__) && !defined __CYGWIN__)
        signal(SIGPIPE, SIG_IGN);
#endif

        new = calloc(1, sizeof(*new));
        if ( ! new )
                return prelude_error_from_errno(errno);

        new->refcount = 1;

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
                        return ret;
                }
        }

        new->state = PRELUDE_CONNECTION_OWN_FD;

        *out = new;

        return 0;
}



void prelude_connection_set_fd_ref(prelude_connection_t *cnx, prelude_io_t *fd)
{
        prelude_return_if_fail(cnx);
        prelude_return_if_fail(fd);

        destroy_connection_fd(cnx);
        cnx->fd = fd;
        cnx->state &= ~PRELUDE_CONNECTION_OWN_FD;
}



void prelude_connection_set_fd_nodup(prelude_connection_t *cnx, prelude_io_t *fd)
{
        prelude_return_if_fail(cnx);
        prelude_return_if_fail(fd);

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

        prelude_return_val_if_fail(conn, prelude_error(PRELUDE_ERROR_ASSERTION));
        prelude_return_val_if_fail(profile, prelude_error(PRELUDE_ERROR_ASSERTION));

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
        prelude_return_val_if_fail(cnx, prelude_error(PRELUDE_ERROR_ASSERTION));
        return close_connection_fd(cnx);
}




int prelude_connection_send(prelude_connection_t *cnx, prelude_msg_t *msg)
{
        ssize_t ret;

        prelude_return_val_if_fail(cnx, prelude_error(PRELUDE_ERROR_ASSERTION));
        prelude_return_val_if_fail(msg, prelude_error(PRELUDE_ERROR_ASSERTION));

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

        prelude_return_val_if_fail(cnx, prelude_error(PRELUDE_ERROR_ASSERTION));

        if ( ! (cnx->state & PRELUDE_CONNECTION_STATE_ESTABLISHED) )
                return -1;

        ret = prelude_msg_read(msg, cnx->fd);
        if ( ret < 0 )
                return ret;

        tag = prelude_msg_get_tag(*msg);
        if ( tag == PRELUDE_MSG_IDMEF && !(cnx->permission & PRELUDE_CONNECTION_PERMISSION_IDMEF_READ) )
                return prelude_error_verbose(PRELUDE_ERROR_PROFILE,
                                             "Insufficient credentials for receiving IDMEF message");

        if ( tag == PRELUDE_MSG_OPTION_REQUEST && !(cnx->permission & PRELUDE_CONNECTION_PERMISSION_ADMIN_READ) )
                return prelude_error_verbose(PRELUDE_ERROR_PROFILE,
                                             "Insufficient credentials for receiving administrative message");

        return ret;
}


int prelude_connection_recv_idmef(prelude_connection_t *con, idmef_message_t **idmef)
{
        int ret;

        ret = prelude_connection_recv(con, &con->msg);
        if ( ret < 0 ) {
                if ( prelude_error_get_code(ret) != PRELUDE_ERROR_EAGAIN )
                        con->msg = NULL;

                return ret;
        }
        if ( prelude_msg_get_tag(con->msg) != PRELUDE_MSG_IDMEF ) {
                prelude_msg_destroy(con->msg);
                con->msg = NULL;
                return prelude_error_from_errno(EINVAL);
        }

        ret = idmef_message_new(idmef);
        if ( ret < 0 ) {
                prelude_msg_destroy(con->msg);
                con->msg = NULL;
                return ret;
        }

        ret = idmef_message_read(*idmef, con->msg);
        if ( ret < 0 ) {
                idmef_message_destroy(*idmef);
                prelude_msg_destroy(con->msg);
                con->msg = NULL;
                return ret;
        }

        idmef_message_set_pmsg(*idmef, con->msg);
        con->msg = NULL;

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
        prelude_return_val_if_fail(cnx, NULL);
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
        prelude_return_val_if_fail(cnx, NULL);
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
        prelude_return_val_if_fail(cnx, 0);
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
        prelude_return_val_if_fail(cnx, FALSE);
        return (cnx->state & PRELUDE_CONNECTION_STATE_ESTABLISHED) ? TRUE : FALSE;
}



void prelude_connection_set_state(prelude_connection_t *cnx, prelude_connection_state_t state)
{
        prelude_return_if_fail(cnx);
        cnx->state = state;
}




prelude_connection_state_t prelude_connection_get_state(prelude_connection_t *cnx)
{
        prelude_return_val_if_fail(cnx, prelude_error(PRELUDE_ERROR_ASSERTION));
        return cnx->state;
}




ssize_t prelude_connection_forward(prelude_connection_t *cnx, prelude_io_t *src, size_t count)
{
        ssize_t ret;

        prelude_return_val_if_fail(cnx, prelude_error(PRELUDE_ERROR_ASSERTION));
        prelude_return_val_if_fail(src, prelude_error(PRELUDE_ERROR_ASSERTION));

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
        prelude_return_val_if_fail(cnx, 0);
        return cnx->dport;
}


const char *prelude_connection_get_peer_addr(prelude_connection_t *cnx)
{
        prelude_return_val_if_fail(cnx, NULL);
        return cnx->daddr;
}



uint64_t prelude_connection_get_peer_analyzerid(prelude_connection_t *cnx)
{
        prelude_return_val_if_fail(cnx, 0);
        return cnx->peer_analyzerid;
}


void prelude_connection_set_peer_analyzerid(prelude_connection_t *cnx, uint64_t analyzerid)
{
        prelude_return_if_fail(cnx);
        cnx->peer_analyzerid = analyzerid;
}



int prelude_connection_new_msgbuf(prelude_connection_t *connection, prelude_msgbuf_t **msgbuf)
{
        int ret;

        prelude_return_val_if_fail(connection, prelude_error(PRELUDE_ERROR_ASSERTION));

        ret = prelude_msgbuf_new(msgbuf);
        if ( ret < 0 )
                return ret;

        prelude_msgbuf_set_data(*msgbuf, connection);
        prelude_msgbuf_set_callback(*msgbuf, connection_write_msgbuf);

        return 0;
}



void prelude_connection_set_data(prelude_connection_t *conn, void *data)
{
        prelude_return_if_fail(conn);
        conn->data = data;
}



void *prelude_connection_get_data(prelude_connection_t *conn)
{
        prelude_return_val_if_fail(conn, NULL);
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
                { NULL, 0, 0 },
        };

        prelude_return_val_if_fail(out, prelude_error(PRELUDE_ERROR_ASSERTION));
        prelude_return_val_if_fail(permission, prelude_error(PRELUDE_ERROR_ASSERTION));

        *out = 0;
        strncpy(buf, permission, sizeof(buf));

        tmp = buf;
        while ( (tptr = strsep(&tmp, ":")) ) {
                if ( ! tmp )
                        continue;

                while ( *tptr == ' ' ) tptr++;
                if ( ! *tptr )
                        continue;

                for ( i = 0; tbl[i].name; i++ ) {
                        if ( strcmp(tbl[i].name, tptr) != 0 )
                                continue;

                        break;
                }

                if ( ! tbl[i].name )
                        return prelude_error_verbose(PRELUDE_ERROR_UNKNOWN_PERMISSION_TYPE, "unknown permission type '%s'", tptr);

                while ( *tmp == ' ' )
                        tmp++;

                while ( (c = *tmp++) ) {

                        if ( c == 'r' )
                                *out |= tbl[i].val_read;

                        else if ( c == 'w' )
                                *out |= tbl[i].val_write;

                        else if ( c == ' ' )
                                break;

                        else return prelude_error_verbose(PRELUDE_ERROR_UNKNOWN_PERMISSION_BIT, "unknown permission bit: '%c'", c);
                }
        }

        return 0;
}



int prelude_connection_permission_to_string(prelude_connection_permission_t permission, prelude_string_t *out)
{
        size_t i;
        int ret = 0;
        const struct {
                const char *name;
                prelude_connection_permission_t val_read;
                prelude_connection_permission_t val_write;
        } tbl[] = {
                { "idmef", PRELUDE_CONNECTION_PERMISSION_IDMEF_READ, PRELUDE_CONNECTION_PERMISSION_IDMEF_WRITE },
                { "admin", PRELUDE_CONNECTION_PERMISSION_ADMIN_READ, PRELUDE_CONNECTION_PERMISSION_ADMIN_WRITE },
        };

        prelude_return_val_if_fail(out, prelude_error(PRELUDE_ERROR_ASSERTION));

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
        prelude_return_val_if_fail(conn, 0);
        return conn->permission;
}


/**
 * prelude_connection_ref:
 * @conn: Pointer to a #prelude_connection_t object to reference.
 *
 * Increases @conn reference count.
 *
 * prelude_connection_destroy() will decrease the refcount until it
 * reaches 0, at which point @conn will be destroyed.
 *
 * Returns: @conn.
 */
prelude_connection_t *prelude_connection_ref(prelude_connection_t *conn)
{
        prelude_return_val_if_fail(conn, NULL);

        conn->refcount++;
        return conn;
}
