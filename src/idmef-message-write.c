
/*****
*
* Copyright (C) 2001-2005 Yoann Vandoorselaere <yoann@prelude-ids.org>
* Copyright (C) 2003-2005 Nicolas Delon <nicolas@prelude-ids.org>
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

/* Auto-generated by the GenerateIDMEFMessageWriteC package */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <assert.h>

#include "prelude-inttypes.h"
#include "prelude-list.h"
#include "prelude-log.h"
#include "prelude-io.h"
#include "prelude-ident.h"
#include "prelude-message-id.h"
#include "idmef-message-id.h"
#include "idmef.h"
#include "idmef-tree-wrap.h"
#include "idmef-message-write.h"
#include "prelude-client.h"
#include "common.h"



/*
 * If you wonder why we do this, and why life is complicated,
 * then wonder why the hell the guys that wrote IDMEF choose to use XML.
 * XML is dog slow. And XML'll never achieve performance needed for real time IDS.
 *
 * Here we are trying to communicate using a home made, binary version of IDMEF.
 */


static inline void prelude_string_write(prelude_string_t *string, prelude_msgbuf_t *msg, uint8_t tag)
{
        if ( ! string || prelude_string_is_empty(string) )
                return;

        prelude_msgbuf_set(msg, tag, prelude_string_get_len(string) + 1, prelude_string_get_string(string));
}



static inline void uint64_write(uint64_t data, prelude_msgbuf_t *msg, uint8_t tag) 
{
        uint64_t dst;
        
        dst = prelude_hton64(data);
        
        prelude_msgbuf_set(msg, tag, sizeof(dst), &dst);
}



static inline void uint32_write(uint32_t data, prelude_msgbuf_t *msg, uint8_t tag) 
{        
        data = htonl(data);
        prelude_msgbuf_set(msg, tag, sizeof(data), &data);
}



static inline void int32_write(uint32_t data, prelude_msgbuf_t *msg, uint8_t tag) 
{
	uint32_write(data, msg, tag);
}



static inline void uint8_write(uint8_t data, prelude_msgbuf_t *msg, uint8_t tag)
{
	prelude_msgbuf_set(msg, tag, sizeof (data), &data);
}



static inline void uint16_write(uint16_t data, prelude_msgbuf_t *msg, uint8_t tag) 
{
        data = htons(data);
        prelude_msgbuf_set(msg, tag, sizeof(data), &data);
}



static inline void float_write(float data, prelude_msgbuf_t *msg, uint8_t tag)
{
	if ( data == 0.0 )
		return;

	prelude_msgbuf_set(msg, tag, sizeof (data), &data);
}



inline void idmef_time_write(idmef_time_t *data, prelude_msgbuf_t *msg, uint8_t tag) 
{
        uint32_t tmp;
        unsigned char buf[12];

        if ( ! data )
                return;
      
        tmp = htonl(idmef_time_get_sec(data));
        memcpy(buf, &tmp, sizeof(tmp));

        tmp = htonl(idmef_time_get_usec(data));
        memcpy(buf + 4, &tmp, sizeof(tmp));

        tmp = htonl(idmef_time_get_gmt_offset(data));
        memcpy(buf + 8, &tmp, sizeof(tmp));

        prelude_msgbuf_set(msg, tag, sizeof (buf), buf);
}



static inline void idmef_data_write(idmef_data_t *data, prelude_msgbuf_t *msg, uint8_t tag)
{
	idmef_data_type_t type;

	if ( ! data )
		return;

	type = idmef_data_get_type(data);
	if ( type == IDMEF_DATA_TYPE_UNKNOWN )
		return;

	uint32_write(idmef_data_get_type(data), msg, tag);

	switch ( type ) {
	case IDMEF_DATA_TYPE_CHAR: case IDMEF_DATA_TYPE_BYTE:
		uint8_write(* (const uint8_t *) idmef_data_get_data(data), msg, tag);
		break;

	case IDMEF_DATA_TYPE_UINT32:
		uint32_write(idmef_data_get_uint32(data), msg, tag);
		break;

	case IDMEF_DATA_TYPE_UINT64:
		uint64_write(idmef_data_get_uint64(data), msg, tag);
		break;

	case IDMEF_DATA_TYPE_FLOAT:
		float_write(idmef_data_get_uint64(data), msg, tag);
		break;

	case IDMEF_DATA_TYPE_CHAR_STRING: case IDMEF_DATA_TYPE_BYTE_STRING:
		prelude_msgbuf_set(msg, tag, idmef_data_get_len(data), idmef_data_get_data(data));
		break;

	case IDMEF_DATA_TYPE_UNKNOWN:
		/* nop */;
	}
}

