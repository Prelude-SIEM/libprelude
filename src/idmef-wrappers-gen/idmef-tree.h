/*****
*
* Copyright (C) 2001, 2002, 2004 Yoann Vandoorselaere <yoann@prelude-ids.org>
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


/*
 * NOTE ON GENERATION TIME MACROS:
 *  
 *  - IS_LISTED: use within object that will be a member of the list
 *    always put this field as the first field of the struct, idmef-object.c
 *    access idmef structure as a void * and therefore cannot use list_entry
 *    to retrieve the list entry, thus it assumes that the prelude_list_t
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

#include "prelude-inttypes.h"

#include "prelude-hash.h"

#include "prelude-io.h"
#include "prelude-message.h"

#define LISTED_OBJECT(name, type) prelude_list_t name

#define IS_LISTED prelude_list_t list

#define	UNION(type, var) type var; union

#define	UNION_MEMBER(value, type, name) type name

#define ENUM(...) typedef enum

#define PRE_DECLARE(type, class)

#define TYPE_ID(type, id) type

#define PRIMITIVE_TYPE(type)
#define PRIMITIVE_TYPE_STRUCT(type)

#define HIDE(type, name) type name

#define REFCOUNT int refcount

#define DYNAMIC_IDENT(x) uint64_t x


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
        IDMEF_ADDITIONAL_DATA_TYPE_ERROR      = -1,
        IDMEF_ADDITIONAL_DATA_TYPE_STRING     =  0,
        IDMEF_ADDITIONAL_DATA_TYPE_BOOLEAN    =  1,
        IDMEF_ADDITIONAL_DATA_TYPE_BYTE       =  2,
        IDMEF_ADDITIONAL_DATA_TYPE_CHARACTER  =  3,
        IDMEF_ADDITIONAL_DATA_TYPE_DATE_TIME  =  4,
        IDMEF_ADDITIONAL_DATA_TYPE_INTEGER    =  5,
        IDMEF_ADDITIONAL_DATA_TYPE_NTPSTAMP   =  6,
        IDMEF_ADDITIONAL_DATA_TYPE_PORTLIST   =  7,
        IDMEF_ADDITIONAL_DATA_TYPE_REAL       =  8,
        IDMEF_ADDITIONAL_DATA_TYPE_XML        =  9
} TYPE_ID(idmef_additional_data_type_t, 3);



struct {
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
        IDMEF_CLASSIFICATION_ORIGIN_ERROR           = -1,
        IDMEF_CLASSIFICATION_ORIGIN_UNKNOWN         =  0,
        IDMEF_CLASSIFICATION_ORIGIN_BUGTRAQID       =  1,
        IDMEF_CLASSIFICATION_ORIGIN_CVE             =  2,
        IDMEF_CLASSIFICATION_ORIGIN_VENDOR_SPECIFIC =  3,
        IDMEF_CLASSIFICATION_ORIGIN_OSVDB           =  4,
} TYPE_ID(idmef_classification_origin_t, 5);



struct {
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
        IDMEF_USERID_TYPE_ORIGINAL_ERROR = -1,
        IDMEF_USERID_TYPE_ORIGINAL_USER  =  0,
        IDMEF_USERID_TYPE_CURRENT_USER   =  1,
        IDMEF_USERID_TYPE_TARGET_USER    =  2,
        IDMEF_USERID_TYPE_USER_PRIVS     =  3,
        IDMEF_USERID_TYPE_CURRENT_GROUP  =  4,
        IDMEF_USERID_TYPE_GROUP_PRIVS    =  5,
        IDMEF_USERID_TYPE_OTHER_PRIVS    =  6
} TYPE_ID(idmef_userid_type_t, 7);


struct {
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
        IDMEF_USER_CATEGORY_ERROR	 = -1,
        IDMEF_USER_CATEGORY_UNKNOWN      =  0,
        IDMEF_USER_CATEGORY_APPLICATION  =  1,
        IDMEF_USER_CATEGORY_OS_DEVICE    =  2
} TYPE_ID(idmef_user_category_t, 9);



struct {
	REFCOUNT;
        uint64_t ident;
        idmef_user_category_t category;
        LISTED_OBJECT(userid_list, idmef_userid_t);
} TYPE_ID(idmef_user_t, 10);




/*
 * Address class
 */
