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

#include <gcrypt.h>

#include "libmissing.h"
#include "idmef.h"
#include "timer.h"
#include "common.h"
#include "client-ident.h"
#include "prelude-log.h"
#include "prelude-ident.h"
#include "prelude-async.h"
#include "prelude-getopt.h"
#include "prelude-connection-mgr.h"
#include "prelude-client.h"
#include "idmef-message-write.h"
#include "config-engine.h"
#include "tls-auth.h"


#define CLIENT_STATUS_STARTING 0
#define CLIENT_STATUS_STARTING_STR "starting"

#define CLIENT_STATUS_RUNNING  1
#define CLIENT_STATUS_RUNNING_STR "running"

#define CLIENT_STATUS_EXITING  2
#define CLIENT_STATUS_EXITING_STR "exiting"


/*
 * directory where TLS private keys file are stored.
 */
#define TLS_KEY_DIR PRELUDE_CONFIG_DIR "/tls/keys"

/*
 * directory where TLS client certificate file are stored.
 */
#define TLS_CLIENT_CERT_DIR PRELUDE_CONFIG_DIR "/tls/client"

/*
 * directory where TLS server certificate file are stored.
 */
#define TLS_SERVER_CERT_DIR PRELUDE_CONFIG_DIR "/tls/server"


/*
 * directory where analyzerID file are stored.
 */
#define IDENT_DIR PRELUDE_CONFIG_DIR "/analyzerid"


/*
 * send an heartbeat every hours.
 */
#define DEFAULT_HEARTBEAT_INTERVAL 60 * 60


#define CAPABILITY_SEND (PRELUDE_CLIENT_CAPABILITY_SEND_IDMEF| \
                         PRELUDE_CLIENT_CAPABILITY_SEND_ADMIN| \
                         PRELUDE_CLIENT_CAPABILITY_SEND_CM)



struct prelude_client {
        
        int flags;
        int status;
        
        prelude_bool_t ignore_error;
        
        prelude_client_capability_t capability;
        
        /*
         * information about the user/group this analyzer is running as
         */
        uid_t uid;
        gid_t gid;

        /*
         * name, analyzerid, and config file for this analyzer.
         */
        char *name;
        char *md5sum;
        
        uint64_t analyzerid;
        char *config_filename;
        
        idmef_analyzer_t *analyzer;

        void *credentials;
        prelude_connection_mgr_t *manager_list;
        prelude_timer_t heartbeat_timer;
        
        prelude_msgbuf_t *msgbuf;
        prelude_ident_t *unique_ident;

        prelude_option_t *analyzer_name_opt, *config_file_opt;
        
        void (*heartbeat_cb)(prelude_client_t *client, idmef_message_t *heartbeat);
};




static char *generate_md5sum(const char *filename)
{
        void *data;
        int ret, fd;
        size_t len, i;
        struct stat st;
        unsigned char out[33], digest[16];
        
        len = gcry_md_get_algo_dlen(GCRY_MD_MD5);
        assert(len == sizeof(digest));
        
        fd = open(filename, O_RDONLY);
        if ( fd < 0 )
                return NULL;
        
        ret = fstat(fd, &st);
        if ( ret < 0 || ! S_ISREG(st.st_mode) ) {
                close(fd);
                return NULL;
        }
        
        data = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
        if ( ! data ) {
                log(LOG_ERR, "could not mmap %s.\n", filename);
                close(fd);
                return NULL;
        }
        
        gcry_md_hash_buffer(GCRY_MD_MD5, digest, data, st.st_size);
        munmap(data, st.st_size);
        close(fd);
        
        for ( i = ret = 0; i < len; i++ ) 
                ret += snprintf(out + ret, sizeof(out) - ret, "%.2x", digest[i]);
        
        return strdup(out);
}