void idmef_additional_data_write(idmef_additional_data_t *additional_data, prelude_msgbuf_t *msg)
{
        if ( ! additional_data )
                return;

        prelude_msgbuf_set(msg, IDMEF_MSG_ADDITIONAL_DATA_TAG, 0, NULL);

        uint32_write(idmef_additional_data_get_type(additional_data), msg, IDMEF_MSG_ADDITIONAL_DATA_TYPE);
        prelude_string_write(idmef_additional_data_get_meaning(additional_data), msg, IDMEF_MSG_ADDITIONAL_DATA_MEANING);
        idmef_data_write(idmef_additional_data_get_data(additional_data), msg, IDMEF_MSG_ADDITIONAL_DATA_DATA);

        prelude_msgbuf_set(msg, IDMEF_MSG_END_OF_TAG, 0, NULL);
}


void idmef_reference_write(idmef_reference_t *reference, prelude_msgbuf_t *msg)
{
        if ( ! reference )
                return;

        prelude_msgbuf_set(msg, IDMEF_MSG_REFERENCE_TAG, 0, NULL);

        uint32_write(idmef_reference_get_origin(reference), msg, IDMEF_MSG_REFERENCE_ORIGIN);
        prelude_string_write(idmef_reference_get_name(reference), msg, IDMEF_MSG_REFERENCE_NAME);
        prelude_string_write(idmef_reference_get_url(reference), msg, IDMEF_MSG_REFERENCE_URL);
        prelude_string_write(idmef_reference_get_meaning(reference), msg, IDMEF_MSG_REFERENCE_MEANING);

        prelude_msgbuf_set(msg, IDMEF_MSG_END_OF_TAG, 0, NULL);
}


void idmef_classification_write(idmef_classification_t *classification, prelude_msgbuf_t *msg)
{
        if ( ! classification )
                return;

        prelude_msgbuf_set(msg, IDMEF_MSG_CLASSIFICATION_TAG, 0, NULL);

        prelude_string_write(idmef_classification_get_ident(classification), msg, IDMEF_MSG_CLASSIFICATION_IDENT);
        prelude_string_write(idmef_classification_get_text(classification), msg, IDMEF_MSG_CLASSIFICATION_TEXT);

        {
                idmef_reference_t *reference = NULL;

                while ( (reference = idmef_classification_get_next_reference(classification, reference)) ) {
                        idmef_reference_write(reference, msg);
                }
        }


        prelude_msgbuf_set(msg, IDMEF_MSG_END_OF_TAG, 0, NULL);
}


void idmef_user_id_write(idmef_user_id_t *user_id, prelude_msgbuf_t *msg)
{
        if ( ! user_id )
                return;

        prelude_msgbuf_set(msg, IDMEF_MSG_USER_ID_TAG, 0, NULL);

        prelude_string_write(idmef_user_id_get_ident(user_id), msg, IDMEF_MSG_USER_ID_IDENT);
        uint32_write(idmef_user_id_get_type(user_id), msg, IDMEF_MSG_USER_ID_TYPE);
        prelude_string_write(idmef_user_id_get_tty(user_id), msg, IDMEF_MSG_USER_ID_TTY);
        prelude_string_write(idmef_user_id_get_name(user_id), msg, IDMEF_MSG_USER_ID_NAME);

	{
		uint32_t *tmp;

		tmp = idmef_user_id_get_number(user_id);
		if ( tmp ) {
			uint32_write(*tmp, msg, IDMEF_MSG_USER_ID_NUMBER);
		}
	}
        prelude_msgbuf_set(msg, IDMEF_MSG_END_OF_TAG, 0, NULL);
}


void idmef_user_write(idmef_user_t *user, prelude_msgbuf_t *msg)
{
        if ( ! user )
                return;

        prelude_msgbuf_set(msg, IDMEF_MSG_USER_TAG, 0, NULL);

        prelude_string_write(idmef_user_get_ident(user), msg, IDMEF_MSG_USER_IDENT);
        uint32_write(idmef_user_get_category(user), msg, IDMEF_MSG_USER_CATEGORY);

        {
                idmef_user_id_t *user_id = NULL;

                while ( (user_id = idmef_user_get_next_user_id(user, user_id)) ) {
                        idmef_user_id_write(user_id, msg);
                }
        }


        prelude_msgbuf_set(msg, IDMEF_MSG_END_OF_TAG, 0, NULL);
}


void idmef_address_write(idmef_address_t *address, prelude_msgbuf_t *msg)
{
        if ( ! address )
                return;

        prelude_msgbuf_set(msg, IDMEF_MSG_ADDRESS_TAG, 0, NULL);

        prelude_string_write(idmef_address_get_ident(address), msg, IDMEF_MSG_ADDRESS_IDENT);
        uint32_write(idmef_address_get_category(address), msg, IDMEF_MSG_ADDRESS_CATEGORY);
        prelude_string_write(idmef_address_get_vlan_name(address), msg, IDMEF_MSG_ADDRESS_VLAN_NAME);

	{
		int32_t *tmp;

		tmp = idmef_address_get_vlan_num(address);
		if ( tmp ) {
			int32_write(*tmp, msg, IDMEF_MSG_ADDRESS_VLAN_NUM);
		}
	}        prelude_string_write(idmef_address_get_address(address), msg, IDMEF_MSG_ADDRESS_ADDRESS);
        prelude_string_write(idmef_address_get_netmask(address), msg, IDMEF_MSG_ADDRESS_NETMASK);

        prelude_msgbuf_set(msg, IDMEF_MSG_END_OF_TAG, 0, NULL);
}


