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


#define CAPABILITY_SEND (PRELUDE_CLIENT_CAPABILITY_SEND_IDMEF|PRELUDE_CLIENT_CAPABILITY_SEND_ADMIN|PRELUDE_CLIENT_CAPABILITY_SEND_CM)



struct prelude_client {
        
        int flags;
        int initial_hb;
        
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
        if ( ret < 0 )
                return NULL;
        
        data = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
        if ( ! data ) {
                log(LOG_ERR, "could not mmap %s.\n", filename);
                return NULL;
        }
        
        gcry_md_hash_buffer(GCRY_MD_MD5, digest, data, st.st_size);
        munmap(data, st.st_size);
        close(fd);
        
        for ( i = ret = 0; i < len; i++ ) 
                ret += snprintf(out + ret, sizeof(out) - ret, "%.2x", digest[i]);
        
        return strdup(out);
}



static int add_hb_data(idmef_heartbeat_t *hb, idmef_string_t *meaning, idmef_data_t *data)
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



static void heartbeat_expire_cb(void *data)
{
        char buf[128];
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
        
        add_hb_data(heartbeat, idmef_string_new_constant("Analyzer status"),
                    idmef_string_new_ref(client->initial_hb ? "starting" : "running"));

        if ( client->md5sum )
                add_hb_data(heartbeat, idmef_string_new_constant("Analyzer md5sum"),
                            idmef_string_new_ref(client->md5sum));

        add_hb_data(heartbeat, idmef_string_new_constant("Analyzer heartbeat interval"),
                    idmef_string_new_ref(buf));
        
        idmef_heartbeat_set_create_time(heartbeat, idmef_time_new_gettimeofday());
        idmef_heartbeat_set_analyzer(heartbeat, idmef_analyzer_ref(client->analyzer));
                
        if ( client->heartbeat_cb ) {
                client->heartbeat_cb(client, message);
                goto out;
        }

        idmef_write_message(client->msgbuf, message);
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
        char *name, *path;
        struct utsname uts;
	idmef_process_t *process;
        
	idmef_analyzer_set_analyzerid(client->analyzer, client->analyzerid);
        
        if ( uname(&uts) < 0 ) {
                log(LOG_ERR, "uname returned an error.\n");
                return -1;
        }

        idmef_analyzer_set_ostype(client->analyzer, idmef_string_new_dup(uts.sysname));
	idmef_analyzer_set_osversion(client->analyzer, idmef_string_new_dup(uts.release));

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
        
        if ( name ) 
                idmef_process_set_name(process, idmef_string_new_ref(name));

        if ( path )
                idmef_process_set_path(process, idmef_string_new_ref(path));
        
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



static int set_node_address_vlan_num(void *context, prelude_option_t *opt, const char *arg) 
{
        idmef_address_set_vlan_num(context, atoi(arg));
        return 0;
}



static int set_node_address_vlan_name(void *context, prelude_option_t *opt, const char *arg) 
{
        idmef_address_set_vlan_name(context, idmef_string_new_dup(arg));
        return 0;
}



static int set_node_address_address(void *context, prelude_option_t *opt, const char *arg) 
{        
        idmef_address_set_address(context, idmef_string_new_dup(arg));
        return 0;
}




static int set_node_address_netmask(void *context, prelude_option_t *opt, const char *arg) 
{
        idmef_address_set_netmask(context, idmef_string_new_dup(arg));
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

        prelude_option_instance_new(opt, arg, addr);
        
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



static int set_node_location(void *context, prelude_option_t *opt, const char *arg) 
{
        idmef_node_t *node;
        prelude_client_t *ptr = context;

        node = idmef_analyzer_new_node(ptr->analyzer);
        if ( ! node )
                return -1;

        idmef_node_set_location(node, idmef_string_new_dup(arg));

        return 0;
}



static int set_node_name(void *context, prelude_option_t *opt, const char *arg) 
{
        idmef_node_t *node;
        prelude_client_t *ptr = context;

        node = idmef_analyzer_new_node(ptr->analyzer);
        if ( ! node )
                return -1;

        idmef_node_set_name(node, idmef_string_new_dup(arg));

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



static int set_manager_addr(void *context, prelude_option_t *opt, const char *arg)
{
        prelude_client_t *ptr = context;

        ptr->manager_list = prelude_connection_mgr_new(ptr, arg);

        return (ptr->manager_list) ? 0 : -1;
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



static int setup_options(prelude_client_t *client, int argc, char **argv)
{
        int ret;
        int old_flags;
        prelude_option_t *opt;
        
        prelude_option_add(NULL, CLI_HOOK|CFG_HOOK|WIDE_HOOK, 0, "heartbeat-interval",
                           "Number of minutes between two heartbeat", required_argument,
                           set_heartbeat_interval, get_heartbeat_interval);

        
        if ( client->capability & CAPABILITY_SEND ) {
                 prelude_option_add(NULL, CLI_HOOK|CFG_HOOK|WIDE_HOOK, 0, "manager-addr",
                                    "Address where manager is listening (addr:port)",
                                    required_argument, set_manager_addr, NULL);
        }
        
        opt = prelude_option_add(NULL, CLI_HOOK, 0, "config-file",
                                 "Configuration file for this analyzer", required_argument,
                                 set_configuration_file, NULL);

        prelude_option_set_warnings(0, &old_flags);
        ret = prelude_option_parse_arguments((void *) &client, opt, NULL, argc, argv);
        prelude_option_set_warnings(old_flags, NULL);
        
        if ( ret < 0 )
                return -1;
        
        opt = prelude_option_add(NULL, CLI_HOOK|CFG_HOOK, 0, "analyzer-name",
                                 "Name for this analyzer", required_argument, set_name, NULL);
        
        prelude_option_set_warnings(0, &old_flags);
        ret = prelude_option_parse_arguments((void *) &client, opt, NULL, argc, argv);
        prelude_option_set_warnings(old_flags, NULL);
        
        if ( ret < 0 )
                return -1;
        
        prelude_option_add(NULL, CFG_HOOK|WIDE_HOOK, 0, "node-name",
                           NULL, required_argument, set_node_name, NULL);

        prelude_option_add(NULL, CFG_HOOK|WIDE_HOOK, 0, "node-location",
                           NULL, required_argument, set_node_location, NULL);
        
        prelude_option_add(NULL, CFG_HOOK|WIDE_HOOK, 0, "node-category",
                           NULL, required_argument, set_node_category, NULL);
        
        opt = prelude_option_add(NULL, CFG_HOOK|WIDE_HOOK|ALLOW_MULTIPLE_CALL, 0, "node-address",
                                 NULL, required_argument, set_node_address, NULL);
        
        prelude_option_add(opt, CFG_HOOK|WIDE_HOOK, 0, "address",
                           NULL, required_argument, set_node_address_address, NULL);

        prelude_option_add(opt, CFG_HOOK|WIDE_HOOK, 0, "netmask",
                           NULL, required_argument, set_node_address_netmask, NULL);

        prelude_option_add(opt, CFG_HOOK|WIDE_HOOK, 0, "category",
                           NULL, required_argument, set_node_address_category, NULL);

        prelude_option_add(opt, CFG_HOOK|WIDE_HOOK, 0, "vlan-name",
                           NULL, required_argument, set_node_address_vlan_name, NULL);

        prelude_option_add(opt, CFG_HOOK|WIDE_HOOK, 0, "vlan-num",
                           NULL, required_argument, set_node_address_vlan_num, NULL);

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
        
        return new;
}



int prelude_client_init(prelude_client_t *new, const char *sname, const char *config, int argc, char **argv)
{
        int ret;
        char filename[256];
        
        new->name = strdup(sname);
        new->config_filename = config ? strdup(config) : NULL;
        
        if ( argv )
                new->md5sum = generate_md5sum(argv[0]);
        
        new->unique_ident = prelude_ident_new();
        if ( ! new->unique_ident ) {
                log(LOG_ERR, "memory exhausted.\n");
                return -1;
        }
                
        ret = setup_options(new, argc, argv);
        if ( ret < 0 )
                return -1;

        new->credentials = tls_auth_init(new);
        if ( ! new->credentials ) {
                file_error(new);
                return -1;
        }

        prelude_client_get_backup_filename(new, filename, sizeof(filename));
        ret = access(filename, W_OK);
        if ( ret < 0 ) {
                file_error(new);
                return -1;
        }
        
        ret = prelude_client_ident_init(new, &new->analyzerid);
        if ( ret < 0 ) {
                file_error(new);
                return -1;
        }
        
        setup_heartbeat_timer(new, DEFAULT_HEARTBEAT_INTERVAL);
        timer_init(&new->heartbeat_timer);
        
        ret = prelude_option_parse_arguments(new, NULL, new->config_filename, argc, argv);
        if ( ret == prelude_option_end )
                return -1;
        
        if ( ret == prelude_option_error ) {
                log(LOG_INFO, "%s: error processing sensor options.\n", new->config_filename);
                idmef_analyzer_destroy(new->analyzer);
                return -1;
        }
        
        ret = fill_client_infos(new, argv ? argv[0] : NULL);
        if ( ret < 0 )
                return -1;

        ret = create_heartbeat_msgbuf(new);
        if ( ret < 0 )
                return -1;

        if ( new->manager_list || new->heartbeat_cb ) {
                new->initial_hb = 1;
                heartbeat_expire_cb(new);
                new->initial_hb = 0;
        }
        
        return 0;
}




idmef_analyzer_t *prelude_client_get_analyzer(prelude_client_t *client)
{
        return client->analyzer;
}



void prelude_client_set_analyzerid(prelude_client_t *client, uint64_t analyzerid)
{
        client->analyzerid = analyzerid;
}



uint64_t prelude_client_get_analyzerid(prelude_client_t *client)
{
        return client->analyzerid;
}



const char *prelude_client_get_name(prelude_client_t *client)
{
        return client->name;
}



void prelude_client_send_msg(prelude_client_t *client, prelude_msg_t *msg)
{
        if ( client->flags & PRELUDE_CLIENT_ASYNC_SEND )
                prelude_connection_mgr_broadcast_async(client->manager_list, msg);
        else 
                prelude_connection_mgr_broadcast(client->manager_list, msg);
}



void prelude_client_set_uid(prelude_client_t *client, uid_t uid)
{
        client->uid = uid;
}



uid_t prelude_client_get_uid(prelude_client_t *client)
{
        return client->uid;
}



void prelude_client_set_gid(prelude_client_t *client, gid_t gid)
{
        client->gid = gid;
}



gid_t prelude_client_get_gid(prelude_client_t *client)
{
        return client->gid;
}



prelude_connection_mgr_t *prelude_client_get_manager_list(prelude_client_t *client)
{
        return client->manager_list;
}



void prelude_client_set_manager_list(prelude_client_t *client, prelude_connection_mgr_t *mgrlist)
{
        client->manager_list = mgrlist;
}



void prelude_client_set_heartbeat_cb(prelude_client_t *client, void (*cb)(prelude_client_t *client, idmef_message_t *hb))
{
        client->heartbeat_cb = cb;
}



void prelude_client_set_name(prelude_client_t *client, const char *name)
{
        if ( client->name )
                free(client->name);

        client->name = strdup(name);
}




void prelude_client_destroy(prelude_client_t *client)
{        
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



int prelude_client_set_flags(prelude_client_t *client, int flags)
{
        int ret = 0;

        client->flags = flags;
        
        if ( flags & PRELUDE_CLIENT_ASYNC_SEND )
                ret = prelude_async_init();
        
        if ( flags & PRELUDE_CLIENT_ASYNC_TIMER ) {
                ret = prelude_async_init();
                prelude_async_set_flags(PRELUDE_ASYNC_TIMER);       
        }

        return ret;
}




int prelude_client_get_flags(prelude_client_t *client)
{
        return client->flags;
}



void prelude_client_set_capability(prelude_client_t *client, prelude_client_capability_t capability)
{
        client->capability = capability;
}



prelude_client_capability_t prelude_client_get_capability(prelude_client_t *client)
{
        return client->capability;
}



void prelude_client_get_ident_filename(prelude_client_t *client, char *buf, size_t size) 
{
        snprintf(buf, size, IDENT_DIR "/%s", client->name);
}



void prelude_client_get_tls_key_filename(prelude_client_t *client, char *buf, size_t size) 
{
        snprintf(buf, size, TLS_KEY_DIR "/%s", client->name);
}


void prelude_client_get_tls_server_ca_cert_filename(prelude_client_t *client, char *buf, size_t size) 
{
        snprintf(buf, size, TLS_SERVER_CERT_DIR "/%s.ca", client->name);
}



void prelude_client_get_tls_server_trusted_cert_filename(prelude_client_t *client, char *buf, size_t size) 
{
        snprintf(buf, size, TLS_SERVER_CERT_DIR "/%s.trusted", client->name);
}



void prelude_client_get_tls_server_keycert_filename(prelude_client_t *client, char *buf, size_t size) 
{
        snprintf(buf, size, TLS_SERVER_CERT_DIR "/%s.keycrt", client->name);
}



void prelude_client_get_tls_client_trusted_cert_filename(prelude_client_t *client, char *buf, size_t size) 
{
        snprintf(buf, size, TLS_CLIENT_CERT_DIR "/%s.trusted", client->name);
}



void prelude_client_get_tls_client_keycert_filename(prelude_client_t *client, char *buf, size_t size) 
{
        snprintf(buf, size, TLS_CLIENT_CERT_DIR "/%s.keycrt", client->name);
}



void prelude_client_get_backup_filename(prelude_client_t *client, char *buf, size_t size) 
{
        snprintf(buf, size, PRELUDE_SPOOL_DIR "/%s", client->name);
}



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
