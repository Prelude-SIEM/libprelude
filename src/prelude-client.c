/*****
*
* Copyright (C) 2004-2005,2006,2007 PreludeIDS Technologies. All Rights Reserved.
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
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <assert.h>
#include <errno.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/utsname.h>

#include <gcrypt.h>
#include <gnutls/gnutls.h>
#include <gnutls/x509.h>

#include "glthread/lock.h"

#define PRELUDE_ERROR_SOURCE_DEFAULT PRELUDE_ERROR_SOURCE_CLIENT
#include "prelude-error.h"

#include "idmef.h"
#include "common.h"
#include "prelude-log.h"
#include "prelude-ident.h"
#include "prelude-async.h"
#include "prelude-option.h"
#include "prelude-connection-pool.h"
#include "prelude-client.h"
#include "prelude-timer.h"
#include "prelude-message-id.h"
#include "prelude-option-wide.h"
#include "idmef-message-write.h"
#include "idmef-additional-data.h"
#include "config-engine.h"
#include "tls-auth.h"


#define CLIENT_STATUS_NEED_INIT 0
#define CLIENT_STATUS_INIT_DONE 1

#define CLIENT_STATUS_STARTING 2
#define CLIENT_STATUS_STARTING_STR "starting"

#define CLIENT_STATUS_RUNNING  3
#define CLIENT_STATUS_RUNNING_STR "running"

#define CLIENT_STATUS_EXITING  4
#define CLIENT_STATUS_EXITING_STR "exiting"


/*
 * directory where analyzerID file are stored.
 */
#define IDENT_DIR PRELUDE_CONFIG_DIR "/analyzerid"


/*
 * send an heartbeat every 600 seconds by default.
 */
#define DEFAULT_HEARTBEAT_INTERVAL 600



typedef struct {
        prelude_client_t *client;
        idmef_address_t *addr;
        idmef_address_t *idmef_addr;
} node_address_data_t;


struct prelude_client {

        int refcount;
        int flags;
        int status;

        prelude_connection_permission_t permission;

        /*
         * information about the user/group this analyzer is running as
         */
        prelude_client_profile_t *profile;

        /*
         * name, analyzerid, and config file for this analyzer.
         */
        char *sha1sum;
        char *config_filename;
        prelude_bool_t config_external;

        idmef_analyzer_t *analyzer;
        idmef_analyzer_t *_analyzer_copy;

        prelude_connection_pool_t *cpool;
        prelude_timer_t heartbeat_timer;


        prelude_msgbuf_t *msgbuf;
        gl_lock_t msgbuf_lock;

        prelude_ident_t *unique_ident;

        prelude_option_t *config_file_opt;

        void (*heartbeat_cb)(prelude_client_t *client, idmef_message_t *heartbeat);
};



extern int _prelude_internal_argc;
extern char *_prelude_internal_argv[1024];
extern int _prelude_connection_keepalive_time;
extern int _prelude_connection_keepalive_probes;
extern int _prelude_connection_keepalive_intvl;
prelude_option_t *_prelude_generic_optlist = NULL;



static int client_write_cb(prelude_msgbuf_t *msgbuf, prelude_msg_t *msg)
{
        prelude_client_send_msg(prelude_msgbuf_get_data(msgbuf), msg);
        return 0;
}



static int generate_sha1sum(const char *filename, prelude_string_t *out)
{
        int ret;
        size_t len, i;
        unsigned char digest[20], *data;

        ret = _prelude_load_file(filename, &data, &len);
        if ( ret < 0 )
                return ret;

        gcry_md_hash_buffer(GCRY_MD_SHA1, digest, data, len);
        _prelude_unload_file(data, len);

        len = gcry_md_get_algo_dlen(GCRY_MD_SHA1);
        assert(len == sizeof(digest));

        for ( i = 0; i < len; i++ ) {
                ret = prelude_string_sprintf(out, "%.2x", digest[i]);
                if ( ret < 0 )
                        return ret;
        }

        return 0;
}