void idmef_process_write(idmef_process_t *process, prelude_msgbuf_t *msg)
{
        if ( ! process )
                return;

        prelude_msgbuf_set(msg, IDMEF_MSG_PROCESS_TAG, 0, NULL);

        prelude_string_write(idmef_process_get_ident(process), msg, IDMEF_MSG_PROCESS_IDENT);
        prelude_string_write(idmef_process_get_name(process), msg, IDMEF_MSG_PROCESS_NAME);

	{
		uint32_t *tmp;

		tmp = idmef_process_get_pid(process);
		if ( tmp ) {
			uint32_write(*tmp, msg, IDMEF_MSG_PROCESS_PID);
		}
	}        prelude_string_write(idmef_process_get_path(process), msg, IDMEF_MSG_PROCESS_PATH);

        {
                prelude_string_t *arg = NULL;

                while ( (arg = idmef_process_get_next_arg(process, arg)) ) {
                        prelude_string_write(arg, msg, IDMEF_MSG_PROCESS_ARG);
                }
        }


        {
                prelude_string_t *env = NULL;

                while ( (env = idmef_process_get_next_env(process, env)) ) {
                        prelude_string_write(env, msg, IDMEF_MSG_PROCESS_ENV);
                }
        }


        prelude_msgbuf_set(msg, IDMEF_MSG_END_OF_TAG, 0, NULL);
}


void idmef_web_service_write(idmef_web_service_t *web_service, prelude_msgbuf_t *msg)
{
        if ( ! web_service )
                return;

        prelude_msgbuf_set(msg, IDMEF_MSG_WEB_SERVICE_TAG, 0, NULL);

        prelude_string_write(idmef_web_service_get_url(web_service), msg, IDMEF_MSG_WEB_SERVICE_URL);
        prelude_string_write(idmef_web_service_get_cgi(web_service), msg, IDMEF_MSG_WEB_SERVICE_CGI);
        prelude_string_write(idmef_web_service_get_http_method(web_service), msg, IDMEF_MSG_WEB_SERVICE_HTTP_METHOD);

        {
                prelude_string_t *arg = NULL;

                while ( (arg = idmef_web_service_get_next_arg(web_service, arg)) ) {
                        prelude_string_write(arg, msg, IDMEF_MSG_WEB_SERVICE_ARG);
                }
        }


        prelude_msgbuf_set(msg, IDMEF_MSG_END_OF_TAG, 0, NULL);
}


void idmef_snmp_service_write(idmef_snmp_service_t *snmp_service, prelude_msgbuf_t *msg)
{
        if ( ! snmp_service )
                return;

        prelude_msgbuf_set(msg, IDMEF_MSG_SNMP_SERVICE_TAG, 0, NULL);

        prelude_string_write(idmef_snmp_service_get_oid(snmp_service), msg, IDMEF_MSG_SNMP_SERVICE_OID);
        prelude_string_write(idmef_snmp_service_get_community(snmp_service), msg, IDMEF_MSG_SNMP_SERVICE_COMMUNITY);
        prelude_string_write(idmef_snmp_service_get_security_name(snmp_service), msg, IDMEF_MSG_SNMP_SERVICE_SECURITY_NAME);
        prelude_string_write(idmef_snmp_service_get_context_name(snmp_service), msg, IDMEF_MSG_SNMP_SERVICE_CONTEXT_NAME);
        prelude_string_write(idmef_snmp_service_get_context_engine_id(snmp_service), msg, IDMEF_MSG_SNMP_SERVICE_CONTEXT_ENGINE_ID);
        prelude_string_write(idmef_snmp_service_get_command(snmp_service), msg, IDMEF_MSG_SNMP_SERVICE_COMMAND);

        prelude_msgbuf_set(msg, IDMEF_MSG_END_OF_TAG, 0, NULL);
}


