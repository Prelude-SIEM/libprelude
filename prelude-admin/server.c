/*****
*
* Copyright (C) 2004, 2005 PreludeIDS Technologies. All Rights Reserved.
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
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <poll.h>

#include <gcrypt.h>
#include <gnutls/gnutls.h>
#include <gnutls/extra.h>

#include "prelude-client.h"
#include "prelude-error.h"
#include "common.h"

#include "server.h"
#include "tls-register.h"


#define ANON_DH_BITS 1024


static const char *one_shot_passwd;
static gnutls_anon_server_credentials anoncred;


#ifdef GNUTLS_SRP_ENABLED
 static gnutls_srp_server_credentials srpcred;
#endif


static int anon_check_passwd(prelude_io_t *fd)
{
        ssize_t ret;
        const char *result;
        unsigned char *rbuf;

        ret = prelude_io_read_delimited(fd, &rbuf);
        if ( ret < 0 ) {
                fprintf(stderr, "error receiving authentication result: %s.\n", prelude_strerror(ret));
                return -1;
        }

        if ( rbuf[ret - 1] != 0 ) {
                fprintf(stderr, "invalid password token received.\n");
                return -1;
        }

        if ( strcmp((char *) rbuf, one_shot_passwd) == 0 )  {
                result = "OK";
        } else {
                result = "NOK";
                fprintf(stderr, "Anonymous authentication one-shot password check failed.\n");
        }

        free(rbuf);

        ret = prelude_io_write_delimited(fd, result, strlen(result));
        if ( ret < 0 ) {
                fprintf(stderr, "error sending authentication token: %s.\n", prelude_strerror(ret));
                return -1;
        }

        return *result == 'O' ? 0 : -1;
}


static inline gnutls_transport_ptr fd_to_ptr(int fd)
{
        union {
                gnutls_transport_ptr ptr;
                int fd;
        } data;

        data.fd = fd;

        return data.ptr;
}


static inline int ptr_to_fd(gnutls_transport_ptr ptr)
{
        union {
                gnutls_transport_ptr ptr;
                int fd;
        } data;

        data.ptr = ptr;

        return data.fd;
}


static ssize_t tls_pull(gnutls_transport_ptr fd, void *buf, size_t count)
{
        return read(ptr_to_fd(fd), buf, count);
}



static ssize_t tls_push(gnutls_transport_ptr fd, const void *buf, size_t count)
{
        return write(ptr_to_fd(fd), buf, count);
}


static gnutls_session new_tls_session(int sock)
{
        int ret;
        gnutls_session session;
        const int kx_priority[] = {
                GNUTLS_KX_ANON_DH,
#ifdef GNUTLS_SRP_ENABLED
                GNUTLS_KX_SRP, GNUTLS_KX_SRP_DSS, GNUTLS_KX_SRP_RSA,
#endif
                0 };

        gnutls_init(&session, GNUTLS_SERVER);

        gnutls_set_default_priority(session);
        gnutls_kx_set_priority(session, kx_priority);

#ifdef GNUTLS_SRP_ENABLED
        gnutls_credentials_set(session, GNUTLS_CRD_SRP, srpcred);
        gnutls_certificate_server_set_request(session, GNUTLS_CERT_IGNORE);
#endif
        gnutls_credentials_set(session, GNUTLS_CRD_ANON, anoncred);

        gnutls_transport_set_ptr(session, fd_to_ptr(sock));
        gnutls_transport_set_pull_function(session, tls_pull);
        gnutls_transport_set_push_function(session, tls_push);

        ret = gnutls_handshake(session);
        if ( ret < 0 ) {
                fprintf(stderr, "GnuTLS handshake failed: %s.\n", gnutls_strerror(ret));
                gnutls_alert_send_appropriate(session, ret);
                return NULL;
        }

        return session;
}



static int handle_client_connection(const char *srcinfo, prelude_client_profile_t *cp, prelude_io_t *fd,
                                    gnutls_x509_privkey key, gnutls_x509_crt cacrt, gnutls_x509_crt crt)
{
        gnutls_session session;

        session = new_tls_session(prelude_io_get_fd(fd));
        if ( ! session )
                return -1;

        prelude_io_set_tls_io(fd, session);

        if ( gnutls_auth_get_type(session) == GNUTLS_CRD_ANON && anon_check_passwd(fd) < 0 )
                return -1;

        return tls_handle_certificate_request(srcinfo, cp, fd, key, cacrt, crt);
}



static int process_event(prelude_client_profile_t *cp, int server_sock, prelude_io_t *fd,
                         gnutls_x509_privkey key, gnutls_x509_crt cacrt, gnutls_x509_crt crt)
{
        char buf[512];
        void *inaddr;
        socklen_t len;
        int ret, csock;
        union {
                struct sockaddr sa;
#ifndef HAVE_IPV6
                struct sockaddr_in addr;
# define ADDR_PORT(x) (x).sin_port
#else
                struct sockaddr_in6 addr;
# define ADDR_PORT(x) (x).sin6_port
#endif
        } addr;

        len = sizeof(addr.addr);

        csock = accept(server_sock, &addr.sa, &len);
        if ( csock < 0 ) {
                fprintf(stderr, "accept returned an error: %s.\n", strerror(errno));
                return -1;
        }

        inaddr = prelude_sockaddr_get_inaddr(&addr.sa);
        if ( ! inaddr )
                return -1;

        inet_ntop(addr.sa.sa_family, inaddr, buf, sizeof(buf));
        snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf), ":%u", ntohs(ADDR_PORT(addr.addr)));

        prelude_io_set_sys_io(fd, csock);

        fprintf(stderr, "\nConnection from %s...\n", buf);
        ret = handle_client_connection("", cp, fd, key, cacrt, crt);
        if ( ret == 0 )
                fprintf(stderr, "%s successfully registered.\n", buf);

        prelude_io_close(fd);

        return ret;
}



static int wait_connection(prelude_client_profile_t *cp, int sock,
                           struct pollfd *pfd, size_t size, int keepalive,
                           gnutls_x509_privkey key, gnutls_x509_crt cacrt, gnutls_x509_crt crt)
{
        size_t i;
        prelude_io_t *fd;
        int ret, active_fd;

        ret = prelude_io_new(&fd);
        if ( ret < 0 ) {
                fprintf(stderr, "%s: error creating a new IO object: %s.\n",
                        prelude_strsource(ret), prelude_strerror(ret));
                return -1;
        }

        do {
                active_fd = poll(pfd, size, -1);
                if ( active_fd < 0 ) {
                        if ( errno != EINTR )
                                fprintf(stderr, "poll error : %s.\n", strerror(errno));
                        return -1;
                }

                for ( i = 0; i < size && active_fd > 0; i++ ) {
                        if ( pfd[i].revents & POLLIN ) {
                                active_fd--;
                                ret = process_event(cp, pfd[i].fd, fd, key, cacrt, crt);
                        }
                }

        } while ( keepalive || ret < 0 );

        prelude_io_destroy(fd);

        return ret;
}



static int setup_server(const char *addr, unsigned int port, struct pollfd *pfd, size_t *size)
{
        size_t i = 0;
        char buf[1024];
        struct addrinfo hints, *ai, *ai_start;
        int sock, ret, on = 1, prev_family = PF_UNSPEC;

        snprintf(buf, sizeof(buf), "%u", port);
        memset(&hints, 0, sizeof(hints));

        hints.ai_flags = AI_PASSIVE;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_protocol = IPPROTO_TCP;
        hints.ai_family = PF_UNSPEC;

#ifdef AI_ADDRCONFIG
        hints.ai_flags |= AI_ADDRCONFIG;
#endif

        ret = getaddrinfo(addr, buf, &hints, &ai);
        if ( ret != 0 ) {
                fprintf(stderr, "could not resolve %s: %s.\n", addr ? addr : "",
                        (ret == EAI_SYSTEM) ? strerror(errno) : gai_strerror(ret));
                return -1;
        }

        for ( ai_start = ai; ai && i < *size; ai = ai->ai_next ) {
                inet_ntop(ai->ai_family, prelude_sockaddr_get_inaddr(ai->ai_addr), buf, sizeof(buf));

                fprintf(stderr, "Waiting for peers install request on %s:%u...\n",
                        buf, ntohs(((struct sockaddr_in *) ai->ai_addr)->sin_port));

                sock = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
                if ( sock < 0 ) {
                        fprintf(stderr, "could not open socket for '%s': %s.\n", buf, strerror(errno));
                        break;
                }

                ret = setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (void *) &on, sizeof(int));
                if ( ret < 0 )
                        fprintf(stderr, "could not set SO_REUSEADDR: %s.\n", strerror(errno));

                ret = bind(sock, ai->ai_addr, ai->ai_addrlen);
                if ( ret < 0 ) {
                        close(sock);

                        /*
                         * More information on this at:
                         * http://lists.debian.org/debian-ipv6/2001/01/msg00031.html
                         */
                        if ( errno == EADDRINUSE && ! addr && prev_family != PF_UNSPEC && ai->ai_family != prev_family ) {
                                ret = 0;
                                continue;
                        }

                        fprintf(stderr, "could not bind to '%s': %s.\n", buf, strerror(errno));
                        break;
                }

                ret = listen(sock, 1);
                if ( ret < 0 ) {
                        close(sock);
                        fprintf(stderr, "could not listen on '%s': %s.\n", buf, strerror(errno));
                        break;
                }

                pfd[i].fd = sock;
                pfd[i].events = POLLIN;
                prev_family = ai->ai_family;

                i++;
        }

        if ( i == 0 ) {
                fprintf(stderr, "could not find any address to listen on.\n");
                return -1;
        }

        freeaddrinfo(ai_start);
        *size = i;

        return ret;
}



