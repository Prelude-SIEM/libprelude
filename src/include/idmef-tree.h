/*****
*
* Copyright (C) 2001, 2002 Yoann Vandoorselaere <yoann@prelude-ids.org>
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

/* IMPORTANT NOTE: pass this file to Perl script after preprocessing with
                   cpp -D_GENERATE -E idmef-tree-input.h */

/*
 * NOTE ON GENERATION TIME MACROS:
 *  
 *  - IS_LISTED: use within object that will be a member of the list
 *    always put this field as the first field of the struct, idmef-object.c
 *    access idmef structure as a void * and therefore cannot use list_entry
 *    to retrieve the list entry, thus it assumes that the struct list_head
 *    is the first field of the struct
 *
 *  - LISTED_OBJECT(name, type): include list named 'name' consisted of objects 
 *    named 'type' (objects must have IS_LISTED member). 'name' should end with '_list'. 
 *
 *  - UNION_MEMBER(union, var, val, type, name): use within 'union' clause. 
 *    Parameters: 'union' - name of the union macro is used within 
 *                'var' - variable controlling selection of memeber form the union
 *                'val' - value of variable 'var' for which this member will be used
 *                'type' - type of member variable
 *                'name' - name of member variable    
 *
 * - FORCE_REGISTER(type, class): force parser to treat type 'type' as a 'class', 
 *   even if it was not (yet) defined; example: FORCE_REGISTER(my_struct_t, struct)
 *   registers my_struct_t as struct. 
 *
 * - TYPE_ID(type, id): set ID number of type 'type' to 'id'
 */


#ifndef _LIBPRELUDE_IDMEF_TREE_H
#define _LIBPRELUDE_IDMEF_TREE_H

#ifndef _GENERATE

#include <inttypes.h>

#include "prelude-hash.h"

#include "prelude-io.h"
#include "prelude-message.h"

#define LISTED_OBJECT(name, type) struct list_head name

#define IS_LISTED struct list_head list

#define	UNION(type, var) type var; union

#define	UNION_MEMBER(value, type, name) type name

#define ENUM(...) typedef enum

#define PRE_DECLARE(type, class)

#define TYPE_ID(type, id) type

#define PRIMITIVE_TYPE(type)
#define PRIMITIVE_TYPE_STRUCT(type)

#define HIDE(type, name) type name

#define REFCOUNT int refcount

#endif /* _GENERATE */

/*
 * Default value for an enumeration member should always be 0.
 * If there is no default value, no enumeration member should use
 * the 0 value
 */

#define IDMEF_VERSION "0.6"

#ifndef _GENERATE
#include "idmef-string.h"
#include "idmef-time.h"
#include "idmef-data.h"
#endif

PRIMITIVE_TYPE(void)
PRIMITIVE_TYPE(int16_t)
PRIMITIVE_TYPE(int32_t)
PRIMITIVE_TYPE(int64_t)
PRIMITIVE_TYPE(uint16_t)
PRIMITIVE_TYPE(uint32_t)
PRIMITIVE_TYPE(uint64_t)
PRIMITIVE_TYPE(uchar_t)
PRIMITIVE_TYPE(float)

PRIMITIVE_TYPE_STRUCT(idmef_string_t)
PRIMITIVE_TYPE_STRUCT(idmef_time_t)
PRIMITIVE_TYPE_STRUCT(idmef_data_t)


/*
 * Additional Data class
 */
ENUM() {
        string    = 0,
        boolean   = 1,
        byte      = 2,
        character = 3,
        date_time = 4,
        integer   = 5,
        ntpstamp  = 6,
        portlist  = 7,
        real      = 8,
        xml       = 9
} TYPE_ID(idmef_additional_data_type_t, 3);



typedef struct {
        IS_LISTED;
	REFCOUNT;
        idmef_additional_data_type_t type;
        idmef_string_t meaning;
	idmef_data_t data;
} TYPE_ID(idmef_additional_data_t, 4);



/*
 * Classification class
 */