void idmef_service_write(idmef_service_t *service, prelude_msgbuf_t *msg)
{
        if ( ! service )
                return;

        prelude_msgbuf_set(msg, IDMEF_MSG_SERVICE_TAG, 0, NULL);

        prelude_string_write(idmef_service_get_ident(service), msg, IDMEF_MSG_SERVICE_IDENT);

	{
		uint8_t *tmp;

		tmp = idmef_service_get_ip_version(service);
		if ( tmp ) {
			uint8_write(*tmp, msg, IDMEF_MSG_SERVICE_IP_VERSION);
		}
	}
	{
		uint8_t *tmp;

		tmp = idmef_service_get_iana_protocol_number(service);
		if ( tmp ) {
			uint8_write(*tmp, msg, IDMEF_MSG_SERVICE_IANA_PROTOCOL_NUMBER);
		}
	}        prelude_string_write(idmef_service_get_iana_protocol_name(service), msg, IDMEF_MSG_SERVICE_IANA_PROTOCOL_NAME);
        prelude_string_write(idmef_service_get_name(service), msg, IDMEF_MSG_SERVICE_NAME);

	{
		uint16_t *tmp;

		tmp = idmef_service_get_port(service);
		if ( tmp ) {
			uint16_write(*tmp, msg, IDMEF_MSG_SERVICE_PORT);
		}
	}        prelude_string_write(idmef_service_get_portlist(service), msg, IDMEF_MSG_SERVICE_PORTLIST);
        prelude_string_write(idmef_service_get_protocol(service), msg, IDMEF_MSG_SERVICE_PROTOCOL);

        switch ( idmef_service_get_type(service) ) {

                case IDMEF_SERVICE_TYPE_WEB:
                        idmef_web_service_write(idmef_service_get_web_service(service), msg);
                        break;

                case IDMEF_SERVICE_TYPE_SNMP:
                        idmef_snmp_service_write(idmef_service_get_snmp_service(service), msg);
                        break;

                default:
                        /* nop */;

        }

        prelude_msgbuf_set(msg, IDMEF_MSG_END_OF_TAG, 0, NULL);
}


void idmef_node_write(idmef_node_t *node, prelude_msgbuf_t *msg)
{
        if ( ! node )
                return;

        prelude_msgbuf_set(msg, IDMEF_MSG_NODE_TAG, 0, NULL);

        prelude_string_write(idmef_node_get_ident(node), msg, IDMEF_MSG_NODE_IDENT);
        uint32_write(idmef_node_get_category(node), msg, IDMEF_MSG_NODE_CATEGORY);
        prelude_string_write(idmef_node_get_location(node), msg, IDMEF_MSG_NODE_LOCATION);
        prelude_string_write(idmef_node_get_name(node), msg, IDMEF_MSG_NODE_NAME);

        {
                idmef_address_t *address = NULL;

                while ( (address = idmef_node_get_next_address(node, address)) ) {
                        idmef_address_write(address, msg);
                }
        }


        prelude_msgbuf_set(msg, IDMEF_MSG_END_OF_TAG, 0, NULL);
}


void idmef_source_write(idmef_source_t *source, prelude_msgbuf_t *msg)
{
        if ( ! source )
                return;

        prelude_msgbuf_set(msg, IDMEF_MSG_SOURCE_TAG, 0, NULL);

        prelude_string_write(idmef_source_get_ident(source), msg, IDMEF_MSG_SOURCE_IDENT);
        uint32_write(idmef_source_get_spoofed(source), msg, IDMEF_MSG_SOURCE_SPOOFED);
        prelude_string_write(idmef_source_get_interface(source), msg, IDMEF_MSG_SOURCE_INTERFACE);
        idmef_node_write(idmef_source_get_node(source), msg);
        idmef_user_write(idmef_source_get_user(source), msg);
        idmef_process_write(idmef_source_get_process(source), msg);
        idmef_service_write(idmef_source_get_service(source), msg);

        prelude_msgbuf_set(msg, IDMEF_MSG_END_OF_TAG, 0, NULL);
}


void idmef_file_access_write(idmef_file_access_t *file_access, prelude_msgbuf_t *msg)
{
        if ( ! file_access )
                return;

        prelude_msgbuf_set(msg, IDMEF_MSG_FILE_ACCESS_TAG, 0, NULL);

        idmef_user_id_write(idmef_file_access_get_user_id(file_access), msg);

        {
                prelude_string_t *permission = NULL;

                while ( (permission = idmef_file_access_get_next_permission(file_access, permission)) ) {
                        prelude_string_write(permission, msg, IDMEF_MSG_FILE_ACCESS_PERMISSION);
                }
        }


        prelude_msgbuf_set(msg, IDMEF_MSG_END_OF_TAG, 0, NULL);
}