#ifdef GNUTLS_SRP_ENABLED

static int copy_datum(gnutls_datum *dst, const gnutls_datum *src)
{
        dst->size = src->size;

        dst->data = gnutls_malloc(dst->size);
        if ( ! dst->data ) {
                fprintf(stderr, "memory exhausted.\n");
                return -1;
        }

        memcpy(dst->data, src->data, dst->size);

        return 0;
}



static int srp_callback(gnutls_session session, const char *username, gnutls_datum *salt,
                        gnutls_datum *verifier, gnutls_datum *generator, gnutls_datum *prime)
{
        int ret;

        if ( strcmp(username, "prelude-adduser") != 0 )
                return -1;

        salt->size = 4;

        salt->data = gnutls_malloc(4);
        if ( ! salt->data ) {
                fprintf(stderr, "memory exhausted.\n");
                return -1;
        }

        gcry_randomize(salt->data, salt->size, GCRY_WEAK_RANDOM);

        ret = copy_datum(generator, &gnutls_srp_1024_group_generator);
        if ( ret < 0 )
                return -1;

        ret = copy_datum(prime, &gnutls_srp_1024_group_prime);
        if ( ret < 0 )
                return -1;

        return gnutls_srp_verifier(username, one_shot_passwd, salt, generator, prime, verifier);
}

