/*****
*
* Copyright (C) 2004 Yoann Vandoorselaere <yoann@prelude-ids.org>
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
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/utsname.h>
#include <fcntl.h>
#include <assert.h>
#include <pthread.h>
#include <errno.h>

#include <gcrypt.h>
#include <gnutls/gnutls.h>
#include <gnutls/x509.h>


#define PRELUDE_ERROR_SOURCE_DEFAULT PRELUDE_ERROR_SOURCE_CLIENT
#include "prelude-error.h"

#include "libmissing.h"
#include "idmef.h"
#include "common.h"
#include "prelude-log.h"
#include "prelude-ident.h"
#include "prelude-async.h"
#include "prelude-option.h"
#include "prelude-connection-mgr.h"
#include "prelude-client.h"
#include "prelude-timer.h"
#include "prelude-message-id.h"
#include "prelude-option-wide.h"
#include "idmef-message-write.h"
#include "idmef-additional-data.h"
#include "config-engine.h"
#include "tls-auth.h"


#define CLIENT_STATUS_STARTING 0
#define CLIENT_STATUS_STARTING_STR "starting"

#define CLIENT_STATUS_RUNNING  1
#define CLIENT_STATUS_RUNNING_STR "running"

#define CLIENT_STATUS_EXITING  2
#define CLIENT_STATUS_EXITING_STR "exiting"


/*
 * directory where analyzerID file are stored.
 */
#define IDENT_DIR PRELUDE_CONFIG_DIR "/analyzerid"


/*
 * send an heartbeat every 600 seconds by default.
 */
#define DEFAULT_HEARTBEAT_INTERVAL 600



struct prelude_client {
        
        int flags;
        int status;
        
        prelude_bool_t ignore_error;
        
        prelude_connection_capability_t capability;
        
        /*
         * information about the user/group this analyzer is running as
         */
        prelude_client_profile_t *profile;
        
        /*
         * name, analyzerid, and config file for this analyzer.
         */        
        char *md5sum;
        char *config_filename;
        idmef_analyzer_t *analyzer;
        
        prelude_connection_mgr_t *manager_list;
        prelude_timer_t heartbeat_timer;

        
        prelude_msgbuf_t *msgbuf;
        pthread_mutex_t msgbuf_lock;
        
        prelude_ident_t *unique_ident;

        prelude_option_t *config_file_opt;
        
        void (*heartbeat_cb)(prelude_client_t *client, idmef_message_t *heartbeat);
};



prelude_option_t *prelude_generic_optlist = NULL;



static int generate_md5sum(const char *filename, prelude_string_t *out)
{
        void *data;
        int ret, fd;
        size_t len, i;
        struct stat st;
        unsigned char digest[16];
        
        len = gcry_md_get_algo_dlen(GCRY_MD_MD5);
        assert(len == sizeof(digest));
        
        fd = open(filename, O_RDONLY);
        if ( fd < 0 )
                return prelude_error_from_errno(errno);
        
        ret = fstat(fd, &st);
        if ( ret < 0 || ! S_ISREG(st.st_mode) ) {
                close(fd);
                return prelude_error_from_errno(errno);
        }
        
        data = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
        if ( ! data ) {
                close(fd);
                return prelude_error_from_errno(errno);
        }
        
        gcry_md_hash_buffer(GCRY_MD_MD5, digest, data, st.st_size);
        munmap(data, st.st_size);
        close(fd);
        
        for ( i = 0; i < len; i++ ) {
                ret = prelude_string_sprintf(out, "%.2x", digest[i]);
                if ( ret < 0 )
                        return ret;
        }
        
        return 0;
}



static int add_hb_data(idmef_heartbeat_t *hb, prelude_string_t *meaning, const char *data)
{
        idmef_additional_data_t *ad;
        
        ad = idmef_heartbeat_new_additional_data(hb);
        if ( ! ad )
                return prelude_error_from_errno(errno);

        idmef_additional_data_set_meaning(ad, meaning);
        idmef_additional_data_set_string_ref(ad, data);

        return 0;
}



static const char *client_get_status(prelude_client_t *client)
{
        if ( client->status == CLIENT_STATUS_RUNNING )
                return CLIENT_STATUS_RUNNING_STR;

        else if ( client->status == CLIENT_STATUS_STARTING )
                return CLIENT_STATUS_STARTING_STR;

        else if ( client->status == CLIENT_STATUS_EXITING )
                return CLIENT_STATUS_EXITING_STR;

        abort();
}
        



