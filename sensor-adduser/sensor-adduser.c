/*****
*
* Copyright (C) 2001, 2002 Yoann Vandoorselaere <yoann@mandrakesoft.com>
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
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "common.h"
#include "prelude-message-id.h"
#include "prelude-path.h"
#include "prelude-log.h"
#include "prelude-io.h"
#include "prelude-auth.h"
#include "prelude-message.h"
#include "prelude-getopt.h"
#include "plaintext.h"
#include "ssl-register.h"
#include "config.h"


static uint16_t port = 5553;
static int uid_set = 0;
static uid_t sensor_uid;
static const char *addr = NULL;
static const char *sensor_name = NULL;


static int set_sensor_name(const char *optarg) 
{
        sensor_name = optarg;
        prelude_set_program_name(optarg);
        return prelude_option_success;
}



static int set_sensor_uid(const char *optarg) 
{
        uid_set = 1;
        sensor_uid = atoi(optarg);
        prelude_set_program_userid(sensor_uid);
        return prelude_option_success;
}



static int set_manager_addr(const char *optarg) 
{
        char *ptr;
        
        addr = strdup(optarg);
        if ( ! addr ) {
                log(LOG_ERR, "couldn't duplicate string.\n");
                return prelude_option_error;
        }

        ptr = strchr(addr, ':');
        if ( ptr ) {
                *ptr++ = '\0';
                port = atoi(ptr);
        } 
        
        return prelude_option_success;
}



static int print_help(const char *optarg) 
{
        prelude_option_print(CLI_HOOK, 50);
        return prelude_option_end;
}




static prelude_io_t *connect_manager(struct in_addr in, uint16_t port) 
{
        int ret, sock;
        prelude_io_t *fd;
        struct sockaddr_in daddr;

        daddr.sin_family = AF_INET;
        daddr.sin_port = htons(port);

        daddr.sin_addr = in;

        sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if ( sock < 0) {
                fprintf(stderr, "error creating socket.\n");
                return NULL;
        }

        ret = connect(sock, (struct sockaddr *) &daddr, sizeof(daddr));
        if ( ret < 0 ) {
                fprintf(stderr, "couldn't connect to %s.\n", addr);
                close(sock);
                return NULL;
        }

        fd = prelude_io_new();
        if ( ! fd ) {
                fprintf(stderr, "error creating an IO object.\n");
                close(sock);
                return NULL;
        }

        fprintf(stderr, "Succeeded.\n");
        
        prelude_io_set_sys_io(fd, sock);

        return fd;
}




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

        do {
                status = prelude_msg_read(&msg, fd);
                if ( status == prelude_msg_error ) {
                        log(LOG_ERR, "an error occured while reading the Manager configuration msg.\n");
                        return -1;
                }
                
        } while ( status != prelude_msg_finished );

        if ( prelude_msg_get_tag(msg) != PRELUDE_MSG_AUTH ) {
                log(LOG_ERR, "Manager didn't sent us any authentication message.\n");
                return -1;
        }

        while ( (ret = prelude_msg_get(msg, &tag, &dlen, &buf)) > 0  ) {
                
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



static int ask_one_shot_password(char **buf) 
{
        int ret;
        char *pass, *confirm;

        fprintf(stderr,
                "\n\nPlease use the one-shot password provided by the \"manager-adduser\" program.\n\n");
        
        pass = getpass("Enter registration one shot password : ");
        if ( ! pass )
                return -1;

        pass = strdup(pass);
        if ( ! pass )
                return -1;
        
        confirm = getpass("Please confirm one shot passwrd : ");
        if ( ! confirm )
                return -1;

        ret = strcmp(pass, confirm);
        memset(confirm, 0, strlen(confirm));

        if ( ret == 0 ) {
                *buf = pass;
                return 0;
        }

        fprintf(stderr, "Bad password, they don't match.\n");
        
        memset(pass, 0, strlen(pass));
        free(pass);
        
        return ask_one_shot_password(buf);
}




static int handle_argument(int argc, char **argv) 
{
        int ret;
        
        prelude_option_add(NULL, CLI_HOOK, 'h', "help", "Print this help",
                           no_argument, print_help, NULL);
        
        prelude_option_add(NULL, CLI_HOOK, 's', "sensorname", "Sensor name",
                           required_argument, set_sensor_name, NULL);
        
        prelude_option_add(NULL, CLI_HOOK, 'u', "uid", "Sensor user ID",
                           required_argument, set_sensor_uid, NULL);

        prelude_option_add(NULL, CLI_HOOK, 'm', "manager-addr", "Manager address",
                           required_argument, set_manager_addr, NULL);
        
        ret = prelude_option_parse_arguments(NULL, NULL, argc, argv);
        if ( ret == prelude_option_end || ret == prelude_option_error )
                exit(ret);
        
        if ( ! addr ) {
                log(LOG_INFO, "No manager address specified.\n");
                return -1;
        }

        if ( ! sensor_name ) {
                log(LOG_INFO, "No sensor name specified.\n");
                return -1;
        }

        if ( ! uid_set ) {
                log(LOG_INFO, "No sensor UID specified.\n");
                return -1;
        }
        
        return 0;
}




static int setup_sensor_files(void) 
{
        int fd, ret;
        char buf[256];

        prelude_get_backup_filename(buf, sizeof(buf));

        /*
         * The user may have changed permission, and we don't want
         * to be vulnerable to a symlink attack anyway. 
         */
        fd = prelude_open_persistant_tmpfile(buf, 0, S_IRUSR|S_IWUSR);
        if ( fd < 0 ) {
                log(LOG_ERR, "couldn't create %s.\n", buf);
                return -1;
        }

        ret = fchown(fd, sensor_uid, 0);
        if ( ret < 0 ) {
                log(LOG_ERR, "couldn't chown %s to UID %d.\n", buf, sensor_uid);
                return -1;
        }
        
        return 0;
}





