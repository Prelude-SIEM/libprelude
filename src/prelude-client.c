/*****
*
* Copyright (C) 2001 Yoann Vandoorselaere <yoann@mandrakesoft.com>
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

#include "config.h"

#ifdef HAVE_SSL
#include "ssl.h"
#endif

#include "list.h"
#include "common.h"
#include "config-engine.h"
#include "plugin-common.h"
#include "prelude-io.h"
#include "prelude-auth.h"
#include "prelude-message.h"
#include "prelude-message-id.h"
#include "prelude-client.h"
#include "prelude-getopt.h"
#include "sensor.h"
#include "client-ident.h"
#include "auth.h"


#define RECONNECT_TIME_WAIT 10
#define UNIX_SOCK "/var/lib/prelude/socket"



struct prelude_client {
        char *addr;
        uint16_t port;
        
        
        /*
         * This pointer point to the object suitable
         * for writing to the Manager.
         */
        prelude_io_t *fd;
        uint8_t connection_broken;
};






/*
 * Connect to the specified address in a generic manner
 * (can be Unix or Inet ).
 */
static int generic_connect(int sock, struct sockaddr *addr, socklen_t addrlen)
{
        int ret;
        
        ret = fcntl(sock, F_SETOWN, getpid());
        if ( ret < 0 ) {
                log(LOG_ERR, "couldn't set children to receive signal.\n");
                return -1;
        }

        ret = connect(sock, addr, addrlen);
	if ( ret < 0 ) {
                log(LOG_ERR,"error connecting socket.\n");
                return -1;
	}
        
	return 0;
}



/*
 * Connect to an UNIX socket.
 */
static int unix_connect(void)
{
        int ret, sock;
    	struct sockaddr_un addr;
        
	log(LOG_INFO, "- Connecting to Unix prelude Manager server.\n");
        
	sock = socket(AF_UNIX, SOCK_STREAM, 0);
	if ( sock < 0 ) {
                log(LOG_ERR,"Error opening unix socket.\n");
		return -1;
	}
        
	addr.sun_family = AF_UNIX;
	strcpy(addr.sun_path, UNIX_SOCK);

        ret = generic_connect(sock, (struct sockaddr *)&addr, sizeof(addr));
        if ( ret < 0 ) {
                log(LOG_ERR,"Error connecting to Manager server.\n");
                close(sock);
                return -1;
        }

	return sock;
}




/*
 * Setup an Inet connection.
 */
static int inet_connect(const char *addr, unsigned int port)
{
        int ret, on, len, sock;
	struct sockaddr_in daddr, saddr;
        
	log(LOG_INFO, "- Connecting to Tcp prelude Manager server %s:%d.\n", addr, port);

        sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if ( sock < 0 ) {
		log(LOG_ERR,"Error opening inet socket.\n");
		return -1;
	}

	daddr.sin_family = AF_INET;
	daddr.sin_port = htons(port);
	daddr.sin_addr.s_addr = inet_addr(addr);

        /*
         * We want packet to be sent as soon as possible,
         * this mean not using the Nagle algorithm which try to minimize
         * packets on the network at the expense of buffering the data.
         */
        on = 1;
        ret = setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, &on, sizeof(int));
        if ( ret < 0 ) 
                log(LOG_ERR, "couldn't turn the tcp Nagle algorithm off.\n");
        
        
        ret = generic_connect(sock, (struct sockaddr *)&daddr, sizeof(daddr));
        if ( ret < 0 ) {
                log(LOG_ERR,"Error connecting to %s.\n", addr);
                goto err;
        }

        len = sizeof(saddr);
        ret = getsockname(sock, (struct sockaddr *)&saddr, &len);
        if ( ret < 0 ) {
                log(LOG_ERR, "couldn't get source connection informations.\n");
                goto err;
        }
        
        return sock;

 err:
        close(sock);
        return -1;
}




static int handle_plaintext_connection(prelude_client_t *client, int sock) 
{
        int ret;
        int ulen, plen;
        char *user, *pass;
        prelude_msg_t *msg;

        ret = prelude_auth_read_entry(SENSORS_AUTH_FILE, NULL, &user, &pass);
        if ( ret < 0 ) {
                log(LOG_INFO,
                    "\n\ncouldn't get username / password to use for plaintext connection.\n"
                    "Please use the \"sensor-adduser\" program on the Manager host.\n\n");
                return -1;
        }
        
        ulen = strlen(user) + 1;
        plen = strlen(pass) + 1;
        
        msg = prelude_msg_new(2, ulen + plen, PRELUDE_MSG_AUTH_PLAINTEXT, 0);
        if ( ! msg ) {
                free(user);
                free(pass);
                return -1;
        }

        prelude_msg_set(msg, PRELUDE_MSG_AUTH_USERNAME, ulen, user);
        prelude_msg_set(msg, PRELUDE_MSG_AUTH_PASSWORD, plen, pass);
        prelude_io_set_sys_io(client->fd, sock);
        
        ret = prelude_msg_write(msg, client->fd);
        if ( ret <= 0 )
                ret = -1;

        prelude_msg_destroy(msg);

        return ret;
}



#ifdef HAVE_SSL

