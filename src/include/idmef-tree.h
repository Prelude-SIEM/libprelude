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


#ifndef IDMEF_TREE_H
#define IDMEF_TREE_H

#include <inttypes.h>

#define IDMEF_VERSION "0.5"




/*
 * Time class
 */
typedef struct {
        uint32_t sec;
        uint32_t usec;
} idmef_time_t;

#define idmef_create_time_t idmef_time_t
#define idmef_detect_time_t idmef_time_t
#define idmef_analyzer_time_t idmef_time_t




/*
 * Additional Data class
 */
typedef enum {
        string    = 0,
        byte      = 1,
        character = 2,
        date_time = 3,
        integer   = 4,
        ntpstamp  = 5,
        portlist  = 6,
        real      = 7,
        boolean   = 8,
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
        other_privs   = 6,
} idmef_userid_type_t;


typedef struct {
        struct list_head list;

        uint64_t ident;
        idmef_userid_type_t type;
        const char *name;
        uint32_t number;
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
        uint64_t ident;
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
        
        uint64_t ident;
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
        const char *string;
        struct list_head list;
} idmef_string_t;

#define idmef_process_env_t idmef_string_t
#define idmef_process_arg_t idmef_string_t

typedef struct {
        uint64_t ident;
        const char *name;
        uint32_t pid;
        const char *path;

        struct list_head arg_list;
        struct list_head env_list;
} idmef_process_t;



/*
 * WebService class
 */
typedef struct {
        const char *url;
        const char *cgi;
        const char *http_method;
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

        

typedef enum {
        default_service = 0,
        web_service = 1,
        snmp_service = 2,
} idmef_service_type_t;



/*
 * Service class
 */
typedef struct {
        uint64_t ident;
        const char *name;
        uint16_t port;
        const char *portlist;
        const char *protocol;

        idmef_service_type_t type;
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
        hosts        = 6,
        kerberos     = 7,
        nds          = 8,
        nis          = 9,
        nisplus      = 10,
        nt           = 11,
        wfw          = 12,
} idmef_node_category_t;


typedef struct {
        uint64_t ident;
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
    
        uint64_t ident;
        idmef_spoofed_t spoofed;
        const char *interface;

        idmef_node_t *node;
        idmef_user_t *user;
        idmef_process_t *process;
        idmef_service_t *service;
        
} idmef_source_t;


/*
 * File Access class
 */
typedef struct {
        struct list_head list;
        idmef_userid_t userid;
        const char *permission;
} idmef_file_access_t;


/*
 * Linkage class
 */
typedef enum {
        hard_link     = 0,
        mount_point   = 1,
        reparse_point = 2,
        shortcut      = 3,
        stream        = 4,
        symbolic_link = 5,
} idmef_linkage_category_t;


typedef struct {
        struct list_head list;
        
        idmef_linkage_category_t category;
        const char *name;
        const char *path;
        struct idmef_file *file;
} idmef_linkage_t;



/*
 * Inode class
 */
typedef struct {
        idmef_time_t change_time;
        uint32_t number;
        uint32_t major_device;
        uint32_t minor_device;
        uint32_t c_major_device;
        uint32_t c_minor_device;
} idmef_inode_t;



        
/*
 * File class
 */
typedef enum {
        current  = 0,
        original = 1,
} idmef_file_category_t;
        
        
typedef struct idmef_file {
        struct list_head list;
        
        uint64_t ident;
        idmef_file_category_t category;
        const char *fstype;

        const char *name;
        const char *path;

        idmef_time_t *create_time;
        idmef_time_t *modify_time;
        idmef_time_t *access_time;

        uint32_t data_size;
        uint32_t disk_size;

        struct list_head file_access_list;
        struct list_head file_linkage_list;

        idmef_inode_t *inode;
} idmef_file_t;



/*
 * Target class
 */
typedef struct {
        struct list_head list;
        
        uint64_t ident;
        idmef_spoofed_t decoy;
        const char *interface;

        idmef_node_t *node;
        idmef_user_t *user;
        idmef_process_t *process;
        idmef_service_t *service;
        struct list_head file_list;
} idmef_target_t;





/*
 * Analyzer class
 */
typedef struct {
        uint64_t analyzerid;
        const char *manufacturer;
        const char *model;
        const char *version;
        const char *class;
        const char *ostype;
        const char *osversion;
        
        idmef_node_t *node;
        idmef_process_t *process;
} idmef_analyzer_t;



/*
 *
 */

typedef struct {
        struct list_head list;
        uint64_t alertident;
        uint64_t analyzerid;
} idmef_alertident_t;



/*
 * Impact class
 */
typedef enum {
        impact_low    = 0,
        impact_medium = 1,
        impact_high   = 2,
} idmef_impact_severity_t;


typedef enum {
        unknown_completion = 0,
        failed     = 1,
        succeeded  = 2,
} idmef_impact_completion_t;


typedef enum {
        admin      = 0,
        dos        = 1,
        file       = 2,
        recon      = 3,
        user       = 4,
        other      = 5,
} idmef_impact_type_t;


typedef struct {
        idmef_impact_severity_t severity;
        idmef_impact_completion_t completion;
        idmef_impact_type_t type;
        const char *description;
} idmef_impact_t;


/*
 * Action class
 */
typedef enum {
        block_installed    = 0,
        notification_sent  = 1,
        taken_offline      = 2,
        action_other       = 3,
} idmef_action_category_t;


typedef struct {
        struct list_head list;
        idmef_action_category_t category;
        const char *description;
} idmef_action_t;



/*
 * Confidence class
 */
typedef enum {
        low     = 0,
        medium  = 1,
        high    = 2,
        numeric = 3,
} idmef_confidence_rating_t;


typedef struct {
        idmef_confidence_rating_t rating;
        float confidence;
} idmef_confidence_t;


/*
 * Assessment class
 */
typedef struct {
        idmef_impact_t *impact;
        struct list_head action_list;
        idmef_confidence_t *confidence;
} idmef_assessment_t;



/*
 * Toolalert class
 */
typedef struct {
        const char *name;
        const char *command;
        struct list_head alertident_list;
} idmef_tool_alert_t;





/*
 * CorrelationAlert class
 */
typedef struct {
        const char *name;
        struct list_head alertident_list;
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
        idmef_default           = 0,
        idmef_tool_alert        = 1,
        idmef_correlation_alert = 2,
        idmef_overflow_alert    = 3,
} idmef_alert_type_t;



typedef enum {
        unknown_impact              = 0,
        bad_unknown                 = 1,
        not_suspicious              = 2,
        attempted_admin             = 3,
        successful_admin            = 4,
        attempted_dos               = 5,
        successful_dos              = 6,
        attempted_recon             = 7,
        successful_recon            = 8,
        successful_recon_limited    = 9,
        successful_recon_largescale = 10,
        attempted_user              = 11,
        successful_user             = 12,
} idmef_alert_impact_t;



typedef struct {
        uint64_t ident;

        idmef_assessment_t *assessment;
    
        idmef_analyzer_t analyzer;
    
        idmef_time_t create_time;
        idmef_time_t *detect_time;
        idmef_time_t *analyzer_time;

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
        uint64_t ident;
        idmef_analyzer_t analyzer;

        idmef_time_t create_time;
        idmef_time_t *analyzer_time;

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

        /*
         * specific to prelude
         */
        int msglen;
        int msgcount;
        

        /*
         * end of specific things.
         */
        const char *version;

        idmef_message_type_t type;
        union {
                idmef_alert_t *alert;
                idmef_heartbeat_t *heartbeat;
        } message;
        
} idmef_message_t;

#endif