ENUM(addr) {
        IDMEF_ADDRESS_CATEGORY_ERROR	     = -1,
        IDMEF_ADDRESS_CATEGORY_UNKNOWN       =  0,
        IDMEF_ADDRESS_CATEGORY_ATM           =  1,
        IDMEF_ADDRESS_CATEGORY_E_MAIL        =  2,
        IDMEF_ADDRESS_CATEGORY_LOTUS_NOTES   =  3,
        IDMEF_ADDRESS_CATEGORY_MAC           =  4,
        IDMEF_ADDRESS_CATEGORY_SNA           =  5,
        IDMEF_ADDRESS_CATEGORY_VM            =  6,
        IDMEF_ADDRESS_CATEGORY_IPV4_ADDR     =  7,
        IDMEF_ADDRESS_CATEGORY_IPV4_ADDR_HEX =  8,
        IDMEF_ADDRESS_CATEGORY_IPV4_NET      =  9,
        IDMEF_ADDRESS_CATEGORY_IPV4_NET_MASK = 10,
        IDMEF_ADDRESS_CATEGORY_IPV6_ADDR     = 11,
        IDMEF_ADDRESS_CATEGORY_IPV6_ADDR_HEX = 12,
        IDMEF_ADDRESS_CATEGORY_IPV6_NET      = 13,
        IDMEF_ADDRESS_CATEGORY_IPV6_NET_MASK = 14,
} TYPE_ID(idmef_address_category_t, 11);




struct {
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

struct {
	REFCOUNT;
        uint64_t ident;
        idmef_string_t name;
        uint32_t pid;
        idmef_string_t path;

        LISTED_OBJECT(arg_list, idmef_string_t);
        LISTED_OBJECT(env_list, idmef_string_t);
} TYPE_ID(idmef_process_t, 13);



struct {
	REFCOUNT;
        idmef_string_t url;
        idmef_string_t cgi;
        idmef_string_t http_method;
        LISTED_OBJECT(arg_list, idmef_string_t);
} TYPE_ID(idmef_webservice_t, 14);




/*
 * SNMPService class
 */
struct {
	REFCOUNT;
        idmef_string_t oid;
        idmef_string_t community;
        idmef_string_t command;
} TYPE_ID(idmef_snmpservice_t, 15);

        

ENUM() {
        IDMEF_SERVICE_TYPE_ERROR   = -1,
        IDMEF_SERVICE_TYPE_DEFAULT =  0,
        IDMEF_SERVICE_TYPE_WEB     =  1,
        IDMEF_SERVICE_TYPE_SNMP    =  2
} TYPE_ID(idmef_service_type_t, 16);



/*
 * Service class
 */
struct {
	REFCOUNT;
        uint64_t ident;
        idmef_string_t name;
        uint16_t port;
        idmef_string_t portlist;
        idmef_string_t protocol;