static void heartbeat_expire_cb(void *data)
{
        char buf[128];
        const char *str;
        idmef_message_t *message;
        idmef_heartbeat_t *heartbeat;
        prelude_client_t *client = data;
        
        message = idmef_message_new();
        if ( ! message ) {
                log(LOG_ERR, "error creating new IDMEF message.\n");
                goto out;
        }
        
        heartbeat = idmef_message_new_heartbeat(message);
        if ( ! heartbeat ) {
                log(LOG_ERR, "error creating new IDMEF heartbeat.\n");
                goto out;
        }

        snprintf(buf, sizeof(buf), "%u", prelude_timer_get_expire(&client->heartbeat_timer));

        str = client_get_status(client);
        add_hb_data(heartbeat, prelude_string_new_constant("Analyzer status"), str);

        if ( client->md5sum )
                add_hb_data(heartbeat, prelude_string_new_constant("Analyzer md5sum"), client->md5sum);
        
        add_hb_data(heartbeat, prelude_string_new_constant("Analyzer heartbeat interval"), buf);
        
        idmef_heartbeat_set_create_time(heartbeat, idmef_time_new_from_gettimeofday());
        idmef_heartbeat_set_analyzer(heartbeat, idmef_analyzer_ref(client->analyzer));
                
        if ( client->heartbeat_cb ) {
                client->heartbeat_cb(client, message);
                goto out;
        }

        prelude_client_send_idmef(client, message);
        
  out:
        idmef_message_destroy(message);
        prelude_timer_reset(&client->heartbeat_timer);
}




static void setup_heartbeat_timer(prelude_client_t *client, int expire)
{
        prelude_timer_set_data(&client->heartbeat_timer, client);
        prelude_timer_set_expire(&client->heartbeat_timer, expire);
        prelude_timer_set_callback(&client->heartbeat_timer, heartbeat_expire_cb);
}



static int fill_client_infos(prelude_client_t *client, const char *program) 
{
        int ret;
        struct utsname uts;
        uint64_t analyzerid;
        prelude_string_t *buf;
	idmef_process_t *process;
        char filename[256], *name, *path;

        analyzerid = prelude_client_profile_get_analyzerid(client->profile);
        
	idmef_analyzer_set_analyzerid(client->analyzer, analyzerid);
        if ( uname(&uts) < 0 )
                return prelude_error_from_errno(errno);

        idmef_analyzer_set_ostype(client->analyzer, prelude_string_new_dup(uts.sysname));
	idmef_analyzer_set_osversion(client->analyzer, prelude_string_new_dup(uts.release));

        process = idmef_analyzer_new_process(client->analyzer);
        if ( ! process ) 
                return prelude_error_from_errno(errno);

	idmef_process_set_pid(process, getpid());
        
        if ( ! program )
                return 0;
        
        ret = prelude_get_file_name_and_path(program, &name, &path);
        if ( ret < 0 )
                return ret;

        idmef_process_set_name(process, prelude_string_new_nodup(name));
        idmef_process_set_path(process, prelude_string_new_nodup(path));

        buf = prelude_string_new();
        if ( ! buf )
                return prelude_error_from_errno(errno);
        
        snprintf(filename, sizeof(filename), "%s/%s", path, name);

        ret = generate_md5sum(filename, buf);
        if ( ret < 0 )
                return ret;
        
        client->md5sum = prelude_string_get_string_released(buf);
        prelude_string_destroy(buf);
        
        return (client->md5sum) ? 0 : prelude_error_from_errno(errno);
}




static int set_node_address_category(void *context, prelude_option_t *opt, const char *arg) 
{
        idmef_address_category_t category;

        category = idmef_address_category_to_numeric(arg);        
        if ( category < 0 ) 
                return -1;
        
        idmef_address_set_category(context, category);

        return 0;
}



static int get_node_address_category(void *context, prelude_option_t *opt, char *out, size_t size)
{
        idmef_address_category_t category = idmef_address_get_category(context);
        snprintf(out, size, "%s", idmef_address_category_to_string(category));
        return 0;
}



static int set_node_address_vlan_num(void *context, prelude_option_t *opt, const char *arg) 
{
        idmef_address_set_vlan_num(context, atoi(arg));
        return 0;
}



static int get_node_address_vlan_num(void *context, prelude_option_t *opt, char *out, size_t size)
{
        int32_t *num;

        num = idmef_address_get_vlan_num(context);
        if ( num )
                snprintf(out, size, "%d", *num);

        return 0;
}



static int set_node_address_vlan_name(void *context, prelude_option_t *opt, const char *arg) 
{
        idmef_address_set_vlan_name(context, prelude_string_new_dup(arg));
        return 0;
}



static int get_node_address_vlan_name(void *context, prelude_option_t *opt, char *out, size_t size)
{
        prelude_string_t *str;
        
        str = idmef_address_get_vlan_name(context);
        if ( ! str || prelude_string_is_empty(str) )
                return 0;
        
        snprintf(out, size, "%s", prelude_string_get_string(str));
        
        return 0;
}



static int set_node_address_address(void *context, prelude_option_t *opt, const char *arg) 
{       
        idmef_address_set_address(context, prelude_string_new_dup(arg));
        return 0;
}



static int get_node_address_address(void *context, prelude_option_t *opt, char *out, size_t size)
{
        prelude_string_t *str;

        str = idmef_address_get_address(context);
        if ( ! str || prelude_string_is_empty(str) )
                return 0;
        
        snprintf(out, size, "%s", prelude_string_get_string(str));

        return 0;
}