static int add_hb_data(idmef_heartbeat_t *hb, prelude_string_t *meaning, const char *data)
{
        int ret;
        idmef_additional_data_t *ad;

        ret = idmef_heartbeat_new_additional_data(hb, &ad, -1);
        if ( ret < 0 )
                return ret;

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



static void gen_heartbeat(prelude_client_t *client)
{
        int ret;
        idmef_time_t *time;
        prelude_string_t *str;
        idmef_message_t *message;
        idmef_heartbeat_t *heartbeat;

        prelude_log_debug(2, "running heartbeat callback.\n");

        ret = idmef_message_new(&message);
        if ( ret < 0 ) {
                prelude_perror(ret, "error creating new IDMEF message");
                goto out;
        }

        ret = idmef_message_new_heartbeat(message, &heartbeat);
        if ( ret < 0 ) {
                prelude_perror(ret, "error creating new IDMEF heartbeat.\n");
                goto out;
        }

        idmef_heartbeat_set_heartbeat_interval(heartbeat, prelude_timer_get_expire(&client->heartbeat_timer));

        ret = prelude_string_new_constant(&str, "Analyzer status");
        if ( ret < 0 )
                goto out;

        add_hb_data(heartbeat, str, client_get_status(client));

        if ( client->sha1sum ) {
                ret = prelude_string_new_constant(&str, "Analyzer SHA1");
                if ( ret < 0 )
                        goto out;

                add_hb_data(heartbeat, str, client->sha1sum);
        }

        ret = idmef_time_new_from_gettimeofday(&time);
        if ( ret < 0 )
                goto out;

        idmef_heartbeat_set_create_time(heartbeat, time);
        idmef_heartbeat_set_analyzer(heartbeat, idmef_analyzer_ref(client->_analyzer_copy), IDMEF_LIST_PREPEND);

        if ( client->heartbeat_cb ) {
                client->heartbeat_cb(client, message);
                goto out;
        }

        prelude_client_send_idmef(client, message);

  out:
        idmef_message_destroy(message);
}


static void heartbeat_expire_cb(void *data)
{
        prelude_client_t *client = data;

        gen_heartbeat(client);

        if ( client->status != CLIENT_STATUS_EXITING )
                prelude_timer_reset(&client->heartbeat_timer);
}


static void setup_heartbeat_timer(prelude_client_t *client, int expire)
{
        prelude_timer_set_data(&client->heartbeat_timer, client);
        prelude_timer_set_expire(&client->heartbeat_timer, expire);
        prelude_timer_set_callback(&client->heartbeat_timer, heartbeat_expire_cb);
}


#ifdef HAVE_IPV6
static prelude_bool_t is_loopback_ipv6(struct in6_addr *addr)
{
        struct in6_addr lo;

        inet_pton(AF_INET6, "::1", &lo);

        return (memcmp(addr, &lo, sizeof(lo)) == 0) ? TRUE : FALSE;
}
#endif


static prelude_bool_t is_loopback_ipv4(struct in_addr *addr)
{
        return (ntohl(addr->s_addr) >> 24) == 127 ? TRUE : FALSE;
}


static prelude_bool_t is_loopback(int family, void *addr)
{
        if ( family == AF_INET )
                return is_loopback_ipv4(addr);

#ifdef HAVE_IPV6
        else if ( family == AF_INET6 )
                return is_loopback_ipv6(addr);
#endif

        else
                return FALSE;
}


static int set_analyzer_host_info(idmef_analyzer_t *analyzer, const char *node_str, const char *addr_str)
{
        int ret;
        idmef_node_t *node;
        idmef_address_t *addr;
        prelude_string_t *str;

        if ( ! node_str && ! addr_str )
                return 0;

        ret = idmef_analyzer_new_node(analyzer, &node);
        if ( ret < 0 )
                return ret;

        ret = idmef_node_new_name(node, &str);
        if ( ret < 0 )
                return ret;

        if ( node_str && prelude_string_is_empty(str) )
                prelude_string_set_dup(str, node_str);

        if ( addr_str ) {
                if ( ! (addr = idmef_node_get_next_address(node, NULL)) ) {
                        ret = idmef_node_new_address(node, &addr, 0);
                        if ( ret < 0 )
                                return ret;
                }

                ret = idmef_address_new_address(addr, &str);
                if ( ret < 0 )
                        return ret;

                if ( prelude_string_is_empty(str) )
                        prelude_string_set_dup(str, addr_str);
        }

        return 0;
}


static int get_fqdn(idmef_analyzer_t *analyzer, const char *nodename)
{
        int ret;
        void *in_addr;
        char addr[256], *addrp = NULL;
        struct addrinfo hints, *ai, *ais;

        prelude_log_debug(1, "Detected nodename: '%s'.\n", nodename);

        memset(&hints, 0, sizeof(hints));
        hints.ai_flags = AI_CANONNAME;

        ret = getaddrinfo(nodename, NULL, &hints, &ai);
        if ( ret < 0 )
                return ret;

        if ( ai->ai_canonname ) {
                nodename = ai->ai_canonname;
                prelude_log_debug(1, "Found canonical name: '%s'.\n", nodename);
        }

        for ( ais = ai; ai != NULL; ai = ai->ai_next ) {
                in_addr = prelude_sockaddr_get_inaddr(ai->ai_addr);
                if ( ! in_addr )
                        continue;

                if ( ! inet_ntop(ai->ai_family, in_addr, addr, sizeof(addr)) )
                        continue;

                if ( is_loopback(ai->ai_family, in_addr) )
                        prelude_log_debug(1, "Ignoring loopback address: '%s'.\n", addr);
                else {
                        prelude_log_debug(1, "Found address: '%s'.\n", addr);
                        addrp = addr;
                        break;
                }
        }

        ret = set_analyzer_host_info(analyzer, nodename, addrp);
        freeaddrinfo(ais);

        return ret;
}


static int get_sys_info(idmef_analyzer_t *analyzer)
{
        int ret;
        struct utsname uts;
        prelude_string_t *str;

        if ( uname(&uts) < 0 )
                return prelude_error_from_errno(errno);

        get_fqdn(analyzer, uts.nodename);

        ret = prelude_string_new_dup(&str, uts.sysname);
        if ( ret < 0 )
                return ret;

        idmef_analyzer_set_ostype(analyzer, str);

        ret = prelude_string_new_dup(&str, uts.release);
        if ( ret < 0 )
                return ret;

        idmef_analyzer_set_osversion(analyzer, str);

        return 0;
}


static int fill_client_infos(prelude_client_t *client, const char *program)
{
        int ret;
        prelude_string_t *str, *sha1;
        idmef_process_t *process;
        char buf[PATH_MAX], *name, *path;

        snprintf(buf, sizeof(buf), "%" PRELUDE_PRIu64, prelude_client_profile_get_analyzerid(client->profile));
        ret = prelude_string_new_dup(&str, buf);
        if ( ret < 0 )
                return ret;

        idmef_analyzer_set_analyzerid(client->analyzer, str);

        ret = get_sys_info(client->analyzer);
        if ( ret < 0 )
                return ret;

        ret = idmef_analyzer_new_process(client->analyzer, &process);
        if ( ret < 0 )
                return ret;

        idmef_process_set_pid(process, getpid());

        if ( ! program || ! *program )
                return 0;

        name = path = NULL;
        _prelude_get_file_name_and_path(program, &name, &path);

        if ( name ) {
                ret = prelude_string_new_nodup(&str, name);
                if ( ret < 0 )
                        return ret;

                idmef_process_set_name(process, str);
        }

        if ( path && name ) {
                ret = idmef_process_new_path(process, &str);
                if ( ret < 0 )
                        return ret;

                ret = prelude_string_sprintf(str, "%s/%s", path, name);
                if ( ret < 0 )
                        return ret;

                ret = prelude_string_new(&sha1);
                if ( ret < 0 )
                        return ret;

                ret = generate_sha1sum(prelude_string_get_string(str), sha1);
                if ( ret < 0 )
                        return ret;

                ret = prelude_string_get_string_released(sha1, &client->sha1sum);
                prelude_string_destroy(sha1);
        }

        if ( path )
                free(path); /* copied above */

        return ret;
}




static int set_node_address_category(prelude_option_t *opt, const char *optarg, prelude_string_t *err, void *context)
{
        idmef_address_category_t category;
        node_address_data_t *data = context;

        category = idmef_address_category_to_numeric(optarg);
        if ( category < 0 )
                return category;

        idmef_address_set_category(data->addr, category);

        return 0;
}



static int get_node_address_category(prelude_option_t *opt, prelude_string_t *out, void *context)
{
        node_address_data_t *data = context;
        idmef_address_category_t category = idmef_address_get_category(data->addr);
        return prelude_string_cat(out, idmef_address_category_to_string(category));
}



static int set_node_address_vlan_num(prelude_option_t *opt, const char *optarg, prelude_string_t *err, void *context)
{
        node_address_data_t *data = context;

        if ( ! optarg )
                idmef_address_unset_vlan_num(data->addr);
        else
                idmef_address_set_vlan_num(data->addr, atoi(optarg));

        return 0;
}



static int get_node_address_vlan_num(prelude_option_t *opt, prelude_string_t *out, void *context)
{
        int32_t *num;
        node_address_data_t *data = context;

        num = idmef_address_get_vlan_num(data->addr);
        if ( num )
                return prelude_string_sprintf(out, "%" PRELUDE_PRId32, *num);

        return 0;
}



static int set_node_address_vlan_name(prelude_option_t *opt, const char *optarg, prelude_string_t *err, void *context)
{
        int ret;
        prelude_string_t *str = NULL;
        node_address_data_t *data = context;

        if ( optarg ) {
                ret = prelude_string_new_dup(&str, optarg);
                if ( ret < 0 )
                        return ret;
        }

        idmef_address_set_vlan_name(data->addr, str);

        return 0;
}



static int get_node_address_vlan_name(prelude_option_t *opt, prelude_string_t *out, void *context)
{
        prelude_string_t *str;
        node_address_data_t *data = context;

        str = idmef_address_get_vlan_name(data->addr);
        if ( ! str )
                return 0;

        return prelude_string_copy_ref(str, out);
}



static int set_node_address_address(prelude_option_t *opt, const char *optarg, prelude_string_t *err, void *context)
{
        int ret;
        prelude_string_t *str = NULL;
        node_address_data_t *data = context;

        if ( optarg ) {
                ret = prelude_string_new_dup(&str, optarg);
                if ( ret < 0 )
                        return ret;
        }

        idmef_address_set_address(data->addr, str);
        return 0;
}



static int get_node_address_address(prelude_option_t *opt, prelude_string_t *out, void *context)
{
        prelude_string_t *str;
        node_address_data_t *data = context;

        str = idmef_address_get_address(data->addr);
        if ( ! str )
                return 0;

        return prelude_string_copy_ref(str, out);
}



static int set_node_address_netmask(prelude_option_t *opt, const char *optarg, prelude_string_t *err, void *context)
{
        int ret;
        prelude_string_t *str = NULL;
        node_address_data_t *data = context;

        if ( optarg ) {
                ret = prelude_string_new_dup(&str, optarg);
                if ( ret < 0 )
                        return ret;
        }

        idmef_address_set_netmask(data->addr, str);
        return 0;
}



static int get_node_address_netmask(prelude_option_t *opt, prelude_string_t *out, void *context)
{
        prelude_string_t *str;
        node_address_data_t *data = context;

        str = idmef_address_get_netmask(data->addr);
        if ( ! str )
                return 0;

        return prelude_string_copy_ref(str, out);
}




static int set_node_address(prelude_option_t *opt, const char *optarg, prelude_string_t *err, void *context)
{
        int ret;
        node_address_data_t *data;
        prelude_option_context_t *octx;
        prelude_client_t *ptr = context;

        octx = prelude_option_search_context(opt, optarg);
        if ( octx )
                return 0;

        data = malloc(sizeof(*data));
        if ( ! data )
                return prelude_error_from_errno(errno);

        data->client = ptr;
        data->idmef_addr = NULL;

        ret = idmef_address_new(&data->addr);
        if ( ret < 0 ) {
                free(data);
                return ret;
        }

        ret = prelude_option_new_context(opt, &octx, optarg, data);
        if ( ret < 0 ) {
                idmef_address_destroy(data->addr);
                free(data);
        }

        return ret;
}


static int commit_node_address(prelude_option_t *opt, prelude_string_t *out, void *context)
{
        int ret;
        idmef_node_t *node;
        idmef_analyzer_t *analyzer;
        idmef_address_t *addr = NULL, *naddr;
        node_address_data_t *data = context;

        ret = idmef_analyzer_new_node(data->client->analyzer, &node);
        if ( ret < 0 )
                return ret;

        if ( node && data->idmef_addr ) {
                while ( (addr = idmef_node_get_next_address(node, addr)) ) {
                        if ( addr == data->idmef_addr ) {
                                idmef_address_destroy(addr);
                                break;
                        }
                }
        }

        ret = idmef_address_clone(data->addr, &naddr);
        if ( ret < 0 )
                return ret;

        data->idmef_addr = naddr;
        idmef_node_set_address(node, naddr, -1);

        if ( data->client->_analyzer_copy ) {
                ret = idmef_analyzer_clone(data->client->analyzer, &analyzer);
                if ( ret < 0 )
                        return ret;

                idmef_analyzer_destroy(data->client->_analyzer_copy);
                data->client->_analyzer_copy = analyzer;
        }

        return 0;
}


static int destroy_node_address(prelude_option_t *opt, prelude_string_t *out, void *context)
{
        int ret;
        idmef_node_t *node;
        idmef_analyzer_t *analyzer;
        idmef_address_t *addr = NULL;
        node_address_data_t *data = context;

        node = idmef_analyzer_get_node(data->client->analyzer);
        if ( node ) {
                while ( (addr = idmef_node_get_next_address(node, addr)) ) {
                        if ( addr == data->idmef_addr ) {
                                idmef_address_destroy(addr);
                                break;
                        }
                }
        }

        if ( data->client->_analyzer_copy ) {
                ret = idmef_analyzer_clone(data->client->analyzer, &analyzer);
                if ( ret == 0 ) {
                        idmef_analyzer_destroy(data->client->_analyzer_copy);
                        data->client->_analyzer_copy = analyzer;
                }
        }

        idmef_address_destroy(data->addr);
        free(data);

        return 0;
}



static int set_node_category(prelude_option_t *opt, const char *optarg, prelude_string_t *err, void *context)
{
        int ret;
        idmef_node_t *node;
        idmef_node_category_t category;
        prelude_client_t *ptr = context;

        category = idmef_node_category_to_numeric(optarg);
        if ( category < 0 )
                return category;

        ret = idmef_analyzer_new_node(ptr->analyzer, &node);
        if ( ret < 0 )
                return -1;

        idmef_node_set_category(node, category);
        return 0;
}



static int get_node_category(prelude_option_t *opt, prelude_string_t *out, void *context)
{
        const char *category;
        prelude_client_t *client = context;
        idmef_node_t *node = idmef_analyzer_get_node(client->analyzer);

        if ( ! node )
                return 0;

        category = idmef_node_category_to_string(idmef_node_get_category(node));
        if ( ! category )
                return -1;

        return prelude_string_cat(out, category);
}



static int set_node_location(prelude_option_t *opt, const char *optarg, prelude_string_t *err, void *context)
{
        int ret;
        idmef_node_t *node;
        prelude_string_t *str = NULL;
        prelude_client_t *ptr = context;

        ret = idmef_analyzer_new_node(ptr->analyzer, &node);
        if ( ret < 0 )
                return ret;

        if ( optarg ) {
                ret = prelude_string_new_dup(&str, optarg);
                if ( ret < 0 )
                        return ret;
        }

        idmef_node_set_location(node, str);

        return 0;
}



static int get_node_location(prelude_option_t *opt, prelude_string_t *out, void *context)
{
        prelude_string_t *str;
        prelude_client_t *client = context;
        idmef_node_t *node = idmef_analyzer_get_node(client->analyzer);

        if ( ! node )
                return 0;

        str = idmef_node_get_location(node);
        if ( ! str )
                return 0;

        return prelude_string_copy_ref(str, out);
}



static int set_node_name(prelude_option_t *opt, const char *optarg, prelude_string_t *err, void *context)
{
        int ret;
        idmef_node_t *node;
        prelude_string_t *str = NULL;
        prelude_client_t *ptr = context;

        ret = idmef_analyzer_new_node(ptr->analyzer, &node);
        if ( ret < 0 )
                return ret;

        if ( optarg ) {
                ret = prelude_string_new_dup(&str, optarg);
                if ( ret < 0 )
                        return ret;
        }

        idmef_node_set_name(node, str);

        return 0;
}



static int get_node_name(prelude_option_t *opt, prelude_string_t *out, void *context)
{
        prelude_string_t *str;
        prelude_client_t *client = context;
        idmef_node_t *node = idmef_analyzer_get_node(client->analyzer);

        if ( ! node )
                return 0;

        str = idmef_node_get_name(node);
        if ( ! str )
                return 0;

        return prelude_string_copy_ref(str, out);
}




static int get_analyzer_name(prelude_option_t *opt, prelude_string_t *out, void *context)
{
        prelude_string_t *str;
        prelude_client_t *client = context;

        str = idmef_analyzer_get_name(client->analyzer);
        if ( ! str )
                return 0;

        return prelude_string_copy_ref(str, out);
}




static int set_analyzer_name(prelude_option_t *opt, const char *optarg, prelude_string_t *err, void *context)
{
        int ret;
        prelude_string_t *str = NULL;
        prelude_client_t *ptr = context;

        if ( optarg ) {
                ret = prelude_string_new_dup(&str, optarg);
                if ( ret < 0 )
                        return ret;
        }

        idmef_analyzer_set_name(ptr->analyzer, str);
        return 0;
}



static int get_manager_addr(prelude_option_t *opt, prelude_string_t *out, void *context)
{
        prelude_client_t *ptr = context;

        if ( ! ptr->cpool || ! prelude_connection_pool_get_connection_string(ptr->cpool) )
                return 0;

        return prelude_string_cat(out, prelude_connection_pool_get_connection_string(ptr->cpool));
}



static int connection_pool_event_cb(prelude_connection_pool_t *pool,
                                    prelude_connection_pool_event_t event,
                                    prelude_connection_t *conn)
{
        int ret;
        prelude_client_t *client;
        prelude_msgbuf_t *msgbuf;
        prelude_msg_t *msg = NULL;

        if ( event != PRELUDE_CONNECTION_POOL_EVENT_INPUT )
                return 0;

        do {
                ret = prelude_connection_recv(conn, &msg);
        } while ( ret < 0 && prelude_error_get_code(ret) == PRELUDE_ERROR_EAGAIN );

        if ( ret < 0 )
                return ret;

        client = prelude_connection_pool_get_data(pool);

        ret = prelude_connection_new_msgbuf(conn, &msgbuf);
        if ( ret < 0 )
                return ret;

        ret = prelude_client_handle_msg_default(client, msg, msgbuf);

        prelude_msg_destroy(msg);
        prelude_msgbuf_destroy(msgbuf);

        return ret;
}



static int set_manager_addr(prelude_option_t *opt, const char *optarg, prelude_string_t *err, void *context)
{
        prelude_client_t *client = context;
        return prelude_connection_pool_set_connection_string(client->cpool, optarg);
}


static int set_tcp_keepalive_time(prelude_option_t *opt, const char *optarg, prelude_string_t *err, void *context)
{
        _prelude_connection_keepalive_time = atoi(optarg);
        return 0;
}


static int set_tcp_keepalive_probes(prelude_option_t *opt, const char *optarg, prelude_string_t *err, void *context)
{
        _prelude_connection_keepalive_probes = atoi(optarg);
        return 0;
}


static int set_tcp_keepalive_intvl(prelude_option_t *opt, const char *optarg, prelude_string_t *err, void *context)
{
        _prelude_connection_keepalive_intvl = atoi(optarg);
        return 0;
}


static int set_tls_options(prelude_option_t *opt, const char *optarg, prelude_string_t *err, void *context)
{
        return tls_auth_init_priority(optarg);
}

static int set_heartbeat_interval(prelude_option_t *opt, const char *optarg, prelude_string_t *err, void *context)
{
        prelude_client_t *ptr = context;

        setup_heartbeat_timer(ptr, atoi(optarg));

        if ( ptr->status == CLIENT_STATUS_RUNNING )
                prelude_timer_reset(&ptr->heartbeat_timer);

        return 0;
}



static int get_heartbeat_interval(prelude_option_t *opt, prelude_string_t *out, void *context)
{
        prelude_client_t *ptr = context;
        return prelude_string_sprintf(out, "%u", ptr->heartbeat_timer.expire);
}




static int set_profile(prelude_option_t *opt, const char *optarg, prelude_string_t *err, void *context)
{
        int ret;
        char buf[PATH_MAX];
        prelude_client_t *client = context;

        ret = prelude_client_profile_set_name(client->profile, optarg);
        if ( ret < 0 )
                return ret;

        if ( client->config_external == TRUE )
                return 0;

        prelude_client_profile_get_config_filename(client->profile, buf, sizeof(buf));

        prelude_client_set_config_filename(client, buf);
        client->config_external = FALSE;

        return 0;
}



static void _prelude_client_destroy(prelude_client_t *client)
{
        if ( client->profile )
                prelude_client_profile_destroy(client->profile);

        if ( client->sha1sum )
                free(client->sha1sum);

        if ( client->msgbuf )
                prelude_msgbuf_destroy(client->msgbuf);

        if ( client->analyzer )
                idmef_analyzer_destroy(client->analyzer);

        if ( client->_analyzer_copy )
                idmef_analyzer_destroy(client->_analyzer_copy);

        if ( client->config_filename )
                free(client->config_filename);

        if ( client->cpool )
                prelude_connection_pool_destroy(client->cpool);

        if ( client->unique_ident )
                prelude_ident_destroy(client->unique_ident);

        free(client);
}



static int handle_client_error(prelude_client_t *client, int error)
{
        char *tmp = NULL;

        prelude_error_code_t code;
        prelude_error_source_t source;

        code = prelude_error_get_code(error);
        source = prelude_error_get_source(error);

        if ( error < 0 && (code == PRELUDE_ERROR_PROFILE || source == PRELUDE_ERROR_SOURCE_CONFIG_ENGINE) ) {
                if ( _prelude_thread_get_error() )
                        tmp = strdup(_prelude_thread_get_error());

                error = prelude_error_verbose(PRELUDE_ERROR_PROFILE, "%s%s%s", tmp ? tmp : "", tmp ? "\n" : "", prelude_client_get_setup_error(client));
                free(tmp);
        }

        return error;
}



int _prelude_client_register_options(void)
{
        int ret;
        prelude_option_t *opt;
        prelude_option_t *root_list;

        prelude_option_new_root(&_prelude_generic_optlist);

        ret = prelude_option_add(_prelude_generic_optlist, &root_list,
                                 PRELUDE_OPTION_TYPE_CLI|PRELUDE_OPTION_TYPE_CFG|PRELUDE_OPTION_TYPE_WIDE, 0,
                                 "prelude", "Prelude generic options", PRELUDE_OPTION_ARGUMENT_NONE, NULL, NULL);
        if ( ret < 0 )
                return ret;

        ret = prelude_option_add(root_list, &opt, PRELUDE_OPTION_TYPE_CLI, 0, "profile",
                                 "Profile to use for this analyzer", PRELUDE_OPTION_ARGUMENT_REQUIRED,
                                 set_profile, NULL);
        if ( ret < 0 )
                return ret;
        prelude_option_set_priority(opt, PRELUDE_OPTION_PRIORITY_IMMEDIATE);

        ret = prelude_option_add(root_list, NULL, PRELUDE_OPTION_TYPE_CLI|PRELUDE_OPTION_TYPE_CFG
                                 |PRELUDE_OPTION_TYPE_WIDE, 0, "heartbeat-interval",
                                 "Number of seconds between two heartbeat",
                                 PRELUDE_OPTION_ARGUMENT_REQUIRED,
                                 set_heartbeat_interval, get_heartbeat_interval);
        if ( ret < 0 )
                return ret;

        ret = prelude_option_add(root_list, &opt, PRELUDE_OPTION_TYPE_CLI|PRELUDE_OPTION_TYPE_CFG
                                 |PRELUDE_OPTION_TYPE_WIDE, 0, "server-addr",
                                 "Address where this agent should report events to (addr:port)",
                                 PRELUDE_OPTION_ARGUMENT_REQUIRED, set_manager_addr, get_manager_addr);
        if ( ret < 0 )
                return ret;
        prelude_option_set_priority(opt, PRELUDE_OPTION_PRIORITY_LAST);

        ret = prelude_option_add(root_list, &opt, PRELUDE_OPTION_TYPE_CFG|PRELUDE_OPTION_TYPE_CLI, 0, "tls-options",
                                 "TLS ciphers, key exchange methods, protocols, macs, and compression options",
                                 PRELUDE_OPTION_ARGUMENT_REQUIRED, set_tls_options, NULL);
        if ( ret < 0 )
                return ret;

        ret = prelude_option_add(root_list, NULL, PRELUDE_OPTION_TYPE_CFG, 0,
                                 "tcp-keepalive-time", "Interval between the last data packet sent and the first keepalive probe",
                                 PRELUDE_OPTION_ARGUMENT_REQUIRED, set_tcp_keepalive_time, NULL);
        if ( ret < 0 )
                return ret;

        ret = prelude_option_add(root_list, NULL, PRELUDE_OPTION_TYPE_CFG, 0,
                                 "tcp-keepalive-probes", "Number of not acknowledged probes to send before considering the connection dead",
                                 PRELUDE_OPTION_ARGUMENT_REQUIRED, set_tcp_keepalive_probes, NULL);
        if ( ret < 0 )
                return ret;

        ret = prelude_option_add(root_list, NULL, PRELUDE_OPTION_TYPE_CFG, 0,
                                 "tcp-keepalive-intvl", "Interval between subsequential keepalive probes",
                                 PRELUDE_OPTION_ARGUMENT_REQUIRED, set_tcp_keepalive_intvl, NULL);
        if ( ret < 0 )
                return ret;

        ret = prelude_option_add(root_list, NULL, PRELUDE_OPTION_TYPE_CLI|PRELUDE_OPTION_TYPE_CFG|
                                 PRELUDE_OPTION_TYPE_WIDE, 0, "analyzer-name", "Name for this analyzer",
                                 PRELUDE_OPTION_ARGUMENT_OPTIONAL, set_analyzer_name, get_analyzer_name);
        if ( ret < 0 )
                return ret;

        ret = prelude_option_add(root_list, NULL, PRELUDE_OPTION_TYPE_CFG|PRELUDE_OPTION_TYPE_WIDE, 0, "node-name",
                                 "Name of the equipment", PRELUDE_OPTION_ARGUMENT_OPTIONAL,
                                 set_node_name, get_node_name);
        if ( ret < 0 )
                return ret;

        ret = prelude_option_add(root_list, NULL, PRELUDE_OPTION_TYPE_CFG|PRELUDE_OPTION_TYPE_WIDE, 0, "node-location",
                                 "Location of the equipment", PRELUDE_OPTION_ARGUMENT_OPTIONAL,
                                 set_node_location, get_node_location);
        if ( ret < 0 )
                return ret;

        ret = prelude_option_add(root_list, NULL, PRELUDE_OPTION_TYPE_CFG|PRELUDE_OPTION_TYPE_WIDE, 0, "node-category",
                                 NULL, PRELUDE_OPTION_ARGUMENT_REQUIRED, set_node_category, get_node_category);
        if ( ret < 0 )
                return ret;

        ret = prelude_option_add(root_list, &opt, PRELUDE_OPTION_TYPE_CFG|PRELUDE_OPTION_TYPE_WIDE
                                 |PRELUDE_OPTION_TYPE_CONTEXT, 0, "node-address",
                                 "Network or hardware address of the equipment",
                                 PRELUDE_OPTION_ARGUMENT_OPTIONAL, set_node_address, NULL);
        if ( ret < 0 )
                return ret;

        prelude_option_set_commit_callback(opt, commit_node_address);
        prelude_option_set_destroy_callback(opt, destroy_node_address);

        ret = prelude_option_add(opt, NULL, PRELUDE_OPTION_TYPE_CFG|PRELUDE_OPTION_TYPE_WIDE, 0, "address",
                                 "Address information", PRELUDE_OPTION_ARGUMENT_OPTIONAL,
                                 set_node_address_address, get_node_address_address);
        if ( ret < 0 )
                return ret;

        ret = prelude_option_add(opt, NULL, PRELUDE_OPTION_TYPE_CFG|PRELUDE_OPTION_TYPE_WIDE, 0, "netmask",
                                 "Network mask for the address, if appropriate", PRELUDE_OPTION_ARGUMENT_OPTIONAL,
                                 set_node_address_netmask, get_node_address_netmask);
        if ( ret < 0 )
                return ret;

        ret = prelude_option_add(opt, NULL, PRELUDE_OPTION_TYPE_CFG|PRELUDE_OPTION_TYPE_WIDE, 0, "category",
                                 "Type of address represented", PRELUDE_OPTION_ARGUMENT_REQUIRED,
                                 set_node_address_category, get_node_address_category);
        if ( ret < 0 )
                return ret;

        ret = prelude_option_add(opt, NULL, PRELUDE_OPTION_TYPE_CFG|PRELUDE_OPTION_TYPE_WIDE, 0, "vlan-name",
                                 "Name of the Virtual LAN to which the address belongs",
                                 PRELUDE_OPTION_ARGUMENT_OPTIONAL, set_node_address_vlan_name,
                                 get_node_address_vlan_name);
        if ( ret < 0 )
                return ret;

        ret = prelude_option_add(opt, NULL, PRELUDE_OPTION_TYPE_CFG|PRELUDE_OPTION_TYPE_WIDE, 0, "vlan-num",
                                 "Number of the Virtual LAN to which the address belongs",
                                 PRELUDE_OPTION_ARGUMENT_OPTIONAL, set_node_address_vlan_num,
                                 get_node_address_vlan_num);
        if ( ret < 0 )
                return ret;

        return 0;
}




/**
 * prelude_client_new:
 * @client: Pointer to a client object to initialize.
 * @profile: Default profile name for this analyzer.
 *
 * This function initialize the @client object.
 *
 * Returns: 0 on success or a negative value if an error occur.
 */
int prelude_client_new(prelude_client_t **client, const char *profile)
{
        int ret;
        prelude_client_t *new;

        prelude_return_val_if_fail(profile, prelude_error(PRELUDE_ERROR_ASSERTION));

        new = calloc(1, sizeof(*new));
        if ( ! new )
                return prelude_error_from_errno(errno);

        gl_lock_init(new->msgbuf_lock);
        prelude_timer_init_list(&new->heartbeat_timer);

        new->refcount = 1;
        new->flags = PRELUDE_CLIENT_FLAGS_HEARTBEAT|PRELUDE_CLIENT_FLAGS_CONNECT|PRELUDE_CLIENT_FLAGS_AUTOCONFIG;
        new->permission = PRELUDE_CONNECTION_PERMISSION_IDMEF_WRITE;

        ret = idmef_analyzer_new(&new->analyzer);
        if ( ret < 0 ) {
                _prelude_client_destroy(new);
                return ret;
        }

        set_analyzer_name(NULL, profile, NULL, new);

        ret = _prelude_client_profile_new(&new->profile);
        if ( ret < 0 ) {
                _prelude_client_destroy(new);
                return ret;
        }

        set_profile(NULL, profile, NULL, new);

        ret = prelude_ident_new(&new->unique_ident);
        if ( ret < 0 ) {
                _prelude_client_destroy(new);
                return ret;
        }

        ret = prelude_connection_pool_new(&new->cpool, new->profile, new->permission);
        if ( ret < 0 )
                return ret;

        prelude_connection_pool_set_data(new->cpool, new);
        prelude_connection_pool_set_flags(new->cpool, prelude_connection_pool_get_flags(new->cpool) |
                                          PRELUDE_CONNECTION_POOL_FLAGS_RECONNECT | PRELUDE_CONNECTION_POOL_FLAGS_FAILOVER);
        prelude_connection_pool_set_event_handler(new->cpool, PRELUDE_CONNECTION_POOL_EVENT_INPUT, connection_pool_event_cb);


        setup_heartbeat_timer(new, DEFAULT_HEARTBEAT_INTERVAL);


        ret = prelude_client_new_msgbuf(new, &new->msgbuf);
        if ( ret < 0 ) {
                _prelude_client_destroy(new);
                return ret;
        }

        *client = new;

        return 0;
}



prelude_client_t *prelude_client_ref(prelude_client_t *client)
{
        prelude_return_val_if_fail(client, NULL);

        client->refcount++;
        return client;
}



/**
 * prelude_client_init:
 * @client: Pointer to a #prelude_client_t object to initialize.
 *
 * This function initialize the @client object, meaning reading generic
 * options from the prelude_client_new() provided configuration file
 * and the array of arguments specified through prelude_init().
 *
 * Calling this function is optional and should be done only if you need more
 * granularity between prelude_client_new() and prelude_client_start():
 *
 * prelude_client_start() will call prelude_client_init() for you if needed.
 *
 * Returns: 0 on success, -1 if an error occured.
 */
int prelude_client_init(prelude_client_t *client)
{
        int ret;
        prelude_string_t *err;
        prelude_option_warning_t old_warnings;

        /*
         * Calling two time init() would result in error in
         * fill_client_infos(), due to idmef_analyzer_t object reuse.
         */
        if ( client->status != CLIENT_STATUS_NEED_INIT )
                return 0;

        prelude_return_val_if_fail(client, prelude_error(PRELUDE_ERROR_ASSERTION));

        prelude_option_set_warnings(0, &old_warnings);

        ret = prelude_option_read(_prelude_generic_optlist, (const char **)&client->config_filename,
                                  &_prelude_internal_argc, _prelude_internal_argv, &err, client);

        prelude_option_set_warnings(old_warnings, NULL);

        if ( ret < 0 )
                return handle_client_error(client, ret);

        ret = _prelude_client_profile_init(client->profile);
        if ( ret < 0 )
                return handle_client_error(client, ret);

        ret = fill_client_infos(client, _prelude_internal_argv[0]);
        if ( ret < 0 )
                return handle_client_error(client, ret);

        client->status = CLIENT_STATUS_INIT_DONE;

        return 0;
}




/**
 * prelude_client_start:
 * @client: Pointer to a client object to initialize.
 *
 * This function start the @client object, triggering
 * a connection from the client to it's server if any were
 * specified, and sending the initial @client heartbeat.
 *
 * If @client was not initialized, then prelude_client_init()
 * will be called and thus this function might fail if the
 * client was not registered.
 *
 * Returns: 0 on success, -1 if an error occured.
 */
int prelude_client_start(prelude_client_t *client)
{
        int ret;
        void *credentials;

        prelude_return_val_if_fail(client, prelude_error(PRELUDE_ERROR_ASSERTION));

        if ( client->status == CLIENT_STATUS_NEED_INIT ) {
                /*
                 * if prelude_client_init() was not called
                 */
                ret = prelude_client_init(client);
                if ( ret < 0 )
                        return ret;
        }

        if ( client->flags & PRELUDE_CLIENT_FLAGS_CONNECT ) {
                if ( ! client->cpool )
                        return prelude_error(PRELUDE_ERROR_CONNECTION_STRING);

                ret = prelude_client_profile_get_credentials(client->profile, &credentials);
                if ( ret < 0 )
                        return handle_client_error(client, ret);

                ret = prelude_connection_pool_init(client->cpool);
                if ( ret < 0 )
                        return handle_client_error(client, ret);
        }


        if ( (client->cpool || client->heartbeat_cb) && client->flags & PRELUDE_CLIENT_FLAGS_HEARTBEAT ) {

                client->status = CLIENT_STATUS_STARTING;
                client->_analyzer_copy = client->analyzer;
                gen_heartbeat(client);

                /*
                 * We use a copy of the analyzer object from the timer,
                 * since it might be run asynchronously.
                 */
                ret = idmef_analyzer_clone(client->analyzer, &client->_analyzer_copy);
                if ( ret < 0 )
                        return ret;

                client->status = CLIENT_STATUS_RUNNING;
                prelude_timer_init(&client->heartbeat_timer);
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
        prelude_return_val_if_fail(client, NULL);
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
        prelude_return_if_fail(client);
        prelude_return_if_fail(msg);

        if ( client->flags & PRELUDE_CLIENT_FLAGS_ASYNC_SEND )
                prelude_connection_pool_broadcast_async(client->cpool, msg);
        else
                prelude_connection_pool_broadcast(client->cpool, msg);
}



/**
 * prelude_client_send_idmef:
 * @client: Pointer to a #prelude_client_t object.
 * @msg: pointer to an IDMEF message to be sent to @client peers.
 *
 * Send @msg to the peers @client is communicating with.
 *
 * The message will be sent asynchronously if @PRELUDE_CLIENT_FLAGS_ASYNC_SEND
 * was set using prelude_client_set_flags().
 */
void prelude_client_send_idmef(prelude_client_t *client, idmef_message_t *msg)
{
        prelude_return_if_fail(client);
        prelude_return_if_fail(msg);

        /*
         * we need to hold a lock since asynchronous heartbeat
         * could write the message buffer at the same time we do.
         */
        gl_lock_lock(client->msgbuf_lock);

        _idmef_message_assign_missing(client, msg);
        idmef_message_write(msg, client->msgbuf);
        prelude_msgbuf_mark_end(client->msgbuf);

        gl_lock_unlock(client->msgbuf_lock);
}



/**
 * prelude_client_recv_msg:
 * @client: Pointer to a #prelude_client_t object.
 * @timeout: Number of millisecond to wait for a message.
 * @msg: Pointer where the received #prelude_msg_t should be stored.
 *
 * Wait @timeout second for a message on @client connection pool.
 *
 * A @timeout of -1, mean prelude_client_recv_msg() will block until
 * a message is received. A @timeout of 0 mean that it will return
 * immediatly.
 *
 * Returns: 0 on timeout, a negative value on error, 1 on success.
 */
int prelude_client_recv_msg(prelude_client_t *client, int timeout, prelude_msg_t **msg)
{
        int ret;
        prelude_msg_t *m = NULL;
        prelude_connection_t *con;

        prelude_return_val_if_fail(client, prelude_error(PRELUDE_ERROR_ASSERTION));
        prelude_return_val_if_fail(msg, prelude_error(PRELUDE_ERROR_ASSERTION));

        ret = prelude_connection_pool_recv(client->cpool, timeout, &con, &m);
        if ( ret <= 0 )
                return ret;

        ret = prelude_client_handle_msg_default(client, m, client->msgbuf);
        if ( ret == 0 ) {
                prelude_msg_destroy(m);
                return 0;
        }

        *msg = m;
        return 1;
}



/**
 * prelude_client_recv_idmef:
 * @client: Pointer to a #prelude_client_t object.
 * @timeout: Number of second to wait for a message.
 * @idmef: Pointer where the received #idmef_message_t should be stored.
 *
 * Wait @timeout second for a message on @client connection pool.
 *
 * A @timeout of -1, mean prelude_client_recv_idmef() will block until
 * a message is received. A @timeout of 0 mean that it will return
 * immediatly.
 *
 * Returns: 0 on timeout, a negative value on error, 1 on success.
 */
int prelude_client_recv_idmef(prelude_client_t *client, int timeout, idmef_message_t **idmef)
{
        int ret;
        prelude_msg_t *msg = NULL;

        prelude_return_val_if_fail(client, prelude_error(PRELUDE_ERROR_ASSERTION));
        prelude_return_val_if_fail(idmef, prelude_error(PRELUDE_ERROR_ASSERTION));

        if ( ! (client->permission & PRELUDE_CONNECTION_PERMISSION_IDMEF_READ) )
                return prelude_error_verbose(PRELUDE_ERROR_GENERIC,
                                             "Client should use 'idmef:r' permission to read IDMEF message");

        ret = prelude_client_recv_msg(client, timeout, &msg);
        if ( ret <= 0 )
                return ret;

        ret = idmef_message_new(idmef);
        if ( ret < 0 ) {
                prelude_msg_destroy(msg);
                return ret;
        }

        ret = idmef_message_read(*idmef, msg);
        if ( ret < 0 ) {
                prelude_msg_destroy(msg);
                idmef_message_destroy(*idmef);
                return ret;
        }

        idmef_message_set_pmsg(*idmef, msg);

        return 1;
}



/**
 * prelude_client_get_connection_pool:
 * @client: pointer to a #prelude_client_t object.
 *
 * Return a pointer to the #prelude_connection_pool_t object used by @client
 * to send messages.
 *
 * Returns: a pointer to a #prelude_connection_pool_t object.
 */
prelude_connection_pool_t *prelude_client_get_connection_pool(prelude_client_t *client)
{
        prelude_return_val_if_fail(client, NULL);
        return client->cpool;
}



/**
 * prelude_client_set_connection_pool:
 * @client: pointer to a #prelude_client_t object.
 * @pool: pointer to a #prelude_client_pool_t object.
 *
 * Use this function in order to set your own list of peer that @client
 * should send message too. This might be usefull in case you don't want
 * this to be automated by prelude_client_init().
 */
void prelude_client_set_connection_pool(prelude_client_t *client, prelude_connection_pool_t *pool)
{
        prelude_return_if_fail(client);
        prelude_return_if_fail(pool);

        if ( client->cpool )
                prelude_connection_pool_destroy(client->cpool);

        client->cpool = pool;
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
        prelude_return_if_fail(client);
        prelude_return_if_fail(cb);

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
        prelude_return_if_fail(client);

        if ( --client->refcount )
                return;

        prelude_timer_destroy(&client->heartbeat_timer);

        if ( client->status >= CLIENT_STATUS_STARTING     &&
             status == PRELUDE_CLIENT_EXIT_STATUS_SUCCESS &&
             client->flags & PRELUDE_CLIENT_FLAGS_HEARTBEAT ) {

                client->status = CLIENT_STATUS_EXITING;
                heartbeat_expire_cb(client);
        }

        _prelude_client_destroy(client);
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

        prelude_return_val_if_fail(client, prelude_error(PRELUDE_ERROR_ASSERTION));

        client->flags = flags;

        if ( flags & PRELUDE_CLIENT_FLAGS_ASYNC_TIMER ) {
                prelude_async_set_flags(PRELUDE_ASYNC_FLAGS_TIMER);
                ret = prelude_async_init();
        }

        if ( flags & PRELUDE_CLIENT_FLAGS_ASYNC_SEND ) {
                prelude_msgbuf_set_flags(client->msgbuf, PRELUDE_MSGBUF_FLAGS_ASYNC);
                ret = prelude_async_init();
        }

        if ( ! (flags & PRELUDE_CLIENT_FLAGS_AUTOCONFIG) )
                prelude_client_set_config_filename(client, NULL);

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
        prelude_return_val_if_fail(client, prelude_error(PRELUDE_ERROR_ASSERTION));
        return client->flags;
}



/**
 * prelude_client_get_required_permission:
 * @client: Pointer on a #prelude_client_t object.
 *
 * Returns: @client permission as set with prelude_client_set_required_permission()
 */
prelude_connection_permission_t prelude_client_get_required_permission(prelude_client_t *client)
{
        prelude_return_val_if_fail(client, prelude_error(PRELUDE_ERROR_ASSERTION));
        return client->permission;
}



/**
 * prelude_client_set_required_permission:
 * @client: Pointer on a #prelude_client_t object.
 * @permission: Required permission for @client.
 *
 * Set the required @permission for @client.
 * The default is #PRELUDE_CONNECTION_PERMISSION_IDMEF_WRITE | #PRELUDE_CONNECTION_PERMISSION_ADMIN_READ.
 * Value set through this function should be set before prelude_client_start().
 *
 * If the client certificate for connecting to one of the specified manager doesn't have theses permission
 * the client will reject the certificate and ask for registration.
 */
void prelude_client_set_required_permission(prelude_client_t *client, prelude_connection_permission_t permission)
{
        prelude_return_if_fail(client);

        if ( permission & PRELUDE_CONNECTION_PERMISSION_IDMEF_READ )
                prelude_connection_pool_set_event_handler(client->cpool, 0, NULL);

        client->permission = permission;
        prelude_connection_pool_set_required_permission(client->cpool, permission);
}



/**
 * prelude_client_get_config_filename:
 * @client: pointer on a #prelude_client_t object.
 *
 * Return the filename where @client configuration is stored.
 * This filename is originally set by the prelude_client_new() function.
 *
 * Returns: a pointer to @client configuration filename.
 */
const char *prelude_client_get_config_filename(prelude_client_t *client)
{
        prelude_return_val_if_fail(client, NULL);
        return client->config_filename;
}



/**
 * prelude_client_set_config_filename:
 * @client: pointer on a #prelude_client_t object.
 * @filename: Configuration file to use for this client.
 *
 * The default for a client is to use a template configuration file (idmef-client.conf).
 * By using this function you might override the default and provide your own
 * configuration file to use for @client. The format of the configuration file need
 * to be compatible with the Prelude format.
 *
 * Returns: 0 on success, -1 if an error occured.
 */
int prelude_client_set_config_filename(prelude_client_t *client, const char *filename)
{
        prelude_return_val_if_fail(client, prelude_error(PRELUDE_ERROR_ASSERTION));

        if ( client->config_filename ) {
                free(client->config_filename);
                client->config_filename = NULL;
        }

        if ( ! filename )
                client->flags &= ~PRELUDE_CLIENT_FLAGS_AUTOCONFIG;

        else {
                client->config_filename = strdup(filename);
                if ( ! client->config_filename )
                        return prelude_error_from_errno(errno);
        }

        client->config_external = TRUE;

        return 0;
}



prelude_ident_t *prelude_client_get_unique_ident(prelude_client_t *client)
{
        prelude_return_val_if_fail(client, NULL);
        return client->unique_ident;
}



prelude_client_profile_t *prelude_client_get_profile(prelude_client_t *client)
{
        prelude_return_val_if_fail(client, NULL);
        return client->profile;
}



#ifndef PRELUDE_DISABLE_DEPRECATED
/**
 * prelude_client_is_setup_needed:
 * @error: Error returned by prelude_client_start().
 *
 * This function should be called as a result of an error by
 * the prelude_client_start() function, to know if the analyzer
 * need to be registered.
 *
 * DEPRECATED: use standard error API.
 *
 * Returns: TRUE if setup is needed, FALSE otherwise.
 */
prelude_bool_t prelude_client_is_setup_needed(int error)
{
        /*
         * Deprecated.
         */
        return FALSE;
}


const char *prelude_client_get_setup_error(prelude_client_t *client)
{
        int ret;
        prelude_string_t *out, *perm;

        prelude_return_val_if_fail(client, NULL);

        ret = prelude_string_new(&out);
        if ( ret < 0 )
                return NULL;

        if ( client->flags & PRELUDE_CLIENT_FLAGS_CONNECT ) {
                ret = prelude_string_new(&perm);
                if ( ret < 0 ) {
                        prelude_string_destroy(out);
                        return NULL;
                }

                prelude_connection_permission_to_string(client->permission, perm);

                ret = prelude_string_sprintf(out, "\nProfile '%s' does not exist. In order to create it, please run:\n"
                                             "prelude-admin register \"%s\" \"%s\" <manager address> --uid %d --gid %d",
                                             prelude_client_profile_get_name(client->profile),
                                             prelude_client_profile_get_name(client->profile),
                                             prelude_string_get_string(perm),
                                             (int) prelude_client_profile_get_uid(client->profile),
                                             (int) prelude_client_profile_get_gid(client->profile));

                prelude_string_destroy(perm);

        } else {
                ret = prelude_string_sprintf(out, "\nProfile '%s' does not exist. In order to create it, please run:\n"
                                             "prelude-admin add \"%s\" --uid %d --gid %d",
                                             prelude_client_profile_get_name(client->profile),
                                             prelude_client_profile_get_name(client->profile),
                                             (int) prelude_client_profile_get_uid(client->profile),
                                             (int) prelude_client_profile_get_gid(client->profile));
        }

        if ( ret < 0 )
                return NULL;

        _prelude_thread_set_error(prelude_string_get_string(out));
        prelude_string_destroy(out);

        return _prelude_thread_get_error();
}
#endif



void prelude_client_print_setup_error(prelude_client_t *client)
{
        prelude_return_if_fail(client);
        prelude_log(PRELUDE_LOG_WARN, "%s\n\n", prelude_client_get_setup_error(client));
}



int prelude_client_handle_msg_default(prelude_client_t *client, prelude_msg_t *msg, prelude_msgbuf_t *msgbuf)
{
        int ret;
        uint8_t tag;

        prelude_return_val_if_fail(client, prelude_error(PRELUDE_ERROR_ASSERTION));
        prelude_return_val_if_fail(msg, prelude_error(PRELUDE_ERROR_ASSERTION));
        prelude_return_val_if_fail(msgbuf, prelude_error(PRELUDE_ERROR_ASSERTION));

        tag = prelude_msg_get_tag(msg);
        if ( tag != PRELUDE_MSG_OPTION_REQUEST )
                return prelude_error_verbose(PRELUDE_ERROR_GENERIC, "Unexpected message type '%d' received", tag);

        /*
         * lock, handle the request, send reply.
         */
        gl_lock_lock(client->msgbuf_lock);

        ret = prelude_option_process_request(client, msg, msgbuf);
        prelude_msgbuf_mark_end(client->msgbuf);

        gl_lock_unlock(client->msgbuf_lock);

        return ret;
}



int prelude_client_new_msgbuf(prelude_client_t *client, prelude_msgbuf_t **msgbuf)
{
        int ret;

        prelude_return_val_if_fail(client, prelude_error(PRELUDE_ERROR_ASSERTION));

        ret = prelude_msgbuf_new(msgbuf);
        if ( ret < 0 )
                return ret;

        prelude_msgbuf_set_data(*msgbuf, client);
        prelude_msgbuf_set_callback(*msgbuf, client_write_cb);

        return 0;
}