	UNION(idmef_service_type_t, type) {
		UNION_MEMBER(IDMEF_SERVICE_TYPE_WEB, idmef_webservice_t, *web);
		UNION_MEMBER(IDMEF_SERVICE_TYPE_SNMP, idmef_snmpservice_t, *snmp);
	} specific;
        
} TYPE_ID(idmef_service_t, 17);




/*
 * Node class
 */
ENUM(node) {
        IDMEF_NODE_CATEGORY_ERROR	 = -1,        
        IDMEF_NODE_CATEGORY_UNKNOWN      =  0,
        IDMEF_NODE_CATEGORY_ADS          =  1,
        IDMEF_NODE_CATEGORY_AFS          =  2,
        IDMEF_NODE_CATEGORY_CODA         =  3,
        IDMEF_NODE_CATEGORY_DFS          =  4,
        IDMEF_NODE_CATEGORY_DNS          =  5,
        IDMEF_NODE_CATEGORY_HOSTS        =  6,
        IDMEF_NODE_CATEGORY_KERBEROS     =  7,
        IDMEF_NODE_CATEGORY_NDS          =  8,
        IDMEF_NODE_CATEGORY_NIS          =  9,
        IDMEF_NODE_CATEGORY_NISPLUS      =  10,
        IDMEF_NODE_CATEGORY_NT           =  11,
        IDMEF_NODE_CATEGORY_WFW          =  12
} TYPE_ID(idmef_node_category_t, 18);


struct {
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
        IDMEF_SOURCE_SPOOFED_ERROR   = -1,
        IDMEF_SOURCE_SPOOFED_UNKNOWN =  0,
        IDMEF_SOURCE_SPOOFED_YES     =  1,
        IDMEF_SOURCE_SPOOFED_NO      =  2
} TYPE_ID(idmef_source_spoofed_t, 20);


struct {
        IS_LISTED;
	REFCOUNT;

        uint64_t ident;
        idmef_source_spoofed_t spoofed;
        idmef_string_t interface;

        idmef_node_t *node;
        idmef_user_t *user;
        idmef_process_t *process;
        idmef_service_t *service;
        
} TYPE_ID(idmef_source_t, 21);


/*
 * File Access class
 */



struct {
        IS_LISTED;
	REFCOUNT;
        
        idmef_userid_t userid;
        LISTED_OBJECT(permission_list, idmef_string_t);
} TYPE_ID(idmef_file_access_t, 22);



/*
 * Inode class
 */
struct {
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
        IDMEF_FILE_CATEGORY_ERROR    = -1,
        IDMEF_FILE_CATEGORY_CURRENT  =  1,
        IDMEF_FILE_CATEGORY_ORIGINAL =  2
} TYPE_ID(idmef_file_category_t, 24);


ENUM() {
        IDMEF_FILE_FSTYPE_ERROR   = -1,
        IDMEF_FILE_FSTYPE_UFS     =  1,
        IDMEF_FILE_FSTYPE_EFS     =  2,
        IDMEF_FILE_FSTYPE_NFS     =  3,
        IDMEF_FILE_FSTYPE_AFS     =  4,
        IDMEF_FILE_FSTYPE_NTFS    =  5,
        IDMEF_FILE_FSTYPE_FAT16   =  6,
        IDMEF_FILE_FSTYPE_FAT32   =  7,
        IDMEF_FILE_FSTYPE_PCFS    =  8,
        IDMEF_FILE_FSTYPE_JOLIET  =  9,
        IDMEF_FILE_FSTYPE_ISO9660 = 10,
} TYPE_ID(idmef_file_fstype_t, 25);



struct {
        IS_LISTED;
	REFCOUNT;
        
        uint64_t ident;
        idmef_file_category_t category;
        idmef_file_fstype_t fstype;

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
} TYPE_ID(idmef_file_t, 26);


/*
 * Linkage class
 */
ENUM() {
        IDMEF_LINKAGE_CATEGORY_ERROR	     = -1,
        IDMEF_LINKAGE_CATEGORY_HARD_LINK     =  1,
        IDMEF_LINKAGE_CATEGORY_MOUNT_POINT   =  2,
        IDMEF_LINKAGE_CATEGORY_REPARSE_POINT =  3,
        IDMEF_LINKAGE_CATEGORY_SHORTCUT      =  4,
        IDMEF_LINKAGE_CATEGORY_STREAM        =  5,
        IDMEF_LINKAGE_CATEGORY_SYMBOLIC_LINK =  6
} TYPE_ID(idmef_linkage_category_t, 27);


struct {
        IS_LISTED;
	REFCOUNT;
        