static int set_node_address_netmask(void *context, prelude_option_t *opt, const char *arg) 
{
        idmef_address_set_netmask(context, prelude_string_new_dup(arg));
        return 0;
}



static int get_node_address_netmask(void *context, prelude_option_t *opt, char *out, size_t size)
{
        prelude_string_t *str;
        
        str = idmef_address_get_netmask(context);
        if ( ! str || prelude_string_is_empty(str) )
                return 0;
                
        snprintf(out, size, "%s", prelude_string_get_string(str));

        return 0;
}



static int set_node_address(void *context, prelude_option_t *opt, const char *arg) 
{
        idmef_node_t *node;
        idmef_address_t *addr;
        prelude_client_t *ptr = context;
        
        node = idmef_analyzer_new_node(ptr->analyzer);
        if ( ! node )
                return -1;
        
        addr = idmef_node_new_address(node);
        if ( ! addr )
                return -1;

        prelude_option_new_context(opt, arg, addr);
        
        return 0;
}


static int destroy_node_address(void *context, prelude_option_t *opt)
{
        idmef_address_t *addr = context;
        
        idmef_address_destroy(addr);
        
        return 0;
}



static int set_node_category(void *context, prelude_option_t *opt, const char *arg) 
{
        idmef_node_t *node;
        idmef_node_category_t category;
        prelude_client_t *ptr = context;

        category = idmef_node_category_to_numeric(arg);
        if ( category < 0 )
                return -1;
        
        node = idmef_analyzer_new_node(ptr->analyzer);
        if ( ! node )
                return -1;
        
        idmef_node_set_category(node, category);

        return 0;
}



static int get_node_category(void *context, prelude_option_t *opt, char *out, size_t size)
{
        idmef_node_category_t category;
        prelude_client_t *client = context;
        idmef_node_t *node = idmef_analyzer_get_node(client->analyzer);
        
        if ( ! node )
                return 0;
        
        category = idmef_node_get_category(node);
        snprintf(out, size, "%s", idmef_node_category_to_string(category));
        
        return 0;
}



static int set_node_location(void *context, prelude_option_t *opt, const char *arg) 
{
        idmef_node_t *node;
        prelude_client_t *ptr = context;

        node = idmef_analyzer_new_node(ptr->analyzer);
        if ( ! node )
                return -1;

        idmef_node_set_location(node, prelude_string_new_dup(arg));

        return 0;
}



static int get_node_location(void *context, prelude_option_t *opt, char *out, size_t size)
{
        prelude_string_t *str;
        prelude_client_t *client = context;
        idmef_node_t *node = idmef_analyzer_get_node(client->analyzer);
        
        if ( ! node )
                return 0;

        str = idmef_node_get_location(node);
        if ( ! str )
                return 0;
        
        snprintf(out, size, "%s", prelude_string_get_string(str));
        
        return 0;
}



static int set_node_name(void *context, prelude_option_t *opt, const char *arg) 
{
        idmef_node_t *node;
        prelude_client_t *ptr = context;

        node = idmef_analyzer_new_node(ptr->analyzer);
        if ( ! node )
                return -1;

        idmef_node_set_name(node, prelude_string_new_dup(arg));

        return 0;
}



static int get_node_name(void *context, prelude_option_t *opt, char *out, size_t size)
{
        prelude_string_t *str;
        prelude_client_t *client = context;
        idmef_node_t *node = idmef_analyzer_get_node(client->analyzer);
        
        if ( ! node )
                return 0;

        str = idmef_node_get_name(node);
        if ( ! str )
                return 0;
        
        snprintf(out, size, "%s", prelude_string_get_string(str));
        
        return 0;
}



static int set_configuration_file(void *context, prelude_option_t *opt, const char *arg) 
{
        prelude_client_t *ptr = context;

        ptr->config_filename = strdup(arg);
        if ( ! ptr->config_filename )
                return -1;
        
        return 0;
}



static int get_analyzer_name(void *context, prelude_option_t *opt, char *out, size_t size)
{
        prelude_string_t *str;
        prelude_client_t *client = context;

        str = idmef_analyzer_get_name(client->analyzer);
        if ( ! str )
                return 0;
        
        snprintf(out, size, "%s", prelude_string_get_string(str));

        return 0;
}




static int set_analyzer_name(void *context, prelude_option_t *opt, const char *arg)
{
        prelude_string_t *str;
        prelude_client_t *ptr = context;
        
        str = idmef_analyzer_new_name(ptr->analyzer);
        if ( ! str )
                return -1;
        
        prelude_string_set_dup(str, arg);
                
        return 0;
}



static int get_manager_addr(void *context, prelude_option_t *opt, char *out, size_t size)
{
        prelude_client_t *ptr = context;

        snprintf(out, size, "%s", prelude_connection_mgr_get_connection_string(ptr->manager_list));

        return 0;
}