static int handle_ssl_connection(prelude_client_t *client, int sock) 
{
        int ret;
        SSL *ssl;
        prelude_msg_t *msg;
        static int ssl_initialized = 0;
        
        if ( ! ssl_initialized ) {
                ret = ssl_init_client();
                if ( ret < 0 )
                        return -1;
                
                ssl_initialized = 1;
        }
        
        msg = prelude_msg_new(1, 0, PRELUDE_MSG_AUTH, 0);
        if ( ! msg )
                return -1;

        prelude_msg_set(msg, PRELUDE_MSG_AUTH_SSL, 0, NULL);
        prelude_msg_write(msg, client->fd);
        prelude_msg_destroy(msg);
        
        ssl = ssl_connect_server(sock);
        if ( ! ssl ) {
                log(LOG_INFO,
                    "\nSSL authentication failed. Use the \"manager-adduser\"\n"
                    "program on the Manager host together with the \"sensor-adduser\"\n"
                    "program on the Sensor host to create an username for this Sensor.\n");
                return -1;
        }
        
        prelude_io_set_ssl_io(client->fd, ssl);

        return 0;
}

#endif





/*
 * Report server should send us a message containing
 * information about the kind of connecition it support.
 */
static int get_manager_setup(prelude_io_t *fd, int *have_ssl, int *have_plaintext) 
{
        int ret;
        void *buf;
        uint8_t tag;
        uint32_t dlen;
        prelude_msg_t *msg = NULL;
        prelude_msg_status_t status;
        
        status = prelude_msg_read(&msg, fd);
        if ( status != prelude_msg_finished ) {
                log(LOG_ERR, "error reading Manager configuration message.\n");
                return -1;
        }

        if ( prelude_msg_get_tag(msg) != PRELUDE_MSG_AUTH ) {
                log(LOG_ERR, "Manager didn't sent us any authentication message.\n");
                return -1;
        }

        while ( (ret = prelude_msg_get(msg, &tag, &dlen, &buf)) ) {
                
                switch (tag) {

                case PRELUDE_MSG_AUTH_HAVE_SSL:
                        *have_ssl = 1;
                        break;

                case PRELUDE_MSG_AUTH_HAVE_PLAINTEXT:
                        *have_plaintext = 1;
                        break;

                default:
                        log(LOG_ERR, "Invalid authentication tag %d.\n", tag);
                        goto err;
                }
        }
        
 err:
        prelude_msg_destroy(msg);
        return ret;
}




static int start_inet_connection(prelude_client_t *client) 
{
        int ret = -1, have_ssl  = 0, have_plaintext = 0, sock;
        
        sock = inet_connect(client->addr, client->port);
        if ( sock < 0 )
                return -1;

        prelude_io_set_sys_io(client->fd, sock);
        
        ret = get_manager_setup(client->fd, &have_ssl, &have_plaintext);
        if ( ret < 0 ) {
                close(sock);
                return -1;
        }

#ifdef HAVE_SSL
        if ( have_ssl ) {
                ret = handle_ssl_connection(client, sock);
                goto end;
        }
#endif

        if ( have_plaintext ) {
                ret = handle_plaintext_connection(client, sock);
                goto end;
        } else {
                log(LOG_INFO, "couldn't agree on a protocol to use.\n");
                ret = -1;
        }

 end:
        if ( ret < 0 )
                close(sock);
        
        return ret;
}




static int start_unix_connection(prelude_client_t *client) 
{
        int sock;
        
        sock = unix_connect();
        if ( sock < 0 )
                return -1;

        prelude_io_set_sys_io(client->fd, sock);
        
        return handle_plaintext_connection(client, sock);
}




static int do_connect(prelude_client_t *client) 
{
        int ret;
        
        ret = strcmp(client->addr, "unix");
	if ( ret == 0 ) {
		ret = start_unix_connection(client);
        } else {
                ret = start_inet_connection(client);
        }
        
        return ret;
}





static void handle_connection_breakage(prelude_client_t *client)
{
        client->connection_broken = 1;
        prelude_io_close(client->fd);
}




/*
 * function called back when a connection timer expire.
 */

void prelude_client_destroy(prelude_client_t *client) 
{
        prelude_io_close(client->fd);
        prelude_io_destroy(client->fd);

        free(client->addr);
        free(client);
}




prelude_client_t *prelude_client_new(const char *addr, uint16_t port) 
{
        prelude_client_t *new;

        signal(SIGPIPE, SIG_IGN);
        
        new = malloc(sizeof(*new));
        if (! new )
                return NULL;

        new->port = port;
        new->addr = strdup(addr);
        new->connection_broken = 0;
        
        new->fd = prelude_io_new();
        if ( ! new->fd ) {
                free(new);
                return NULL;        
        }
        
        return new;
}




int prelude_client_connect(prelude_client_t *client) 
{
        int ret;
        prelude_msg_t *msg;
        
        ret = do_connect(client);
        if ( ret < 0 ) {
                client->connection_broken = 1;
                return -1;
        }

        msg = prelude_option_wide_get_msg();
        if ( ! msg )
                return -1;

        ret = prelude_msg_write(msg, client->fd);
        if ( ret < 0 )
                return -1;

        ret = prelude_client_ident_send(client->fd);
        if ( ret < 0 )
                return -1;
        
        client->connection_broken = 0;
        
        return ret;
}





int prelude_client_send_msg(prelude_client_t *client, prelude_msg_t *msg) 
{
        ssize_t ret;

        if ( client->connection_broken == 1 )
                return -1;
        
        ret = prelude_msg_write(msg, client->fd);
        if ( ret < 0 ) {
                log(LOG_ERR, "couldn't send message to Manager.\n");
                handle_connection_breakage(client);
        }

        return ret;
}




void prelude_client_get_address(prelude_client_t *client, char **addr, uint16_t *port) 
{
        *addr = client->addr;
        *port = client->port;
}



ssize_t prelude_client_forward(prelude_client_t *client, prelude_io_t *src, size_t count) 
{
        return prelude_io_forward(client->fd, src, count);
}