        idmef_linkage_category_t category;
        idmef_string_t name;
        idmef_string_t path;
        idmef_file_t *file;
} TYPE_ID(idmef_linkage_t, 28);




/*
 * Target class
 */

ENUM() {
        IDMEF_TARGET_DECOY_ERROR   = -1,
        IDMEF_TARGET_DECOY_UNKNOWN =  0,
        IDMEF_TARGET_DECOY_YES     =  1,
        IDMEF_TARGET_DECOY_NO      =  2
} TYPE_ID(idmef_target_decoy_t, 29);


struct {
        IS_LISTED;
	REFCOUNT;
        
        uint64_t ident;
        idmef_target_decoy_t decoy;
        idmef_string_t interface;

        idmef_node_t *node;
        idmef_user_t *user;
        idmef_process_t *process;
        idmef_service_t *service;
        LISTED_OBJECT(file_list, idmef_file_t);
} TYPE_ID(idmef_target_t, 30);





/*
 * Analyzer class
 */
struct {
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
        struct idmef_analyzer *analyzer;
        
} TYPE_ID(idmef_analyzer_t, 31);



/*
 * AlertIdent class
 */

struct {
        IS_LISTED;
	REFCOUNT;

        uint64_t alertident;
        uint64_t analyzerid;
} TYPE_ID(idmef_alertident_t, 32);



/*
 * Impact class
 */
ENUM(impact) {
        IDMEF_IMPACT_SEVERITY_ERROR  = -1,
        IDMEF_IMPACT_SEVERITY_LOW    =  1,
        IDMEF_IMPACT_SEVERITY_MEDIUM =  2,
        IDMEF_IMPACT_SEVERITY_HIGH   =  3,
        IDMEF_IMPACT_SEVERITY_INFO   =  4,
} TYPE_ID(idmef_impact_severity_t, 33);


ENUM() {
        IDMEF_IMPACT_COMPLETION_ERROR	   = -1,
        IDMEF_IMPACT_COMPLETION_FAILED     =  1,
        IDMEF_IMPACT_COMPLETION_SUCCEEDED  =  2
} TYPE_ID(idmef_impact_completion_t, 34);


ENUM() {
        IDMEF_IMPACT_TYPE_ERROR	     = -1,
        IDMEF_IMPACT_TYPE_OTHER      =  0,
        IDMEF_IMPACT_TYPE_ADMIN      =  1,
        IDMEF_IMPACT_TYPE_DOS        =  2,
        IDMEF_IMPACT_TYPE_FILE       =  3,
        IDMEF_IMPACT_TYPE_RECON      =  4,
        IDMEF_IMPACT_TYPE_USER       =  5
} TYPE_ID(idmef_impact_type_t, 35);


struct {
	REFCOUNT;

        idmef_impact_severity_t severity;
        idmef_impact_completion_t completion;
        idmef_impact_type_t type;
        idmef_string_t description;
} TYPE_ID(idmef_impact_t, 36);


/*
 * Action class
 */
ENUM(action) {
        IDMEF_ACTION_CATEGORY_ERROR	         = -1,
        IDMEF_ACTION_CATEGORY_OTHER              =  0,
        IDMEF_ACTION_CATEGORY_BLOCK_INSTALLED    =  1,
        IDMEF_ACTION_CATEGORY_NOTIFICATION_SENT  =  2,
        IDMEF_ACTION_CATEGORY_TAKEN_OFFLINE      =  3
} TYPE_ID(idmef_action_category_t, 37);


struct {
        IS_LISTED;
	REFCOUNT;