void idmef_inode_write(idmef_inode_t *inode, prelude_msgbuf_t *msg)
{
        if ( ! inode )
                return;

        prelude_msgbuf_set(msg, IDMEF_MSG_INODE_TAG, 0, NULL);

        idmef_time_write(idmef_inode_get_change_time(inode), msg, IDMEF_MSG_INODE_CHANGE_TIME);

	{
		uint32_t *tmp;

		tmp = idmef_inode_get_number(inode);
		if ( tmp ) {
			uint32_write(*tmp, msg, IDMEF_MSG_INODE_NUMBER);
		}
	}
	{
		uint32_t *tmp;

		tmp = idmef_inode_get_major_device(inode);
		if ( tmp ) {
			uint32_write(*tmp, msg, IDMEF_MSG_INODE_MAJOR_DEVICE);
		}
	}
	{
		uint32_t *tmp;

		tmp = idmef_inode_get_minor_device(inode);
		if ( tmp ) {
			uint32_write(*tmp, msg, IDMEF_MSG_INODE_MINOR_DEVICE);
		}
	}
	{
		uint32_t *tmp;

		tmp = idmef_inode_get_c_major_device(inode);
		if ( tmp ) {
			uint32_write(*tmp, msg, IDMEF_MSG_INODE_C_MAJOR_DEVICE);
		}
	}
	{
		uint32_t *tmp;

		tmp = idmef_inode_get_c_minor_device(inode);
		if ( tmp ) {
			uint32_write(*tmp, msg, IDMEF_MSG_INODE_C_MINOR_DEVICE);
		}
	}
        prelude_msgbuf_set(msg, IDMEF_MSG_END_OF_TAG, 0, NULL);
}


void idmef_linkage_write(idmef_linkage_t *, prelude_msgbuf_t *);

void idmef_checksum_write(idmef_checksum_t *checksum, prelude_msgbuf_t *msg)
{
        if ( ! checksum )
                return;

        prelude_msgbuf_set(msg, IDMEF_MSG_CHECKSUM_TAG, 0, NULL);

        prelude_string_write(idmef_checksum_get_value(checksum), msg, IDMEF_MSG_CHECKSUM_VALUE);
        prelude_string_write(idmef_checksum_get_key(checksum), msg, IDMEF_MSG_CHECKSUM_KEY);
        uint32_write(idmef_checksum_get_algorithm(checksum), msg, IDMEF_MSG_CHECKSUM_ALGORITHM);

        prelude_msgbuf_set(msg, IDMEF_MSG_END_OF_TAG, 0, NULL);
}


void idmef_file_write(idmef_file_t *file, prelude_msgbuf_t *msg)
{
        if ( ! file )
                return;

        prelude_msgbuf_set(msg, IDMEF_MSG_FILE_TAG, 0, NULL);

        prelude_string_write(idmef_file_get_ident(file), msg, IDMEF_MSG_FILE_IDENT);
        prelude_string_write(idmef_file_get_name(file), msg, IDMEF_MSG_FILE_NAME);
        prelude_string_write(idmef_file_get_path(file), msg, IDMEF_MSG_FILE_PATH);
        idmef_time_write(idmef_file_get_create_time(file), msg, IDMEF_MSG_FILE_CREATE_TIME);
        idmef_time_write(idmef_file_get_modify_time(file), msg, IDMEF_MSG_FILE_MODIFY_TIME);
        idmef_time_write(idmef_file_get_access_time(file), msg, IDMEF_MSG_FILE_ACCESS_TIME);

	{
		uint64_t *tmp;

		tmp = idmef_file_get_data_size(file);
		if ( tmp ) {
			uint64_write(*tmp, msg, IDMEF_MSG_FILE_DATA_SIZE);
		}
	}
	{
		uint64_t *tmp;

		tmp = idmef_file_get_disk_size(file);
		if ( tmp ) {
			uint64_write(*tmp, msg, IDMEF_MSG_FILE_DISK_SIZE);
		}
	}
        {
                idmef_file_access_t *file_access = NULL;

                while ( (file_access = idmef_file_get_next_file_access(file, file_access)) ) {
                        idmef_file_access_write(file_access, msg);
                }
        }


        {
                idmef_linkage_t *linkage = NULL;

                while ( (linkage = idmef_file_get_next_linkage(file, linkage)) ) {
                        idmef_linkage_write(linkage, msg);
                }
        }

        idmef_inode_write(idmef_file_get_inode(file), msg);

        {
                idmef_checksum_t *checksum = NULL;

                while ( (checksum = idmef_file_get_next_checksum(file, checksum)) ) {
                        idmef_checksum_write(checksum, msg);
                }
        }

        uint32_write(idmef_file_get_category(file), msg, IDMEF_MSG_FILE_CATEGORY);

	{
		idmef_file_fstype_t *tmp;

		tmp = idmef_file_get_fstype(file);
		if ( tmp ) {
			uint32_write(*tmp, msg, IDMEF_MSG_FILE_FSTYPE);
		}
	}
        prelude_msgbuf_set(msg, IDMEF_MSG_END_OF_TAG, 0, NULL);
}


void idmef_linkage_write(idmef_linkage_t *linkage, prelude_msgbuf_t *msg)
{
        if ( ! linkage )
                return;

        prelude_msgbuf_set(msg, IDMEF_MSG_LINKAGE_TAG, 0, NULL);

        uint32_write(idmef_linkage_get_category(linkage), msg, IDMEF_MSG_LINKAGE_CATEGORY);
        prelude_string_write(idmef_linkage_get_name(linkage), msg, IDMEF_MSG_LINKAGE_NAME);
        prelude_string_write(idmef_linkage_get_path(linkage), msg, IDMEF_MSG_LINKAGE_PATH);
        idmef_file_write(idmef_linkage_get_file(linkage), msg);

        prelude_msgbuf_set(msg, IDMEF_MSG_END_OF_TAG, 0, NULL);
}