int main(int argc, char **argv) 
{
        int ret;
        prelude_io_t *fd;
        struct in_addr in;
        char buf[256], *pass;
        int have_ssl = 0, have_plaintext = 0;

        ret = handle_argument(argc, argv);
        if ( ret < 0 )
                return -1;
        
        fprintf(stderr,
                "\n\nNow please start \"manager-adduser\" on the Manager host where\n"
                "you wish to add the new user.\n\n"

                "Please remember that you should call \"sensor-adduser\" for each configured\n"
                "Manager entry.\n\n"
                
                "Press enter when done.\n");

        fgets(buf, sizeof(buf), stdin);

        
        ret = ask_one_shot_password(&pass);
        if ( ret < 0 )
                return -1;
        
	fprintf(stderr, "connecting to Manager host (%s:%d)... ", addr, port);
        
        ret = prelude_resolve_addr(addr, &in);
        if ( ret < 0 ) {
                fprintf(stderr, "couldn't resolve %s.\n", addr);
                return -1;
        }
        
        fd = connect_manager(in, port);
        if ( ! fd ) 
                return -1;

        ret = get_manager_setup(fd, &have_ssl, &have_plaintext);
        if ( ret < 0 )
                return -1;

#ifdef HAVE_SSL
        if ( have_ssl && strcmp(inet_ntoa(in), "127.0.0.1") != 0 ) {
                ret = ssl_add_certificate(fd, pass, strlen(pass), sensor_uid);
                goto end;
        }
#endif

        if ( have_plaintext ) {
                ret = create_plaintext_user_account(fd, pass, sensor_uid);
                goto end;
        } else {
                log(LOG_INFO, "couldn't agree on a protocol to use.\n");
                ret = -1;
        }

        exit(1);
        
 end:
        if ( ret < 0 )
                exit(ret);

        exit(setup_sensor_files());
}