static int set_manager_addr(void *context, prelude_option_t *opt, const char *arg)
{
        int ret;
        prelude_client_t *ptr = context;
        
        ret = prelude_connection_mgr_set_connection_string(ptr->manager_list, arg);
        
        return (ret == 0 || ptr->ignore_error) ? 0 : -1;
}



static int set_heartbeat_interval(void *context, prelude_option_t *opt, const char *arg)
{
        prelude_client_t *ptr = context;
        
        setup_heartbeat_timer(ptr, atoi(arg));
        prelude_timer_reset(&ptr->heartbeat_timer);
        
        return 0;
}



static int get_heartbeat_interval(void *context, prelude_option_t *opt, char *buf, size_t size)
{        
        prelude_client_t *ptr = context;
        
        snprintf(buf, size, "%u", ptr->heartbeat_timer.expire);
        
        return 0;
}


static int set_ignore_error(void *context, prelude_option_t *opt, const char *arg)
{
        prelude_client_t *client = context;
        client->ignore_error = TRUE;
        return 0;
}



static int set_profile(void *context, prelude_option_t *opt, const char *arg)
{
        prelude_client_t *client = context;
        return prelude_client_profile_set_name(client->profile, arg);
}



static int setup_options(prelude_client_t *client)
{
        prelude_option_t *opt;
        prelude_option_t *root_list;

        if ( prelude_generic_optlist )
                return 0;
        
        root_list = prelude_option_add(NULL, PRELUDE_OPTION_TYPE_CLI|PRELUDE_OPTION_TYPE_CFG|
                                       PRELUDE_OPTION_TYPE_WIDE, 0,
                                       "prelude", "Prelude generic options",
                                       PRELUDE_OPTION_ARGUMENT_NONE, NULL, NULL);

        prelude_option_set_default_context(root_list, client);
        
        prelude_option_add(root_list, PRELUDE_OPTION_TYPE_CLI|PRELUDE_OPTION_TYPE_CFG, 0, "profile",
                           "Profile to use for this analyzer", PRELUDE_OPTION_ARGUMENT_REQUIRED,
                           set_profile, NULL);
        
        client->config_file_opt =
                prelude_option_add(NULL, PRELUDE_OPTION_TYPE_CLI, 0, "prelude-config-file",
                                   "Configuration file for this analyzer", PRELUDE_OPTION_ARGUMENT_REQUIRED,
                                   set_configuration_file, NULL);
        
        prelude_option_add(root_list, PRELUDE_OPTION_TYPE_CLI|PRELUDE_OPTION_TYPE_CFG
                           |PRELUDE_OPTION_TYPE_WIDE, 0, "heartbeat-interval",
                           "Number of seconds between two heartbeat",
                           PRELUDE_OPTION_ARGUMENT_REQUIRED,
                           set_heartbeat_interval, get_heartbeat_interval);
        
        if ( client->capability & PRELUDE_CONNECTION_CAPABILITY_CONNECT ) {
                 prelude_option_add(root_list, PRELUDE_OPTION_TYPE_CLI|PRELUDE_OPTION_TYPE_CFG
                                    |PRELUDE_OPTION_TYPE_WIDE, 0, "server-addr",
                                    "Address where this sensor should report to (addr:port)",
                                    PRELUDE_OPTION_ARGUMENT_REQUIRED, set_manager_addr, get_manager_addr);
        }
                
        prelude_option_add(root_list, PRELUDE_OPTION_TYPE_CLI|PRELUDE_OPTION_TYPE_CFG|PRELUDE_OPTION_TYPE_WIDE,
                           0, "analyzer-name", "Name for this analyzer",
                           PRELUDE_OPTION_ARGUMENT_REQUIRED, set_analyzer_name, get_analyzer_name);
        
        prelude_option_add(root_list, PRELUDE_OPTION_TYPE_CFG|PRELUDE_OPTION_TYPE_WIDE, 0, "node-name",
                           "Name of the equipment", PRELUDE_OPTION_ARGUMENT_REQUIRED, 
                           set_node_name, get_node_name);

        prelude_option_add(root_list, PRELUDE_OPTION_TYPE_CFG|PRELUDE_OPTION_TYPE_WIDE, 0, "node-location",
                           "Location of the equipment", PRELUDE_OPTION_ARGUMENT_REQUIRED, 
                           set_node_location, get_node_location);
        
        prelude_option_add(root_list, PRELUDE_OPTION_TYPE_CFG|PRELUDE_OPTION_TYPE_WIDE, 0, "node-category",
                           NULL, PRELUDE_OPTION_ARGUMENT_REQUIRED, set_node_category, get_node_category);
        
        opt = prelude_option_add(root_list, PRELUDE_OPTION_TYPE_CFG|PRELUDE_OPTION_TYPE_WIDE
                                 |PRELUDE_OPTION_TYPE_CONTEXT, 0, "node-address",
                                 "Network or hardware address of the equipment",
                                 PRELUDE_OPTION_ARGUMENT_REQUIRED, set_node_address, NULL);
        prelude_option_set_destroy_callback(opt, destroy_node_address);
        
        prelude_option_add(opt, PRELUDE_OPTION_TYPE_CFG|PRELUDE_OPTION_TYPE_WIDE, 0, "address",
                           "Address information", PRELUDE_OPTION_ARGUMENT_REQUIRED, 
                           set_node_address_address, get_node_address_address);

        prelude_option_add(opt, PRELUDE_OPTION_TYPE_CFG|PRELUDE_OPTION_TYPE_WIDE, 0, "netmask",
                           "Network mask for the address, if appropriate", PRELUDE_OPTION_ARGUMENT_REQUIRED, 
                           set_node_address_netmask, get_node_address_netmask);

        prelude_option_add(opt, PRELUDE_OPTION_TYPE_CFG|PRELUDE_OPTION_TYPE_WIDE, 0, "category",
                           "Type of address represented", PRELUDE_OPTION_ARGUMENT_REQUIRED, 
                           set_node_address_category, get_node_address_category);

        prelude_option_add(opt, PRELUDE_OPTION_TYPE_CFG|PRELUDE_OPTION_TYPE_WIDE, 0, "vlan-name",
                           "Name of the Virtual LAN to which the address belongs", 
                           PRELUDE_OPTION_ARGUMENT_REQUIRED, set_node_address_vlan_name,
                           get_node_address_vlan_name);

        prelude_option_add(opt, PRELUDE_OPTION_TYPE_CFG|PRELUDE_OPTION_TYPE_WIDE, 0, "vlan-num",
                           "Number of the Virtual LAN to which the address belongs", 
                           PRELUDE_OPTION_ARGUMENT_REQUIRED, set_node_address_vlan_num,
                           get_node_address_vlan_num);

        prelude_generic_optlist = root_list;
        
        return 0;
}