void idmef_target_write(idmef_target_t *target, prelude_msgbuf_t *msg)
{
        if ( ! target )
                return;

        prelude_msgbuf_set(msg, IDMEF_MSG_TARGET_TAG, 0, NULL);

        prelude_string_write(idmef_target_get_ident(target), msg, IDMEF_MSG_TARGET_IDENT);
        uint32_write(idmef_target_get_decoy(target), msg, IDMEF_MSG_TARGET_DECOY);
        prelude_string_write(idmef_target_get_interface(target), msg, IDMEF_MSG_TARGET_INTERFACE);
        idmef_node_write(idmef_target_get_node(target), msg);
        idmef_user_write(idmef_target_get_user(target), msg);
        idmef_process_write(idmef_target_get_process(target), msg);
        idmef_service_write(idmef_target_get_service(target), msg);

        {
                idmef_file_t *file = NULL;

                while ( (file = idmef_target_get_next_file(target, file)) ) {
                        idmef_file_write(file, msg);
                }
        }


        prelude_msgbuf_set(msg, IDMEF_MSG_END_OF_TAG, 0, NULL);
}


void idmef_analyzer_write(idmef_analyzer_t *analyzer, prelude_msgbuf_t *msg)
{
        if ( ! analyzer )
                return;

        prelude_msgbuf_set(msg, IDMEF_MSG_ANALYZER_TAG, 0, NULL);

        prelude_string_write(idmef_analyzer_get_analyzerid(analyzer), msg, IDMEF_MSG_ANALYZER_ANALYZERID);
        prelude_string_write(idmef_analyzer_get_name(analyzer), msg, IDMEF_MSG_ANALYZER_NAME);
        prelude_string_write(idmef_analyzer_get_manufacturer(analyzer), msg, IDMEF_MSG_ANALYZER_MANUFACTURER);
        prelude_string_write(idmef_analyzer_get_model(analyzer), msg, IDMEF_MSG_ANALYZER_MODEL);
        prelude_string_write(idmef_analyzer_get_version(analyzer), msg, IDMEF_MSG_ANALYZER_VERSION);
        prelude_string_write(idmef_analyzer_get_class(analyzer), msg, IDMEF_MSG_ANALYZER_CLASS);
        prelude_string_write(idmef_analyzer_get_ostype(analyzer), msg, IDMEF_MSG_ANALYZER_OSTYPE);
        prelude_string_write(idmef_analyzer_get_osversion(analyzer), msg, IDMEF_MSG_ANALYZER_OSVERSION);
        idmef_node_write(idmef_analyzer_get_node(analyzer), msg);
        idmef_process_write(idmef_analyzer_get_process(analyzer), msg);

        prelude_msgbuf_set(msg, IDMEF_MSG_END_OF_TAG, 0, NULL);
}


void idmef_alertident_write(idmef_alertident_t *alertident, prelude_msgbuf_t *msg)
{
        if ( ! alertident )
                return;

        prelude_msgbuf_set(msg, IDMEF_MSG_ALERTIDENT_TAG, 0, NULL);

        prelude_string_write(idmef_alertident_get_alertident(alertident), msg, IDMEF_MSG_ALERTIDENT_ALERTIDENT);
        prelude_string_write(idmef_alertident_get_analyzerid(alertident), msg, IDMEF_MSG_ALERTIDENT_ANALYZERID);

        prelude_msgbuf_set(msg, IDMEF_MSG_END_OF_TAG, 0, NULL);
}


void idmef_impact_write(idmef_impact_t *impact, prelude_msgbuf_t *msg)
{
        if ( ! impact )
                return;

        prelude_msgbuf_set(msg, IDMEF_MSG_IMPACT_TAG, 0, NULL);


	{
		idmef_impact_severity_t *tmp;

		tmp = idmef_impact_get_severity(impact);
		if ( tmp ) {
			uint32_write(*tmp, msg, IDMEF_MSG_IMPACT_SEVERITY);
			prelude_msg_set_priority(prelude_msgbuf_get_msg(msg),
					         idmef_impact_severity_to_msg_priority(*tmp));
		}
	}
	{
		idmef_impact_completion_t *tmp;

		tmp = idmef_impact_get_completion(impact);
		if ( tmp ) {
			uint32_write(*tmp, msg, IDMEF_MSG_IMPACT_COMPLETION);
		}
	}        uint32_write(idmef_impact_get_type(impact), msg, IDMEF_MSG_IMPACT_TYPE);
        prelude_string_write(idmef_impact_get_description(impact), msg, IDMEF_MSG_IMPACT_DESCRIPTION);

        prelude_msgbuf_set(msg, IDMEF_MSG_END_OF_TAG, 0, NULL);
}