ENUM(origin) {
        origin_unknown  = 0,
        bugtraqid       = 1,
        cve             = 2,
        vendor_specific = 3
} TYPE_ID(idmef_classification_origin_t, 5);



typedef struct {
        IS_LISTED;
	REFCOUNT;
        idmef_classification_origin_t origin;
        idmef_string_t name;
        idmef_string_t url;
} TYPE_ID(idmef_classification_t, 6);





/*
 * UserId class
 */
ENUM() {
        original_user = 0,
        current_user  = 1,
        target_user   = 2,
        user_privs    = 3,
        current_group = 4,
        group_privs   = 5,
        other_privs   = 6
} TYPE_ID(idmef_userid_type_t, 7);


typedef struct {
        IS_LISTED;
	REFCOUNT;
        uint64_t ident;
        idmef_userid_type_t type;
        idmef_string_t name;
        uint32_t number;
} TYPE_ID(idmef_userid_t, 8);






/*
 * User class
 */
ENUM(cat) {
        cat_unknown  = 0,
        application  = 1,
        os_device    = 2
} TYPE_ID(idmef_user_category_t, 9);



typedef struct {
	REFCOUNT;
        uint64_t ident;
        idmef_user_category_t category;
        LISTED_OBJECT(userid_list, idmef_userid_t);
} TYPE_ID(idmef_user_t, 10);




/*
 * Address class
 */
ENUM(addr) {
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
        ipv6_net_mask = 14
} TYPE_ID(idmef_address_category_t, 11);




typedef struct {
        IS_LISTED;
        REFCOUNT;
        uint64_t ident;
        idmef_address_category_t category;
        idmef_string_t vlan_name;
        uint32_t vlan_num;
        idmef_string_t address;
        idmef_string_t netmask;
} TYPE_ID(idmef_address_t, 12);



/*
 * Process class
 */

typedef struct {
	REFCOUNT;
        uint64_t ident;
        idmef_string_t name;
        uint32_t pid;
        idmef_string_t path;

        LISTED_OBJECT(arg_list, idmef_string_t);
        LISTED_OBJECT(env_list, idmef_string_t);
} TYPE_ID(idmef_process_t, 13);



typedef struct {
	REFCOUNT;
        idmef_string_t url;
        idmef_string_t cgi;
        idmef_string_t http_method;
        LISTED_OBJECT(arg_list, idmef_string_t);
} TYPE_ID(idmef_webservice_t, 14);




/*
 * SNMPService class
 */
typedef struct {
	REFCOUNT;
        idmef_string_t oid;
        idmef_string_t community;
        idmef_string_t command;
} TYPE_ID(idmef_snmpservice_t, 15);

        

ENUM() {
        no_specific_service = 0,
        web_service = 1,
        snmp_service = 2
} TYPE_ID(idmef_service_type_t, 16);



/*
 * Service class
 */
typedef struct {
	REFCOUNT;
        uint64_t ident;
        idmef_string_t name;
        uint16_t port;
        idmef_string_t portlist;
        idmef_string_t protocol;

	UNION(idmef_service_type_t, type) {
		UNION_MEMBER(web_service, idmef_webservice_t, *web);
		UNION_MEMBER(snmp_service, idmef_snmpservice_t, *snmp);
	} specific;
        
} TYPE_ID(idmef_service_t, 17);




/*
 * Node class
 */
ENUM(node) {
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
        wfw          = 12
} TYPE_ID(idmef_node_category_t, 18);


typedef struct {
	REFCOUNT;
        uint64_t ident;
        idmef_node_category_t category;
        idmef_string_t location;
        idmef_string_t name;
        LISTED_OBJECT(address_list, idmef_address_t);
} TYPE_ID(idmef_node_t, 19);





/*
 * Source class
 */
ENUM() {
        unknown = 0,
        yes     = 1,
        no      = 2
} TYPE_ID(idmef_spoofed_t, 20);


typedef struct {
        IS_LISTED;
	REFCOUNT;

        uint64_t ident;
        idmef_spoofed_t spoofed;
        idmef_string_t interface;

        idmef_node_t *node;
        idmef_user_t *user;
        idmef_process_t *process;
        idmef_service_t *service;
        
} TYPE_ID(idmef_source_t, 21);