static int client_write_msgbuf(prelude_msgbuf_t *msgbuf, prelude_msg_t *msg)
{
        prelude_client_send_msg(prelude_msgbuf_get_data(msgbuf), msg);
        return 0;
}



static int create_heartbeat_msgbuf(prelude_client_t *client)
{
        int ret;
        
        ret = prelude_msgbuf_new(&client->msgbuf);
        if ( ret < 0 )
                return ret;

        prelude_msgbuf_set_data(client->msgbuf, client);
        prelude_msgbuf_set_callback(client->msgbuf, client_write_msgbuf);
        
        return 0;
}



static void _prelude_client_destroy(prelude_client_t *client)
{
        if ( client->profile )
                prelude_client_profile_destroy(client->profile);
        
        if ( client->md5sum )
                free(client->md5sum);
        
        if ( client->msgbuf )
                prelude_msgbuf_destroy(client->msgbuf);
        
        if ( client->analyzer )
                idmef_analyzer_destroy(client->analyzer);

        if ( client->config_filename )
                free(client->config_filename);
        
        if ( client->manager_list )
                prelude_connection_mgr_destroy(client->manager_list);
        
        if ( client->unique_ident )
                prelude_ident_destroy(client->unique_ident);
        
        free(client);
}



static int connection_mgr_event_cb(prelude_connection_mgr_t *mgr,
                                   prelude_connection_mgr_event_t event,
                                   prelude_connection_t *conn)
{
        int ret;
        prelude_io_t *fd;
        prelude_client_t *client;
        prelude_msg_t *msg = NULL;
        
        if ( event != PRELUDE_CONNECTION_MGR_EVENT_INPUT )
                return 0;

        do {
                ret = prelude_connection_recv(conn, &msg);
        } while ( ret < 0 && prelude_error_get_code(ret) == PRELUDE_ERROR_EAGAIN );

        if ( ret < 0 )
                return ret;
        
        if ( prelude_msg_get_tag(msg) != PRELUDE_MSG_OPTION_REQUEST )
                return ret;

        fd = prelude_connection_get_fd(conn);
        client = prelude_connection_mgr_get_data(mgr);
        
        return prelude_option_process_request(client, fd, msg);
}



static int connection_mgr_create(prelude_client_t *client)
{
        int ret;
        prelude_connection_mgr_t *mgr;
        
        ret = prelude_connection_mgr_new(&mgr, client->profile, client->capability);
        if ( ret < 0 )
                return ret;

        prelude_connection_mgr_set_data(mgr, client);
        prelude_connection_mgr_set_flags(mgr, PRELUDE_CONNECTION_MGR_FLAGS_RECONNECT);
        prelude_connection_mgr_set_event_handler(mgr, PRELUDE_CONNECTION_MGR_EVENT_INPUT, connection_mgr_event_cb);

        client->manager_list = mgr;

        return 0;
}