void idmef_action_write(idmef_action_t *action, prelude_msgbuf_t *msg)
{
        if ( ! action )
                return;

        prelude_msgbuf_set(msg, IDMEF_MSG_ACTION_TAG, 0, NULL);

        uint32_write(idmef_action_get_category(action), msg, IDMEF_MSG_ACTION_CATEGORY);
        prelude_string_write(idmef_action_get_description(action), msg, IDMEF_MSG_ACTION_DESCRIPTION);

        prelude_msgbuf_set(msg, IDMEF_MSG_END_OF_TAG, 0, NULL);
}


void idmef_confidence_write(idmef_confidence_t *confidence, prelude_msgbuf_t *msg)
{
        if ( ! confidence )
                return;

        prelude_msgbuf_set(msg, IDMEF_MSG_CONFIDENCE_TAG, 0, NULL);

        uint32_write(idmef_confidence_get_rating(confidence), msg, IDMEF_MSG_CONFIDENCE_RATING);
        float_write(idmef_confidence_get_confidence(confidence), msg, IDMEF_MSG_CONFIDENCE_CONFIDENCE);

        prelude_msgbuf_set(msg, IDMEF_MSG_END_OF_TAG, 0, NULL);
}


void idmef_assessment_write(idmef_assessment_t *assessment, prelude_msgbuf_t *msg)
{
        if ( ! assessment )
                return;

        prelude_msgbuf_set(msg, IDMEF_MSG_ASSESSMENT_TAG, 0, NULL);

        idmef_impact_write(idmef_assessment_get_impact(assessment), msg);

        {
                idmef_action_t *action = NULL;

                while ( (action = idmef_assessment_get_next_action(assessment, action)) ) {
                        idmef_action_write(action, msg);
                }
        }

        idmef_confidence_write(idmef_assessment_get_confidence(assessment), msg);

        prelude_msgbuf_set(msg, IDMEF_MSG_END_OF_TAG, 0, NULL);
}


void idmef_tool_alert_write(idmef_tool_alert_t *tool_alert, prelude_msgbuf_t *msg)
{
        if ( ! tool_alert )
                return;

        prelude_msgbuf_set(msg, IDMEF_MSG_TOOL_ALERT_TAG, 0, NULL);

        prelude_string_write(idmef_tool_alert_get_name(tool_alert), msg, IDMEF_MSG_TOOL_ALERT_NAME);
        prelude_string_write(idmef_tool_alert_get_command(tool_alert), msg, IDMEF_MSG_TOOL_ALERT_COMMAND);

        {
                idmef_alertident_t *alertident = NULL;

                while ( (alertident = idmef_tool_alert_get_next_alertident(tool_alert, alertident)) ) {
                        idmef_alertident_write(alertident, msg);
                }
        }


        prelude_msgbuf_set(msg, IDMEF_MSG_END_OF_TAG, 0, NULL);
}


void idmef_correlation_alert_write(idmef_correlation_alert_t *correlation_alert, prelude_msgbuf_t *msg)
{
        if ( ! correlation_alert )
                return;

        prelude_msgbuf_set(msg, IDMEF_MSG_CORRELATION_ALERT_TAG, 0, NULL);

        prelude_string_write(idmef_correlation_alert_get_name(correlation_alert), msg, IDMEF_MSG_CORRELATION_ALERT_NAME);

        {
                idmef_alertident_t *alertident = NULL;

                while ( (alertident = idmef_correlation_alert_get_next_alertident(correlation_alert, alertident)) ) {
                        idmef_alertident_write(alertident, msg);
                }
        }


        prelude_msgbuf_set(msg, IDMEF_MSG_END_OF_TAG, 0, NULL);
}


void idmef_overflow_alert_write(idmef_overflow_alert_t *overflow_alert, prelude_msgbuf_t *msg)
{
        if ( ! overflow_alert )
                return;

        prelude_msgbuf_set(msg, IDMEF_MSG_OVERFLOW_ALERT_TAG, 0, NULL);

        prelude_string_write(idmef_overflow_alert_get_program(overflow_alert), msg, IDMEF_MSG_OVERFLOW_ALERT_PROGRAM);

	{
		uint32_t *tmp;

		tmp = idmef_overflow_alert_get_size(overflow_alert);
		if ( tmp ) {
			uint32_write(*tmp, msg, IDMEF_MSG_OVERFLOW_ALERT_SIZE);
		}
	}        idmef_data_write(idmef_overflow_alert_get_buffer(overflow_alert), msg, IDMEF_MSG_OVERFLOW_ALERT_BUFFER);

        prelude_msgbuf_set(msg, IDMEF_MSG_END_OF_TAG, 0, NULL);
}


