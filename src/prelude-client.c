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
#include "prelude-client.h"
#include "sensor.h"


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
        
        ret = prelude_auth_send(client->fd, client->addr);
        if ( ret < 0 ) {
                log(LOG_INFO,
                    "\nPlaintext authentication failed. Use the \"manager-adduser\"\n"
                    "program on the Manager host together with the \"sensor-adduser\"\n"
                    "program on the Sensor host to create an username for this Sensor.\n");
                return -1;
        }

        prelude_io_set_sys_io(client->fd, sock);

        return 0;
}



#ifdef HAVE_SSL

static int handle_ssl_connection(prelude_client_t *client, int sock) 
{
        int ret;
        SSL *ssl;
        static int ssl_initialized = 0;
        
        if ( ! ssl_initialized ) {
                ret = ssl_init_client();
                if ( ret < 0 )
                        return -1;
                
                ssl_initialized = 1;
        }
                
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




static int check_ssl_option(const char *optbuf) 
{
#ifdef HAVE_SSL
        if ( strstr(optbuf, "ssl=supported;") ) {
                log(LOG_INFO, "- Manager support SSL, initializing SSL subsystem.\n");
                return 1;
        } else 
                log(LOG_INFO, "\t- Manager do not support SSL, not using encryption.\n");
#endif
        
        return 0;
}




/*
 * Report server should send us a string looking like :
 * xdr=xxx; ssl=xxx;
 *
 * with xxx having the value "supported" or "unsupported"
 *
 * The string have to be terminated with a '\n\0'.
 */
static int setup_inet_connection(prelude_io_t *fd, int *use_ssl) 
{
        const char *ssl;
        int len = 0, ret;
        char *ptr, buf[1024];
        
        ret = prelude_io_read_delimited(fd, (void **) &ptr);
        if ( ret < 0 ) {
                log(LOG_ERR, "error waiting for Manager config string.\n");
                return -1;
        }
        
        *use_ssl = check_ssl_option(ptr);
       
        free(ptr);
        
        ssl = (*use_ssl == 1) ? "yes" : "no";
        len = snprintf(buf, sizeof(buf), "use_ssl=%s;\n", ssl);
        
	ret = prelude_io_write_delimited(fd, buf, ++len);
	if ( ret != len ) {
                log(LOG_ERR, "couldn't write config string to Manager.\n");
                return -1;
        }
        
        return 0;
}




static int start_inet_connection(prelude_client_t *client) 
{
        int ret = -1, use_ssl, sock;
        
        sock = inet_connect(client->addr, client->port);
        if ( sock < 0 )
                return -1;

        prelude_io_set_sys_io(client->fd, sock);
        
        ret = setup_inet_connection(client->fd, &use_ssl);
        if ( ret < 0 ) {
                close(sock);
                return -1;
        }

        if ( ! use_ssl ) 
                ret = handle_plaintext_connection(client, sock);
#ifdef HAVE_SSL        
        else {
                ret = handle_ssl_connection(client, sock);
        } 
#endif
        
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
        if ( ret == 0 && (msg = prelude_option_wide_get_msg() ) ) 
                prelude_msg_write(msg, client->fd);

        client->connection_broken = (ret < 0) ? 1 : 0;
        
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