/**
 * prelude_client_new:
 * @client: Pointer to a client object to initialize.
 * @capability: capability of this client.
 * @profile: Default profile name for this analyzer.
 * @config: Generic configuration file for this analyzer.
 * @argc: argument count provided on the analyzer command line.
 * @argv: array of argument provided on the analyzer command line.
 *
 * This function initialize the @client object, reading generic
 * option from the @config configuration file and the provided
 * @argv array of arguments.
 *
 * Returns: 0 on success or a negative value if an error occur.
 */
int prelude_client_new(prelude_client_t **client,
                       prelude_connection_capability_t capability,
                       const char *profile, const char *config,
                       int *argc, char **argv)
{
        int ret;
        char filename[256];
        prelude_client_t *new;
        prelude_option_t *opt;
        prelude_option_warning_t old_warnings;
        
        new = calloc(1, sizeof(*new));
        if ( ! new )
                return prelude_error_from_errno(errno);
        
        new->analyzer = idmef_analyzer_new();
        if ( ! new->analyzer ) {
                _prelude_client_destroy(new);
                return prelude_error_from_errno(errno);
        }
        
        new->capability = capability;
        pthread_mutex_init(&new->msgbuf_lock, NULL);
        new->config_filename = config ? strdup(config) : NULL;
        
        setup_options(new);
        set_analyzer_name(new, NULL, profile);

        ret = _prelude_client_profile_new(&new->profile);
        if ( ret < 0 ) {
                _prelude_client_destroy(new);
                return ret;
        }

        prelude_client_profile_set_name(new->profile, profile);
                
        ret = prelude_ident_new(&new->unique_ident);
        if ( ret < 0 ) {
                _prelude_client_destroy(new);
                return ret;
        }

        prelude_option_set_warnings(0, &old_warnings);
        opt = prelude_option_add(prelude_generic_optlist, PRELUDE_OPTION_TYPE_CLI, 0,
                                 "ignore-startup-error", NULL, PRELUDE_OPTION_ARGUMENT_NONE,
                                 set_ignore_error, NULL);
        
        ret = prelude_option_parse_arguments(new, opt, NULL, argc, argv);
        if ( ret < 0 ) {
                _prelude_client_destroy(new);
                return ret;
        }
        
        setup_heartbeat_timer(new, DEFAULT_HEARTBEAT_INTERVAL);
        prelude_timer_init(&new->heartbeat_timer);

        if ( new->capability & PRELUDE_CONNECTION_CAPABILITY_CONNECT ) {
                ret = connection_mgr_create(new);
                if ( ret < 0 ) {
                        _prelude_client_destroy(new);
                        return ret;
                }
        }
        
        ret = prelude_option_parse_arguments(new, prelude_generic_optlist, config, argc, argv);        
        if ( ret < 0 ) {
                _prelude_client_destroy(new);
                return ret;
        }
        
        prelude_option_set_warnings(old_warnings, NULL);
        
        ret = _prelude_client_profile_init(new->profile);
        if ( ret < 0 ) {
                _prelude_client_destroy(new);
                return ret;
        }
        
        
        /*
         * need to be done after option parsing, so that there is no error
         * when prelude-getopt see standalone option.
         */
        prelude_option_destroy(opt);
        prelude_client_profile_get_backup_dirname(new->profile, filename, sizeof(filename));

        ret = access(filename, W_OK);
        if ( ret < 0 && ! new->ignore_error ) {
                _prelude_client_destroy(new);
                return prelude_error(PRELUDE_ERROR_BACKUP_DIRECTORY);
        }
        
        ret = fill_client_infos(new, argv ? argv[0] : NULL);
        if ( ret < 0 ) {
                _prelude_client_destroy(new);
                return ret;
        }
        
        ret = create_heartbeat_msgbuf(new);
        if ( ret < 0 ) {
                _prelude_client_destroy(new);
                return ret;
        }
        
        *client = new;
        
        return 0;
}



/**
 * prelude_client_start:
 * @client: Pointer to a client object to initialize.
 *
 * This function start the @client object, triggering
 * a connection from the client to it's server if any were
 * specified, and sending the initial heartbeat.
 *
 * Returns: 0 on success, -1 if an error occured.
 */
int prelude_client_start(prelude_client_t *client)
{
        int ret;
        
        if ( client->manager_list ) {
                ret = prelude_connection_mgr_init(client->manager_list);                
                if ( ret < 0 ) {
                        log(LOG_ERR, "Initiating connection to the manager has failed.\n");
                        return ret;
                }
        }
        
        if ( client->manager_list || client->heartbeat_cb ) {
                client->status = CLIENT_STATUS_STARTING;
                heartbeat_expire_cb(client);
                client->status = CLIENT_STATUS_RUNNING;
        }
        
        return 0;
}



/**
 * prelude_client_get_analyzer:
 * @client: Pointer to a #prelude_client_t object.
 *
 * Provide access to the #idmef_analyzer_t object associated to @client.
 * This analyzer object is sent along with every alerts and heartbeats emited
 * by this client. The analyzer object is created by prelude_client_init().
 *
 * Returns: the #idmef_analyzer_t object associated with @client.
 */