void idmef_alert_write(idmef_alert_t *alert, prelude_msgbuf_t *msg)
{
        if ( ! alert )
                return;

        prelude_msgbuf_set(msg, IDMEF_MSG_ALERT_TAG, 0, NULL);

        prelude_string_write(idmef_alert_get_messageid(alert), msg, IDMEF_MSG_ALERT_MESSAGEID);

        {
                idmef_analyzer_t *analyzer = NULL;

                while ( (analyzer = idmef_alert_get_next_analyzer(alert, analyzer)) ) {
                        idmef_analyzer_write(analyzer, msg);
                }
        }

        idmef_time_write(idmef_alert_get_create_time(alert), msg, IDMEF_MSG_ALERT_CREATE_TIME);
        idmef_classification_write(idmef_alert_get_classification(alert), msg);
        idmef_time_write(idmef_alert_get_detect_time(alert), msg, IDMEF_MSG_ALERT_DETECT_TIME);
        idmef_time_write(idmef_alert_get_analyzer_time(alert), msg, IDMEF_MSG_ALERT_ANALYZER_TIME);

        {
                idmef_source_t *source = NULL;

                while ( (source = idmef_alert_get_next_source(alert, source)) ) {
                        idmef_source_write(source, msg);
                }
        }


        {
                idmef_target_t *target = NULL;

                while ( (target = idmef_alert_get_next_target(alert, target)) ) {
                        idmef_target_write(target, msg);
                }
        }

        idmef_assessment_write(idmef_alert_get_assessment(alert), msg);

        {
                idmef_additional_data_t *additional_data = NULL;

                while ( (additional_data = idmef_alert_get_next_additional_data(alert, additional_data)) ) {
                        idmef_additional_data_write(additional_data, msg);
                }
        }


        switch ( idmef_alert_get_type(alert) ) {

                case IDMEF_ALERT_TYPE_TOOL:
                        idmef_tool_alert_write(idmef_alert_get_tool_alert(alert), msg);
                        break;

                case IDMEF_ALERT_TYPE_CORRELATION:
                        idmef_correlation_alert_write(idmef_alert_get_correlation_alert(alert), msg);
                        break;

                case IDMEF_ALERT_TYPE_OVERFLOW:
                        idmef_overflow_alert_write(idmef_alert_get_overflow_alert(alert), msg);
                        break;

                default:
                        /* nop */;

        }

        prelude_msgbuf_set(msg, IDMEF_MSG_END_OF_TAG, 0, NULL);
}


void idmef_heartbeat_write(idmef_heartbeat_t *heartbeat, prelude_msgbuf_t *msg)
{
        if ( ! heartbeat )
                return;

        prelude_msgbuf_set(msg, IDMEF_MSG_HEARTBEAT_TAG, 0, NULL);

        prelude_string_write(idmef_heartbeat_get_messageid(heartbeat), msg, IDMEF_MSG_HEARTBEAT_MESSAGEID);

        {
                idmef_analyzer_t *analyzer = NULL;

                while ( (analyzer = idmef_heartbeat_get_next_analyzer(heartbeat, analyzer)) ) {
                        idmef_analyzer_write(analyzer, msg);
                }
        }

        idmef_time_write(idmef_heartbeat_get_create_time(heartbeat), msg, IDMEF_MSG_HEARTBEAT_CREATE_TIME);
        idmef_time_write(idmef_heartbeat_get_analyzer_time(heartbeat), msg, IDMEF_MSG_HEARTBEAT_ANALYZER_TIME);

	{
		uint32_t *tmp;

		tmp = idmef_heartbeat_get_heartbeat_interval(heartbeat);
		if ( tmp ) {
			uint32_write(*tmp, msg, IDMEF_MSG_HEARTBEAT_HEARTBEAT_INTERVAL);
		}
	}
        {
                idmef_additional_data_t *additional_data = NULL;

                while ( (additional_data = idmef_heartbeat_get_next_additional_data(heartbeat, additional_data)) ) {
                        idmef_additional_data_write(additional_data, msg);
                }
        }


        prelude_msgbuf_set(msg, IDMEF_MSG_END_OF_TAG, 0, NULL);
}


void idmef_message_write(idmef_message_t *message, prelude_msgbuf_t *msg)
{
        if ( ! message )
                return;

        prelude_string_write(idmef_message_get_version(message), msg, IDMEF_MSG_MESSAGE_VERSION);

        switch ( idmef_message_get_type(message) ) {

                case IDMEF_MESSAGE_TYPE_ALERT:
                        idmef_alert_write(idmef_message_get_alert(message), msg);
                        break;

                case IDMEF_MESSAGE_TYPE_HEARTBEAT:
                        idmef_heartbeat_write(idmef_message_get_heartbeat(message), msg);
                        break;

                default:
                        /* nop */;

        }

        prelude_msgbuf_set(msg, IDMEF_MSG_END_OF_TAG, 0, NULL);
}