static int add_hb_data(idmef_heartbeat_t *hb, prelude_string_t *meaning, idmef_data_t *data)
{
        idmef_additional_data_t *ad;
        
        ad = idmef_heartbeat_new_additional_data(hb);
        if ( ! ad ) {
                log(LOG_ERR, "error creating new IDMEF data.\n");
                return -1;
        }

        idmef_additional_data_set_data(ad, data);
        idmef_additional_data_set_meaning(ad, meaning);
        idmef_additional_data_set_type(ad, IDMEF_ADDITIONAL_DATA_TYPE_STRING);

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

        snprintf(buf, sizeof(buf), "%u", timer_expire(&client->heartbeat_timer) / 60);

        str = client_get_status(client);
        add_hb_data(heartbeat, prelude_string_new_constant("Analyzer status"),
                    idmef_data_new_ref(str, strlen(str) + 1));

        if ( client->md5sum )
                add_hb_data(heartbeat, prelude_string_new_constant("Analyzer md5sum"),
                            idmef_data_new_ref(client->md5sum, strlen(client->md5sum) + 1));
        
        add_hb_data(heartbeat, prelude_string_new_constant("Analyzer heartbeat interval"),
                    idmef_data_new_ref(buf, strlen(buf) + 1));
        
        idmef_heartbeat_set_create_time(heartbeat, idmef_time_new_gettimeofday());
        idmef_heartbeat_set_analyzer(heartbeat, idmef_analyzer_ref(client->analyzer));
                
        if ( client->heartbeat_cb ) {
                client->heartbeat_cb(client, message);
                goto out;
        }

        idmef_message_write(message, client->msgbuf);
        prelude_msgbuf_mark_end(client->msgbuf);

  out:
        idmef_message_destroy(message);
        timer_reset(&client->heartbeat_timer);
}




static void setup_heartbeat_timer(prelude_client_t *client, int expire)
{        
        timer_set_data(&client->heartbeat_timer, client);
        timer_set_expire(&client->heartbeat_timer, expire * 60);
        timer_set_callback(&client->heartbeat_timer, heartbeat_expire_cb);
}



static int fill_client_infos(prelude_client_t *client, const char *program) 
{
        int ret;
        struct utsname uts;
	idmef_process_t *process;
        char filename[256], *name, *path;
        
	idmef_analyzer_set_analyzerid(client->analyzer, client->analyzerid);
        
        if ( uname(&uts) < 0 ) {
                log(LOG_ERR, "uname returned an error.\n");
                return -1;
        }

        idmef_analyzer_set_ostype(client->analyzer, prelude_string_new_dup(uts.sysname));
	idmef_analyzer_set_osversion(client->analyzer, prelude_string_new_dup(uts.release));

        process = idmef_analyzer_new_process(client->analyzer);
        if ( ! process ) {
                log(LOG_ERR, "cannot create process field of analyzer\n");
                return -1;
        }

	idmef_process_set_pid(process, getpid());

        if ( ! program )
                return 0;
        
        ret = prelude_get_file_name_and_path(program, &name, &path);
        if ( ret < 0 )
                return -1;
        
        idmef_process_set_name(process, prelude_string_new_ref(name));
        idmef_process_set_path(process, prelude_string_new_ref(path));
        
        snprintf(filename, sizeof(filename), "%s/%s", path, name);
        client->md5sum = generate_md5sum(filename);
        
        return 0;
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
        snprintf(out, size, "%d", idmef_address_get_vlan_num(context));
        return 0;
}



static int set_node_address_vlan_name(void *context, prelude_option_t *opt, const char *arg) 
{
        idmef_address_set_vlan_name(context, prelude_string_new_dup(arg));
        return 0;
}



static int get_node_address_vlan_name(void *context, prelude_option_t *opt, char *out, size_t size)
{
        snprintf(out, size, "%s", prelude_string_get_string(idmef_address_get_vlan_name(context)));
        return 0;
}



static int set_node_address_address(void *context, prelude_option_t *opt, const char *arg) 
{       
        idmef_address_set_address(context, prelude_string_new_dup(arg));
        return 0;
}