        idmef_action_category_t category;
        idmef_string_t description;
} TYPE_ID(idmef_action_t, 38);



/*
 * Confidence class
 */
ENUM() {
        IDMEF_CONFIDENCE_RATING_ERROR   = -1,
        IDMEF_CONFIDENCE_RATING_NUMERIC =  0,
        IDMEF_CONFIDENCE_RATING_LOW     =  1,
        IDMEF_CONFIDENCE_RATING_MEDIUM  =  2,
        IDMEF_CONFIDENCE_RATING_HIGH    =  3
} TYPE_ID(idmef_confidence_rating_t, 39);


struct {
	REFCOUNT;

        idmef_confidence_rating_t rating;
        float confidence;
} TYPE_ID(idmef_confidence_t, 40);


/*
 * Assessment class
 */
struct {
	REFCOUNT;

        idmef_impact_t *impact;
        LISTED_OBJECT(action_list, idmef_action_t);
        idmef_confidence_t *confidence;
} TYPE_ID(idmef_assessment_t, 41);



/*
 * Toolalert class
 */
struct {
	REFCOUNT;

        idmef_string_t name;
        idmef_string_t command;
        LISTED_OBJECT(alertident_list, idmef_alertident_t);
} TYPE_ID(idmef_tool_alert_t, 42);





/*
 * CorrelationAlert class
 */
struct {
	REFCOUNT;

        idmef_string_t name;
        LISTED_OBJECT(alertident_list, idmef_alertident_t);
} TYPE_ID(idmef_correlation_alert_t, 43);




/*
 * OverflowAlert class
 */
struct {
	REFCOUNT;

        idmef_string_t program;
        uint32_t *size;
        idmef_data_t *buffer;
} TYPE_ID(idmef_overflow_alert_t, 44);




/*
 * Alert class
 */
ENUM(idmef) {
        IDMEF_ALERT_TYPE_ERROR             = -1,
        IDMEF_ALERT_TYPE_DEFAULT           =  0,
        IDMEF_ALERT_TYPE_TOOL              =  1,
        IDMEF_ALERT_TYPE_CORRELATION       =  2,
        IDMEF_ALERT_TYPE_OVERFLOW          =  3
} TYPE_ID(idmef_alert_type_t, 45);



struct {
        DYNAMIC_IDENT(ident);

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
                UNION_MEMBER(IDMEF_ALERT_TYPE_TOOL, idmef_tool_alert_t, *tool_alert);
                UNION_MEMBER(IDMEF_ALERT_TYPE_CORRELATION, idmef_correlation_alert_t, *correlation_alert);
                UNION_MEMBER(IDMEF_ALERT_TYPE_OVERFLOW, idmef_overflow_alert_t, *overflow_alert);
        } detail;
        
} TYPE_ID(idmef_alert_t, 46);





/*
 * Heartbeat class
 */
struct {
        DYNAMIC_IDENT(ident);
        idmef_analyzer_t *analyzer;

        idmef_time_t create_time;
        idmef_time_t *analyzer_time;

        LISTED_OBJECT(additional_data_list, idmef_additional_data_t);
} TYPE_ID(idmef_heartbeat_t, 47);




/*
 * IDMEF Message class
 */
ENUM() {
        IDMEF_MESSAGE_TYPE_ERROR     = -1,
        IDMEF_MESSAGE_TYPE_ALERT     =  1,
        IDMEF_MESSAGE_TYPE_HEARTBEAT =  2
} TYPE_ID(idmef_message_type_t, 48);


struct {        
        REFCOUNT;
        
        idmef_string_t version;

        UNION(idmef_message_type_t, type) {
		UNION_MEMBER(IDMEF_MESSAGE_TYPE_ALERT, idmef_alert_t, *alert);
		UNION_MEMBER(IDMEF_MESSAGE_TYPE_HEARTBEAT, idmef_heartbeat_t, *heartbeat);
        } message;

	HIDE(prelude_msg_t *, pmsg);
        
} TYPE_ID(idmef_message_t, 49);

#endif /* _LIBPRELUDE_IDMEF_TREE_H */