idmef_analyzer_t *prelude_client_get_analyzer(prelude_client_t *client)
{
        return client->analyzer;
}




/**
 * prelude_client_send_msg:
 * @client: Pointer to a #prelude_client_t object.
 * @msg: pointer to a message that @client should send.
 *
 * Send @msg to the peers @client is communicating with.
 *
 * The message will be sent asynchronously if @PRELUDE_CLIENT_FLAGS_ASYNC_SEND
 * was set using prelude_client_set_flags() in which case the caller should
 * not call prelude_msg_destroy() on @msg.
 */
void prelude_client_send_msg(prelude_client_t *client, prelude_msg_t *msg)
{
        if ( client->flags & PRELUDE_CLIENT_FLAGS_ASYNC_SEND )
                prelude_connection_mgr_broadcast_async(client->manager_list, msg);
        else 
                prelude_connection_mgr_broadcast(client->manager_list, msg);
}



/**
 * prelude_client_send_idmef:
 * @client: Pointer to a #prelude_client_t object.
 * @msg: pointer to an IDMEF message to be sent to @client peers.
 *
 * Send @msg to the peers @client is communicating with.
 *
 * The message will be sent asynchronously if @PRELUDE_CLIENT_FLAGS_ASYNC_SEND
 * was set using prelude_client_set_flags() in which case the caller should
 * not call prelude_msg_destroy() on @msg.
 */
void prelude_client_send_idmef(prelude_client_t *client, idmef_message_t *msg)
{
        uint64_t ident;
        idmef_alert_t *alert;
        idmef_heartbeat_t *heartbeat;
        
        /*
         * we need to hold a lock since asynchronous heartbeat
         * could write the message buffer at the same time we do.
         */
        pthread_mutex_lock(&client->msgbuf_lock);

        if ( idmef_message_get_type(msg) == IDMEF_MESSAGE_TYPE_ALERT ) {
                alert = idmef_message_get_alert(msg);
                
                if ( ! idmef_alert_get_messageid(alert) ) {
                        ident = prelude_ident_inc(client->unique_ident);
                        idmef_alert_set_messageid(alert, ident);
                }
        } else {       
                heartbeat = idmef_message_get_heartbeat(msg);

                if ( ! idmef_heartbeat_get_messageid(heartbeat) ) {
                        ident = prelude_ident_inc(client->unique_ident);
                        idmef_heartbeat_set_messageid(heartbeat, ident);
                }
        }
        
        idmef_message_write(msg, client->msgbuf);
        prelude_msgbuf_mark_end(client->msgbuf);

        pthread_mutex_unlock(&client->msgbuf_lock);
}



/**
 * prelude_client_get_manager_list:
 * @client: pointer to a #prelude_client_t object.
 *
 * Return a pointer to the #prelude_connection_mgr_t object used by @client
 * to send messages.
 *
 * Returns: a pointer to a #prelude_connection_mgr_t object.
 */
prelude_connection_mgr_t *prelude_client_get_manager_list(prelude_client_t *client)
{
        return client->manager_list;
}



/**
 * prelude_client_set_manager_list:
 * @client: pointer to a #prelude_client_t object.
 * @mgrlist: pointer to a #prelude_client_mgr_t object.
 *
 * Use this function in order to set your own list of peer that @client
 * should send message too. This might be usefull in case you don't want
 * this to be automated by prelude_client_init().
 */
void prelude_client_set_manager_list(prelude_client_t *client, prelude_connection_mgr_t *mgrlist)
{        
        client->manager_list = mgrlist;
}



/**
 * prelude_client_set_heartbeat_cb:
 * @client: pointer to a #prelude_client_t object.
 * @cb: pointer to a function handling heartbeat sending.
 *
 * Use if you want to override the default function used to
 * automatically send heartbeat to @client peers.
 */
void prelude_client_set_heartbeat_cb(prelude_client_t *client,
                                     void (*cb)(prelude_client_t *client, idmef_message_t *hb))
{
        client->heartbeat_cb = cb;
}



/**
 * prelude_client_destroy:
 * @client: Pointer on a client object.
 * @status: Exit status for the client.
 *
 * Destroy @client, and send an heartbeat containing the 'exiting'
 * status in case @status is PRELUDE_CLIENT_EXIT_STATUS_SUCCESS.
 *
 * This is useful for analyzer expected to be running periodically,
 * and that shouldn't be treated as behaving anormaly in case no
 * heartbeat is sent.
 *
 * Please note that your are not supposed to run this function
 * from a signal handler.
 */
