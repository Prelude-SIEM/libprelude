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


#ifndef IDMEF_TREE_H
#define IDMEF_TREE_H

#include <inttypes.h>
#include <libprelude/list.h>

#define IDMEF_VERSION "0.5"


/*
 * Additional Data class
 */
typedef enum {
        boolean   = 0,
        byte      = 1,
        character = 2,
        date_time = 3,
        integer   = 4,
        ntpstamps = 5,
        portlist  = 6,
        real      = 7,
        string    = 8,
        xml       = 9,
} idmef_additional_data_type_t;



typedef struct {
        struct list_head list;
        idmef_additional_data_type_t type;
        const char *meaning;
        const char *data;
} idmef_additional_data_t;





/*
 * Classification class
 */
typedef enum {
        origin_unknown  = 0,
        bugtraqid       = 1,
        cve             = 2,
        vendor_specific = 3,
} idmef_classification_origin_t;



typedef struct {
        struct list_head list;
        idmef_classification_origin_t origin;
        const char *name;
        const char *url;
} idmef_classification_t;





/*
 * UserId class
 */
typedef enum {
        current_user  = 0,
        original_user = 1,
        target_user   = 2,
        user_privs    = 3,
        current_group = 4,
        group_privs   = 5,
} idmef_userid_type_t;


typedef struct {
        struct list_head list;
        
        const char *ident;
        idmef_userid_type_t type;
        const char *name;
        const char *number;
} idmef_userid_t;






/*
 * User class
 */
typedef enum {
        cat_unknown  = 0,
        application  = 1,
        os_device    = 2,
} idmef_user_category_t;



typedef struct {
        const char *ident;
        idmef_user_category_t category;
        struct list_head userid_list;
} idmef_user_t;




/*
 * Address class
 */
typedef enum {
        addr_unknown  = 0,
        atm           = 1,
        e_mail        = 2,
        lotus_notes   = 3,
        mac           = 4,
        sna           = 5,
        vm            = 6,
        ipv4_addr     = 7,
        ipv4_addr_hex = 8,
        ipv4_net      = 9,
        ipv4_net_mask = 10,
        ipv6_addr     = 11,
        ipv6_addr_hex = 12,
        ipv6_net      = 13,
        ipv6_net_mask = 14,
} idmef_address_category_t;




typedef struct {
        struct list_head list;
        
        const char *ident;
        idmef_address_category_t category;
        const char *vlan_name;
        int vlan_num;
        const char *address;
        const char *netmask;
} idmef_address_t;



/*
 * Process class
 */
typedef struct {
        const char *ident;
        const char *name;
        uint32_t pid;
        const char *path;
        const char **arg;
        const char **env;
} idmef_process_t;



/*
 * WebService class
 */
typedef struct {
        const char *url;
        const char *cgi;
        const char *method;
        const char *arg;
} idmef_webservice_t;




/*
 * SNMPService class
 */
typedef struct {
        const char *oid;
        const char *community;
        const char *command;
} idmef_snmpservice_t;

        


/*
 * Service class
 */
typedef struct {
        const char *ident;
        const char *name;
        uint16_t port;
        const char *portlist;
        const char *protocol;

        union {
                idmef_webservice_t *web;
                idmef_snmpservice_t *snmp;
        } specific;
        
} idmef_service_t;




/*
 * Node class
 */
typedef enum {
        node_unknown = 0,
        ads          = 1,
        afs          = 2,
        coda         = 3,
        dfs          = 4,
        dns          = 5,
        kerberos     = 6,
        nds          = 7,
        nis          = 8,
        nisplus      = 9,
        nt           = 10,
        wfw          = 11,
} idmef_node_category_t;


typedef struct {
        const char *ident;
        idmef_node_category_t category;
        const char *location;
        const char *name;
        struct list_head address_list;
} idmef_node_t;





/*
 * Source class
 */
typedef enum {
        unknown = 0,
        yes     = 1,
        no      = 2,
} idmef_spoofed_t;


typedef struct {
        struct list_head list;
        
        const char *ident;
        idmef_spoofed_t spoofed;
        const char *interface;

        idmef_node_t node;
        idmef_user_t user;
        idmef_process_t process;
        idmef_service_t service;
        
} idmef_source_t;




/*
 * Target class
 */
typedef struct {
        struct list_head list;
        
        const char *ident;
        idmef_spoofed_t decoy;
        const char *interface;

        idmef_node_t node;
        idmef_user_t user;
        idmef_process_t process;
        idmef_service_t service;
        
} idmef_target_t;





/*
 * Analyzer class
 */
typedef struct {
        const char *analyzerid;
        const char *manufacturer;
        const char *model;
        const char *version;
        const char *class;

        idmef_node_t node;
        idmef_process_t process;
} idmef_analyzer_t;





/*
 * Time class
 */
typedef struct {
        const char *ntpstamp;
        const char *time;
} idmef_time_t;




/*
 * Toolalert class
 */
typedef struct {
        const char *name;
        const char *command;
        const char **analyzerid;
} idmef_tool_alert_t;





/*
 * CorrelationAlert class
 */
typedef struct {
        const char *name;
        const char **alertident;
} idmef_correlation_alert_t;




/*
 * OverflowAlert class
 */
typedef struct {
        const char *program;
        uint32_t size;
        const unsigned char *buffer;
} idmef_overflow_alert_t;




/*
 * Alert class
 */
typedef enum {
        idmef_tool_alert        = 0,
        idmef_correlation_alert = 1,
        idmef_overflow_alert    = 2,
} idmef_alert_type_t;



typedef struct {
        const char *ident;
        const char *impact;
        const char *action;
    
        idmef_analyzer_t analyzer;
    
        idmef_time_t create_time;
        idmef_time_t detect_time;
        idmef_time_t analyzer_time;

        struct list_head source_list;
        struct list_head target_list;
        struct list_head classification_list;
        struct list_head additional_data_list;

        idmef_alert_type_t type;
        union {
                idmef_tool_alert_t *tool_alert;
                idmef_correlation_alert_t *correlation_alert;
                idmef_overflow_alert_t *overflow_alert;
        } detail;
        
} idmef_alert_t;





/*
 * Heartbeat class
 */
typedef struct {
        const char *ident;

        idmef_analyzer_t analyzer;
        idmef_time_t analyzer_time;

        struct list_head additional_data_list;
} idmef_heartbeat_t;




/*
 * IDMEF Message class
 */
typedef enum {
        idmef_alert_message     = 0,
        idmef_heartbeat_message = 1,
} idmef_message_type_t;


typedef struct {
        const char *version;

        idmef_message_type_t type;
        union {
                idmef_alert_t *alert;
                idmef_heartbeat_t *heartbeat;
        } message;
        
} idmef_message_t;

#endif