static int get_node_address_address(void *context, prelude_option_t *opt, char *out, size_t size)
{
        snprintf(out, size, "%s", prelude_string_get_string(idmef_address_get_address(context)));
        return 0;
}



static int set_node_address_netmask(void *context, prelude_option_t *opt, const char *arg) 
{
        idmef_address_set_netmask(context, prelude_string_new_dup(arg));
        return 0;
}



static int get_node_address_netmask(void *context, prelude_option_t *opt, char *out, size_t size)
{
        snprintf(out, size, "%s", prelude_string_get_string(idmef_address_get_netmask(context)));
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
                return -1;
        
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
        prelude_client_t *client = context;
        idmef_node_t *node = idmef_analyzer_get_node(client->analyzer);
        
        if ( ! node )
                return -1;
        
        snprintf(out, size, "%s", prelude_string_get_string(idmef_node_get_location(node)));
        
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
        prelude_client_t *client = context;
        idmef_node_t *node = idmef_analyzer_get_node(client->analyzer);
        
        if ( ! node )
                return -1;
        
        snprintf(out, size, "%s", prelude_string_get_string(idmef_node_get_name(node)));
        
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




static int set_name(void *context, prelude_option_t *opt, const char *arg)
{
        prelude_client_t *ptr = context;

        if ( ptr->name )
                free(ptr->name);
        
        ptr->name = strdup(arg);
        if ( ! ptr->name )
                return -1;
        
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
        timer_reset(&ptr->heartbeat_timer);
        
        return 0;
}



static int get_heartbeat_interval(void *context, prelude_option_t *opt, char *buf, size_t size)
{        
        prelude_client_t *ptr = context;
        
        snprintf(buf, size, "%u", ptr->heartbeat_timer.expire / 60);
        
        return 0;
}


static int set_ignore_error(void *context, prelude_option_t *opt, const char *arg)
{
        prelude_client_t *client = context;
        client->ignore_error = TRUE;
        return 0;
}


static int setup_options(prelude_client_t *client)
{
        prelude_option_t *opt;
                
        prelude_option_add(NULL, CLI_HOOK|CFG_HOOK|WIDE_HOOK, 0, "heartbeat-interval",
                           "Number of minutes between two heartbeat", required_argument,
                           set_heartbeat_interval, get_heartbeat_interval);
        
        if ( client->capability & CAPABILITY_SEND ) {
                 prelude_option_add(NULL, CLI_HOOK|CFG_HOOK|WIDE_HOOK, 0, "manager-addr",
                                    "Address where manager is listening (addr:port)",
                                    required_argument, set_manager_addr, get_manager_addr);
        }
        
        client->config_file_opt =
                prelude_option_add(NULL, CLI_HOOK, 0, "config-file",
                                   "Configuration file for this analyzer", required_argument,
                                   set_configuration_file, NULL);
        
        client->analyzer_name_opt =
                prelude_option_add(NULL, CLI_HOOK|CFG_HOOK, 0, "analyzer-name",
                                   "Name for this analyzer", required_argument, set_name, NULL);
        
        prelude_option_add(NULL, CFG_HOOK|WIDE_HOOK, 0, "node-name",
                           "Name of the equipment", required_argument, 
                           set_node_name, get_node_name);

        prelude_option_add(NULL, CFG_HOOK|WIDE_HOOK, 0, "node-location",
                           "Location of the equipment", required_argument, 
                           set_node_location, get_node_location);
        
        prelude_option_add(NULL, CFG_HOOK|WIDE_HOOK, 0, "node-category",
                           NULL, required_argument, set_node_category, 
                           get_node_category);
        
        opt = prelude_option_add(NULL, CFG_HOOK|WIDE_HOOK|HAVE_CONTEXT, 0, "node-address",
                                 "Network or hardware address of the equipment", required_argument, 
                                 set_node_address, NULL);
        prelude_option_set_destroy_callback(opt, destroy_node_address);
        
        prelude_option_add(opt, CFG_HOOK|WIDE_HOOK, 0, "address",
                           "Address information", required_argument, 
                           set_node_address_address, get_node_address_address);

        prelude_option_add(opt, CFG_HOOK|WIDE_HOOK, 0, "netmask",
                           "Network mask for the address, if appropriate", required_argument, 
                           set_node_address_netmask, get_node_address_netmask);

        prelude_option_add(opt, CFG_HOOK|WIDE_HOOK, 0, "category",
                           "Type of address represented", required_argument, 
                           set_node_address_category, get_node_address_category);

        prelude_option_add(opt, CFG_HOOK|WIDE_HOOK, 0, "vlan-name",
                           "Name of the Virtual LAN to which the address belongs", 
                           required_argument, set_node_address_vlan_name, get_node_address_vlan_name);

        prelude_option_add(opt, CFG_HOOK|WIDE_HOOK, 0, "vlan-num",
                           "Number of the Virtual LAN to which the address belongs", 
                           required_argument, set_node_address_vlan_num, get_node_address_vlan_num);

        return 0;
}


static int get_standalone_option(prelude_client_t *client, prelude_option_t *opt, int argc, char **argv)
{
        int ret;
        
        ret = prelude_option_parse_arguments(client, opt, NULL, argc, argv);
        if ( ret < 0 )
                return -1;

        prelude_option_destroy(opt);
        
        return 0;
}


static int create_heartbeat_msgbuf(prelude_client_t *client)
{
        client->msgbuf = prelude_msgbuf_new(client);
        if ( ! client->msgbuf )
                return -1;

        return 0;
}




static void file_error(prelude_client_t *client) 
{       
        log(LOG_INFO, "\nBasic file configuration does not exist. Please run :\n"
            "prelude-adduser register %s <manager address> --uid %d --gid %d\n"
            "program on the analyzer host to setup this analyzer.\n\n"
            
            "Be aware that you should also replace the <manager address> argument\n"
            "with the address of the server that your analyzer is trying to connect to.\n"
            "\"prelude-adduser\" should be called for each configured manager address.\n\n",
            prelude_client_get_name(client), prelude_client_get_uid(client),
            prelude_client_get_gid(client));
}



/**
 * prelude_client_new:
 * @capability: set of capability for this client.
 *
 * Create a new #prelude_client_t object with the given capability.
 *
 * Returns: a #prelude_client_t object, or NULL if an error occured.
 */
prelude_client_t *prelude_client_new(prelude_client_capability_t capability)
{
        prelude_client_t *new;
        
        new = calloc(1, sizeof(*new));
        if ( ! new ) {
                log(LOG_ERR, "memory exhausted.\n");
                return NULL;
        }
        
        new->analyzer = idmef_analyzer_new();
        if ( ! new->analyzer ) {
                log(LOG_ERR, "memory exhausted.\n");
                free(new);
                return NULL;
        }
        
        new->uid = getuid();
        new->gid = getgid();
        new->capability = capability;

        setup_options(new);
        
        return new;
}



/**
 * prelude_client_init:
 * @client: Pointer to a client object to initialize.
 * @sname: Default name of the analyzer associated with this client.
 * @config: Generic configuration file for this analyzer.
 * @argc: argument count provided on the analyzer command line.
 * @argv: array of argument provided on the analyzer command line.
 *
 * This function initialize the @client object, reading generic
 * option from the @config configuration file and the provided
 * @argv array of arguments.
 *
 * Returns: 0 on success, -1 if an error occured.
 */
int prelude_client_init(prelude_client_t *client, const char *sname, const char *config, int argc, char **argv)
{
        int ret, old_flags;
        char filename[256];
        prelude_option_t *opt;
        
        client->name = strdup(sname);
        client->config_filename = config ? strdup(config) : NULL;
 
        client->unique_ident = prelude_ident_new();
        if ( ! client->unique_ident ) {
                log(LOG_ERR, "memory exhausted.\n");
                return -1;
        }
        
        prelude_option_set_warnings(0, &old_flags);

        opt = prelude_option_add(NULL, CLI_HOOK, 0, "ignore-startup-error",
                                 NULL, no_argument, set_ignore_error, NULL);
        ret = prelude_option_parse_arguments(client, opt, NULL, argc, argv);
        
        ret = get_standalone_option(client, client->config_file_opt, argc, argv);
        if ( ret < 0 )
                return -1;

        ret = get_standalone_option(client, client->analyzer_name_opt, argc, argv);
        if ( ret < 0 )
                return -1;
        
        prelude_option_set_warnings(old_flags, NULL);
        
        client->credentials = tls_auth_init(client);
        if ( ! client->credentials && ! client->ignore_error ) {
                file_error(client);
                return -1;
        }
        
        prelude_client_get_backup_filename(client, filename, sizeof(filename));
        ret = access(filename, W_OK);
        if ( ret < 0 && ! client->ignore_error ) {
                file_error(client);
                return -1;
        }
        
        ret = prelude_client_ident_init(client, &client->analyzerid);
        if ( ret < 0 && ! client->ignore_error ) {
                file_error(client);
                return -1;
        }
        
        client->manager_list = prelude_connection_mgr_new(client);
        if ( ! client->manager_list )
                return -1;
        
        setup_heartbeat_timer(client, DEFAULT_HEARTBEAT_INTERVAL);
        timer_init(&client->heartbeat_timer);
        
        ret = prelude_option_parse_arguments(client, NULL, client->config_filename, argc, argv);
        if ( ret == prelude_option_end )
                return -1;
        
        if ( ret == prelude_option_error ) {
                log(LOG_INFO, "%s: error processing sensor options.\n", client->config_filename);
                idmef_analyzer_destroy(client->analyzer);
                return -1;
        }
        
        ret = fill_client_infos(client, argv ? argv[0] : NULL);
        if ( ret < 0 )
                return -1;

        ret = create_heartbeat_msgbuf(client);
        if ( ret < 0 )
                return -1;
        
        ret = prelude_connection_mgr_init(client->manager_list);
        if ( ret < 0 )
                return -1;
        
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
 * prelude_client_set_analyzerid:
 * @client: Pointer to a #prelude_client_t object.
 * @analyzerid: an unique identity accross the whole environement of analyzer.
 *
 * Provide the ability to manually set @client analyzerid. This unique identity
 * should be unique accross the whole environement of analyzer. The analyzerid
 * is normally allocated when registering an analyzer.
 */
void prelude_client_set_analyzerid(prelude_client_t *client, uint64_t analyzerid)
{
        client->analyzerid = analyzerid;
}



/**
 * prelude_client_get_analyzerid:
 * @client: Pointer to a #prelude_client_t object.
 *
 * Get the unique and permanent analyzerid associated with @client.
 *
 * Returns: a 64bits integer representing the unique identity of @client.
 */
uint64_t prelude_client_get_analyzerid(prelude_client_t *client)
{
        return client->analyzerid;
}



/**
 * prelude_client_get_name:
 * @client: Pointer to a #prelude_client_t object.
 *
 * Get the analyzer name associated with @client. This name might
 * have been set by prelude_client_init() or prelude_client_set_name()
 * functions.
 *
 * Returns: a pointer to the name of @client.
 */
const char *prelude_client_get_name(prelude_client_t *client)
{
        return client->name;
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
 * prelude_client_set_uid:
 * @client: Pointer to a #prelude_client_t object.
 * @uid: UID used by @client.
 *
 * Analyzer UID and GID are part of the different filenames used to
 * store analyzer data. This is required since it is not impossible
 * for two instance of the same analyzer (same name) to use different
 * access privileges.
 *
 * In case your analyzer drop privilege after a call to prelude_client_init(),
 * you might use this function to override the UID gathered using getuid().
 */
void prelude_client_set_uid(prelude_client_t *client, uid_t uid)
{
        client->uid = uid;
}



/**
 * prelude_client_get_uid:
 * @client: pointer to a #prelude_client_t object.
 *
 * Returns: the UID used by @client.
 */
uid_t prelude_client_get_uid(prelude_client_t *client)
{
        return client->uid;
}



/**
 * prelude_client_set_gid:
 * @client: Pointer to a #prelude_client_t object.
 * @gid: GID used by @client.
 *
 * Analyzer UID and GID are part of the different filenames used to
 * store analyzer data. This is required since it is not impossible
 * for two instance of the same analyzer (same name) to use different
 * access privileges.
 *
 * In case your analyzer drop privileges after a call to prelude_client_init()
 * you might use this function to override the GID gathered using getuid().
 */
void prelude_client_set_gid(prelude_client_t *client, gid_t gid)
{
        client->gid = gid;
}



/**
 * prelude_client_get_gid:
 * @client: pointer to a #prelude_client_t object.
 *
 * Returns: the GID used by @client.
 */
gid_t prelude_client_get_gid(prelude_client_t *client)
{
        return client->gid;
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
 * prelude_client_set_name:
 * @client: Pointer to a #prelude_client_t object.
 * @name: public name for @client.
 *
 * This function might be used to override @client name provided
 * using prelude_client_init(). Analyzer are not expected to use
 * this function directly.
 */
void prelude_client_set_name(prelude_client_t *client, const char *name)
{
        if ( client->name )
                free(client->name);

        client->name = strdup(name);
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
                
                timer_destroy(&client->heartbeat_timer);
                client->status = CLIENT_STATUS_EXITING;
                heartbeat_expire_cb(client);
        }
        
        if ( client->name )
                free(client->name);

        if ( client->analyzer )
                idmef_analyzer_destroy(client->analyzer);

        if ( client->config_filename )
                free(client->config_filename);

        if ( client->manager_list )
                prelude_connection_mgr_destroy(client->manager_list);

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
 * prelude_client_set_capability:
 * @client: Pointer on a #prelude_client_t object.
 * @capability: Or'ed list of capability the client have.
 *
 * Set @capability for @client. @client might change depending
 * on the capability you set.
 */
void prelude_client_set_capability(prelude_client_t *client, prelude_client_capability_t capability)
{
        client->capability = capability;
}



/**
 * prelude_client_get_capability:
 * @client: Pointer on a #prelude_client_t object.
 *
 * Returns: @client capability as set with prelude_client_set_capability()
 */
prelude_client_capability_t prelude_client_get_capability(prelude_client_t *client)
{
        return client->capability;
}



/**
 * prelude_client_get_ident_filename:
 * @client: pointer on a #prelude_client_t object.
 * @buf: buffer to write the returned filename to.
 * @size: size of @buf.
 *
 * Return the filename used to store @client unique and permanent analyzer ident.
 */
void prelude_client_get_ident_filename(prelude_client_t *client, char *buf, size_t size) 
{
        snprintf(buf, size, IDENT_DIR "/%s", client->name);
}




/**
 * prelude_client_get_tls_key_filename:
 * @client: pointer on a #prelude_client_t object.
 * @buf: buffer to write the returned filename to.
 * @size: size of @buf.
 *
 * Return the filename used to store @client private key.
 */
void prelude_client_get_tls_key_filename(prelude_client_t *client, char *buf, size_t size) 
{
        snprintf(buf, size, TLS_KEY_DIR "/%s", client->name);
}




/**
 * prelude_client_get_tls_server_filename:
 * @client: pointer on a #prelude_client_t object.
 * @buf: buffer to write the returned filename to.
 * @size: size of @buf.
 *
 * Return the filename used to store @client related CA certificate.
 * This only apply to @client receiving connection from analyzer (server).
 */
void prelude_client_get_tls_server_ca_cert_filename(prelude_client_t *client, char *buf, size_t size) 
{
        snprintf(buf, size, TLS_SERVER_CERT_DIR "/%s.ca", client->name);
}




/**
 * prelude_client_get_tls_server_trusted_cert_filename:
 * @client: pointer on a #prelude_client_t object.
 * @buf: buffer to write the returned filename to.
 * @size: size of @buf.
 *
 * Return the filename used to store certificate that this @client trust.
 * This only apply to @client receiving connection from analyzer (server).
 */
void prelude_client_get_tls_server_trusted_cert_filename(prelude_client_t *client, char *buf, size_t size) 
{
        snprintf(buf, size, TLS_SERVER_CERT_DIR "/%s.trusted", client->name);
}




/**
 * prelude_client_get_tls_server_keycert_filename:
 * @client: pointer on a #prelude_client_t object.
 * @buf: buffer to write the returned filename to.
 * @size: size of @buf.
 *
 * Return the filename used to store certificate for @client private key.
 * This only apply to @client receiving connection from analyzer (server).
 */
void prelude_client_get_tls_server_keycert_filename(prelude_client_t *client, char *buf, size_t size) 
{
        snprintf(buf, size, TLS_SERVER_CERT_DIR "/%s.keycrt", client->name);
}




/**
 * prelude_client_get_tls_client_trusted_cert_filename:
 * @client: pointer on a #prelude_client_t object.
 * @buf: buffer to write the returned filename to.
 * @size: size of @buf.
 *
 * Return the filename used to store peers public certificates that @client trust.
 * This only apply to client connecting to a peer.
 */
void prelude_client_get_tls_client_trusted_cert_filename(prelude_client_t *client, char *buf, size_t size) 
{
        snprintf(buf, size, TLS_CLIENT_CERT_DIR "/%s.trusted", client->name);
}




/**
 * prelude_client_get_tls_client_keycert_filename:
 * @client: pointer on a #prelude_client_t object.
 * @buf: buffer to write the returned filename to.
 * @size: size of @buf.
 *
 * Return the filename used to store public certificate for @client private key.
 * This only apply to client connecting to a peer.
 */
void prelude_client_get_tls_client_keycert_filename(prelude_client_t *client, char *buf, size_t size) 
{
        snprintf(buf, size, TLS_CLIENT_CERT_DIR "/%s.keycrt", client->name);
}




/**
 * prelude_client_get_backup_filename:
 * @client: pointer on a #prelude_client_t object.
 * @buf: buffer to write the returned filename to.
 * @size: size of @buf.
 *
 * Return the filename where message sent by @client will be stored,
 * in case writing the message to the peer fail.
 */
void prelude_client_get_backup_filename(prelude_client_t *client, char *buf, size_t size) 
{
        snprintf(buf, size, PRELUDE_SPOOL_DIR "/%s", client->name);
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



void *prelude_client_get_credentials(prelude_client_t *client)
{
        return client->credentials;
}



void prelude_client_installation_error(prelude_client_t *client) 
{
        if ( client->capability & CAPABILITY_SEND ) {
                log(LOG_INFO,
                    "\nBasic file configuration does not exist. Please run :\n"
                    "prelude-adduser register %s <manager address> --uid %d --gid %d\n"
                    "program to setup the analyzer.\n\n"
                    
                    "Be aware that you should replace the \"<manager address>\" argument with\n"
                    "the server address this analyzer is reporting to as argument.\n"
                    "\"prelude-adduser\" should be called for each configured server address.\n\n",
                    client->name, client->uid, client->gid);
                
        } else {
                log(LOG_INFO,
                    "\nBasic file configuration does not exist. Please run :\n"
                    "prelude-adduser add %s --uid %d --gid %d\n"
                    "program on the sensor host to create an account for this sensor.\n\n",
                    client->name, client->uid, client->gid);
        }
}