/*
 * File Access class
 */
typedef struct {
        IS_LISTED;
	REFCOUNT;

        idmef_userid_t userid;
        LISTED_OBJECT(permission_list, idmef_string_t);
} TYPE_ID(idmef_file_access_t, 22);



/*
 * Inode class
 */
typedef struct {
	REFCOUNT;
        idmef_time_t *change_time;
        uint32_t number;
        uint32_t major_device;
        uint32_t minor_device;
        uint32_t c_major_device;
        uint32_t c_minor_device;
} TYPE_ID(idmef_inode_t, 23);


/* chicken-and-egg problem between idmef_linkage_t and idmef_file_t */

PRE_DECLARE(idmef_linkage_t, struct)


/*
 * File class
 */
ENUM() {
        current  = 1,
        original = 2
} TYPE_ID(idmef_file_category_t, 24);
        
        
typedef struct {
        IS_LISTED;
	REFCOUNT;
        
        uint64_t ident;
        idmef_file_category_t category;
        idmef_string_t fstype;

        idmef_string_t name;
        idmef_string_t path;

        idmef_time_t *create_time;
        idmef_time_t *modify_time;
        idmef_time_t *access_time;

        uint32_t data_size;
        uint32_t disk_size;

        LISTED_OBJECT(file_access_list, idmef_file_access_t);
        LISTED_OBJECT(file_linkage_list, idmef_linkage_t);

        idmef_inode_t *inode;
} TYPE_ID(idmef_file_t, 25);


/*
 * Linkage class
 */
ENUM() {
        hard_link     = 1,
        mount_point   = 2,
        reparse_point = 3,
        shortcut      = 4,
        stream        = 5,
        symbolic_link = 6
} TYPE_ID(idmef_linkage_category_t, 26);


typedef struct {
        IS_LISTED;
	REFCOUNT;
        
        idmef_linkage_category_t category;
        idmef_string_t name;
        idmef_string_t path;
        idmef_file_t *file;
} TYPE_ID(idmef_linkage_t, 27);




/*
 * Target class
 */
typedef struct {
        IS_LISTED;
	REFCOUNT;
        
        uint64_t ident;
        idmef_spoofed_t decoy;
        idmef_string_t interface;

        idmef_node_t *node;
        idmef_user_t *user;
        idmef_process_t *process;
        idmef_service_t *service;
        LISTED_OBJECT(file_list, idmef_file_t);
} TYPE_ID(idmef_target_t, 28);





/*
 * Analyzer class
 */
typedef struct {
	REFCOUNT;

        uint64_t analyzerid;
        idmef_string_t manufacturer;
        idmef_string_t model;
        idmef_string_t version;
        idmef_string_t class;
        idmef_string_t ostype;
        idmef_string_t osversion;
        
        idmef_node_t *node;
        idmef_process_t *process;
} TYPE_ID(idmef_analyzer_t, 29);



/*
 * AlertIdent class
 */

typedef struct {
        IS_LISTED;
	REFCOUNT;

        uint64_t alertident;
        uint64_t analyzerid;
} TYPE_ID(idmef_alertident_t, 30);



/*
 * Impact class
 */
ENUM(impact) {
        impact_low    = 1,
        impact_medium = 2,
        impact_high   = 3
} TYPE_ID(idmef_impact_severity_t, 31);


ENUM() {
        failed     = 1,
        succeeded  = 2
} TYPE_ID(idmef_impact_completion_t, 32);


ENUM() {
        other      = 0,
        admin      = 1,
        dos        = 2,
        file       = 3,
        recon      = 4,
        user       = 5
} TYPE_ID(idmef_impact_type_t, 33);


typedef struct {
	REFCOUNT;

        idmef_impact_severity_t severity;
        idmef_impact_completion_t completion;
        idmef_impact_type_t type;
        idmef_string_t description;
} TYPE_ID(idmef_impact_t, 34);


/*
 * Action class
 */