#endif


int server_create(prelude_client_profile_t *cp, const char *addr, unsigned int port,
                  prelude_bool_t keepalive, const char *pass, gnutls_x509_privkey key, gnutls_x509_crt cacrt, gnutls_x509_crt crt)
{
        int sock;
        size_t size;
        struct pollfd pfd[128];
        gnutls_dh_params dh_params;

#ifdef GNUTLS_SRP_ENABLED
        int ret;

        ret = gnutls_srp_allocate_server_credentials(&srpcred);
        if ( ret < 0 ) {
                fprintf(stderr, "error creating SRP credentials: %s.\n", gnutls_strerror(ret));
                return -1;
        }

        gnutls_srp_set_server_credentials_function(srpcred, srp_callback);
#endif

        one_shot_passwd = pass;
        gnutls_anon_allocate_server_credentials(&anoncred);

        fprintf(stderr, "Generating %d bits Diffie-Hellman key for anonymous authentication...", ANON_DH_BITS);
        gnutls_dh_params_init(&dh_params);
        gnutls_dh_params_generate2(dh_params, ANON_DH_BITS);
        gnutls_anon_set_server_dh_params(anoncred, dh_params);
        fprintf(stderr, "\n");

        size = sizeof(pfd) / sizeof(*pfd);
        sock = setup_server(addr, port, pfd, &size);
        if ( sock < 0 )
                return -1;

        wait_connection(cp, sock, pfd, size, keepalive, key, cacrt, crt);

#ifdef GNUTLS_SRP_ENABLED
        gnutls_srp_free_server_credentials(srpcred);
#endif

        gnutls_anon_free_server_credentials(anoncred);

        return 0;
}