void prelude_client_destroy(prelude_client_t *client, prelude_client_exit_status_t status)
{        
        if ( status == PRELUDE_CLIENT_EXIT_STATUS_SUCCESS ) {
                prelude_timer_lock_critical_region();
                
                client->status = CLIENT_STATUS_EXITING;
                heartbeat_expire_cb(client);
                prelude_timer_destroy(&client->heartbeat_timer);

                prelude_timer_unlock_critical_region();
        }

        prelude_client_profile_destroy(client->profile);
        
        if ( client->md5sum )
                free(client->md5sum);
        
        if ( client->msgbuf )
                prelude_msgbuf_destroy(client->msgbuf);
        
        if ( client->profile )
                free(client->profile);

        if ( client->analyzer )
                idmef_analyzer_destroy(client->analyzer);
        
        if ( client->config_filename )
                free(client->config_filename);

        if ( client->manager_list )
                prelude_connection_mgr_destroy(client->manager_list);

        if ( client->unique_ident )
                prelude_ident_destroy(client->unique_ident);
        
        free(client);
}




/**
 * prelude_client_set_flags:
 * @client: Pointer on a #prelude_client_t object.
 * @flags: Or'd list of flags used by @client.
 *
 * Set specific flags in the @client structure.
 * This function can be called anytime after the creation of the
 * @client object.
 *
 * When settings asynchronous flags such as #PRELUDE_CLIENT_FLAGS_ASYNC_SEND
 * or #PRELUDE_CLIENT_FLAGS_ASYNC_TIMER, be carefull to call
 * prelude_client_set_flags() in the same process you want to use the
 * asynchronous API from. Threads aren't copied accross fork().
 *
 * Returns: 0 if setting @flags succeed, -1 otherwise.
 */
int prelude_client_set_flags(prelude_client_t *client, prelude_client_flags_t flags)
{
        int ret = 0;

        client->flags = flags;
                
        if ( flags & PRELUDE_CLIENT_FLAGS_ASYNC_TIMER ) {
                ret = prelude_async_init();
                prelude_async_set_flags(PRELUDE_ASYNC_TIMER);       
        }
        
        else if ( flags & PRELUDE_CLIENT_FLAGS_ASYNC_SEND )
                ret = prelude_async_init();

        return ret;
}




/**
 * prelude_client_get_flags:
 * @client: Pointer on a #prelude_client_t object.
 *
 * Get flags set through prelude_client_set_flags().
 *
 * Returns: an or'ed list of #prelude_client_flags_t.
 */
prelude_client_flags_t prelude_client_get_flags(prelude_client_t *client)
{
        return client->flags;
}



/**
 * prelude_client_get_capability:
 * @client: Pointer on a #prelude_client_t object.
 *
 * Returns: @client capability as set with prelude_client_set_capability()
 */
prelude_connection_capability_t prelude_client_get_capability(prelude_client_t *client)
{
        return client->capability;
}



/**
 * prelude_client_get_config_filename:
 * @client: pointer on a #prelude_client_t object.
 *
 * Return the filename used to store @client configuration.
 * This filename is originally set by the prelude_async_init() function.
 *
 * Returns: a pointer to @client configuration filename.
 */
const char *prelude_client_get_config_filename(prelude_client_t *client)
{
        return client->config_filename;
}


prelude_ident_t *prelude_client_get_unique_ident(prelude_client_t *client)
{
        return client->unique_ident;
}



prelude_client_profile_t *prelude_client_get_profile(prelude_client_t *client)
{
        return client->profile;
}



prelude_bool_t prelude_client_is_setup_needed(prelude_client_t *client, prelude_error_t err)
{
        prelude_error_code_t code = prelude_error_get_code(err);
        
        return ( code == PRELUDE_ERROR_BACKUP_DIRECTORY    ||
                 code == PRELUDE_ERROR_ANALYZERID_FILE     ||
                 code == PRELUDE_ERROR_ANALYZERID_PARSE    ||
                 code == PRELUDE_ERROR_TLS_KEY             ||
                 code == PRELUDE_ERROR_TLS_CERTIFICATE     ||
                 code == PRELUDE_ERROR_TLS_CERTIFICATE_PARSE ) ? TRUE : FALSE;
}



void prelude_client_print_setup_error(prelude_client_t *client) 
{
        if ( client->capability & PRELUDE_CONNECTION_CAPABILITY_CONNECT ) {
                log(LOG_INFO,
                    "\nBasic file configuration does not exist. Please run :\n"
                    "prelude-adduser register %s <manager address> --uid %d --gid %d\n"
                    "program to setup the analyzer.\n\n"
                    
                    "Be aware that you should replace the \"<manager address>\" argument with\n"
                    "the server address this analyzer is reporting to as argument.\n"
                    "\"prelude-adduser\" should be called for each configured server address.\n\n",
                    prelude_client_profile_get_name(client->profile),
                    prelude_client_profile_get_uid(client->profile),
                    prelude_client_profile_get_gid(client->profile));
                
        } else {
                log(LOG_INFO,
                    "\nBasic file configuration does not exist. Please run :\n"
                    "prelude-adduser add %s --uid %d --gid %d\n\n",
                    prelude_client_profile_get_name(client->profile),
                    prelude_client_profile_get_uid(client->profile),
                    prelude_client_profile_get_gid(client->profile));
        }
}