ENUM(action) {
        action_other       = 0,
        block_installed    = 1,
        notification_sent  = 2,
        taken_offline      = 3
} TYPE_ID(idmef_action_category_t, 35);


typedef struct {
        IS_LISTED;
	REFCOUNT;

        idmef_action_category_t category;
        idmef_string_t description;
} TYPE_ID(idmef_action_t, 36);



/*
 * Confidence class
 */
ENUM() {
        numeric = 0,
        low     = 1,
        medium  = 2,
        high    = 3
} TYPE_ID(idmef_confidence_rating_t, 37);


typedef struct {
	REFCOUNT;

        idmef_confidence_rating_t rating;
        float confidence;
} TYPE_ID(idmef_confidence_t, 38);


/*
 * Assessment class
 */
typedef struct {
	REFCOUNT;

        idmef_impact_t *impact;
        LISTED_OBJECT(action_list, idmef_action_t);
        idmef_confidence_t *confidence;
} TYPE_ID(idmef_assessment_t, 39);



/*
 * Toolalert class
 */
typedef struct {
	REFCOUNT;

        idmef_string_t name;
        idmef_string_t command;
        LISTED_OBJECT(alertident_list, idmef_alertident_t);
} TYPE_ID(idmef_tool_alert_t, 40);





/*
 * CorrelationAlert class
 */
typedef struct {
	REFCOUNT;

        idmef_string_t name;
        LISTED_OBJECT(alertident_list, idmef_alertident_t);
} TYPE_ID(idmef_correlation_alert_t, 41);




/*
 * OverflowAlert class
 */
typedef struct {
	REFCOUNT;

        idmef_string_t program;
        uint32_t *size;
        idmef_data_t *buffer;
} TYPE_ID(idmef_overflow_alert_t, 42);




/*
 * Alert class
 */
ENUM(idmef) {
        idmef_default           = 0,
        idmef_tool_alert        = 1,
        idmef_correlation_alert = 2,
        idmef_overflow_alert    = 3
} TYPE_ID(idmef_alert_type_t, 43);



typedef struct {
        uint64_t ident;

        idmef_assessment_t *assessment;
    
        idmef_analyzer_t *analyzer;
    
        idmef_time_t create_time;
        idmef_time_t *detect_time;
        idmef_time_t *analyzer_time;

        LISTED_OBJECT(source_list, idmef_source_t);
        LISTED_OBJECT(target_list, idmef_target_t);
        LISTED_OBJECT(classification_list, idmef_classification_t);
        LISTED_OBJECT(additional_data_list, idmef_additional_data_t);

        UNION(idmef_alert_type_t, type) {
                UNION_MEMBER(idmef_tool_alert, idmef_tool_alert_t, *tool_alert);
                UNION_MEMBER(idmef_correlation_alert, idmef_correlation_alert_t, *correlation_alert);
                UNION_MEMBER(idmef_overflow_alert, idmef_overflow_alert_t, *overflow_alert);
        } detail;
        
} TYPE_ID(idmef_alert_t, 44);





/*
 * Heartbeat class
 */
typedef struct {
        uint64_t ident;
        idmef_analyzer_t *analyzer;

        idmef_time_t create_time;
        idmef_time_t *analyzer_time;

        LISTED_OBJECT(additional_data_list, idmef_additional_data_t);
} TYPE_ID(idmef_heartbeat_t, 45);




/*
 * IDMEF Message class
 */
ENUM() {
        idmef_alert_message     = 1,
        idmef_heartbeat_message = 2
} TYPE_ID(idmef_message_type_t, 46);


typedef struct {        

        idmef_string_t version;

        UNION(idmef_message_type_t, type) {
		UNION_MEMBER(idmef_alert_message, idmef_alert_t, *alert);
		UNION_MEMBER(idmef_heartbeat_message, idmef_heartbeat_t, *heartbeat);
        } message;

	HIDE(prelude_hash_t *, cache);
	HIDE(prelude_msg_t *, pmsg);
        
} TYPE_ID(idmef_message_t, 47);

#endif /* _LIBPRELUDE_IDMEF_TREE_H */
