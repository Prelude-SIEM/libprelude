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
#include <string.h>
#include <sys/time.h>
#include <inttypes.h>
#include <unistd.h>

#include "config.h"

#include "list.h"
#include "config-engine.h"
#include "plugin-common.h"
#include "common.h"

#ifdef HAVE_SSL
#include <openssl/ssl.h>
#endif

#include "prelude-io.h"
#include "prelude-message.h"
#include "prelude-client-mgr.h"
#include "sensor.h"

#include "ssl.h"
#include "prelude-getopt.h"
#include "prelude-async.h"


static int launch = 1;
static prelude_optlist_t *opts;
static prelude_client_mgr_t *manager_list = NULL;



static void print_help(const char *optarg) 
{
        launch = 0;
        fprintf(stderr, "\nSensor options\n");
        prelude_option_print(opts);
}




static void setup_manager_addr(const char *optarg) 
{        
        manager_list = prelude_client_mgr_new("manager", strdup(optarg));
}




static int parse_argument(const char *filename, int argc, char **argv) 
{
        int ret;

        opts = prelude_option_new();
        if ( ! opts )
                return -1;

        prelude_option_add(opts, 'a', "manager-addr",
                           "Address where manager is listening", required_argument, setup_manager_addr);
        prelude_option_add(opts, 'h', "help",
                           "Print this help", no_argument, print_help);
        
        ret = prelude_option_parse_arguments(opts, filename, argc, argv);
        if ( ret < 0 )
                return -1;

        prelude_option_destroy(opts);

        if ( ! manager_list ) {
                log(LOG_INFO,
                    "No Manager were configured. You need to setup a Manager for this Sensor\n"
                    "to report events. Please use the \"manager-addr\" entry in the Sensor\n"
                    "config file or the -a and eventually -p command line options.\n");
                return -1;
        }
        
        return 0;
}



/**
 * libprelude_sensor_init:
 * @filename: Configuration file of the calling sensor.
 * @argc: Argument count provided to the calling sensor.
 * @argv: Argument array provided to the calling sensor.
 *
 * Init the sensor library,
 * connect to the manager.
 *
 * Returns: 0 on success, -1 on error.
 */
int prelude_sensor_init(const char *filename, int argc, char **argv)
{
        int ret;
        
        ret = parse_argument(filename, argc, argv);
        if ( ret < 0 )
                return -1;
        
        if ( ! launch )
                return 0;

        ret = prelude_async_init();
        if ( ret < 0 ) {
                log(LOG_ERR, "couldn't initialize asynchronous subsystem.\n");
                return -1;
        }
        
        return ret;
}



void prelude_sensor_send_alert(prelude_msg_t *msg) 
{
        prelude_client_mgr_broadcast_msg(manager_list, msg);
}












