/*****
*
* Copyright (C) 2002, 2003 Yoann Vandoorselaere <yoann@prelude-ids.org>
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
#include <string.h>
#include <unistd.h>
#include <inttypes.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>

#include "list.h"
#include "common.h"
#include "prelude-log.h"
#include "prelude-io.h"
#include "prelude-message.h"
#include "prelude-message-buffered.h"
#include "prelude-message-id.h"
#include "idmef-message-id.h"
#include "idmef-tree.h"
#include "idmef-msg-send.h"
#include "sensor.h"
#include "prelude-client.h"



/*
 * If you wonder why we do this, and why life is complicated,
 * then wonder why the hell the guys that wrote IDMEF choose to use XML.
 * XML is dog slow. And XML'll never achieve performance needed for real time IDS.
 *
 * Here we are trying to communicate using a home made, binary version of IDMEF.
 */


inline void idmef_send_string(prelude_msgbuf_t *msg, uint8_t tag, idmef_string_t *string)
{
        if ( ! string || ! string->string )
                return;

        prelude_msgbuf_set(msg, tag, string->len, string->string);
}



inline void idmef_send_uint64(prelude_msgbuf_t *msg, uint8_t tag, uint64_t *data) 
{
        uint64_t dst;
        
        if ( *data == 0 )
                return;

        dst = prelude_hton64(*data);
                
        prelude_msgbuf_set(msg, tag, sizeof(*data), &dst);
}



inline void idmef_send_uint32(prelude_msgbuf_t *msg, uint8_t tag, uint32_t data) 
{        
        if ( data == 0 )
                return;
        
        data = htonl(data);
        prelude_msgbuf_set(msg, tag, sizeof(data), &data);
}



inline void idmef_send_uint16(prelude_msgbuf_t *msg, uint8_t tag, uint16_t data) 
{
        if ( data == 0 )
                return;
        
        data = htons(data);
        prelude_msgbuf_set(msg, tag, sizeof(data), &data);
}




void idmef_send_time(prelude_msgbuf_t *msg, uint8_t tag, idmef_time_t *time) 
{
        if (! time )
                return;

        prelude_msgbuf_set(msg, tag, 0, NULL);        
        idmef_send_uint32(msg, MSG_TIME_SEC, time->sec);
        idmef_send_uint32(msg, MSG_TIME_USEC, time->usec);
        prelude_msgbuf_set(msg, MSG_END_OF_TAG, 0, NULL);
}


void idmef_send_additional_data(prelude_msgbuf_t *msg, idmef_additional_data_t *data) 
{        
        prelude_msgbuf_set(msg, MSG_ADDITIONALDATA_TAG, 0, NULL);
        idmef_send_uint32(msg, MSG_ADDITIONALDATA_TYPE, data->type);
        idmef_send_string(msg, MSG_ADDITIONALDATA_MEANING, &data->meaning);
        prelude_msgbuf_set(msg, MSG_ADDITIONALDATA_DATA, data->dlen, data->data);
        prelude_msgbuf_set(msg, MSG_END_OF_TAG, 0, NULL);
}
       


void idmef_send_additional_data_list(prelude_msgbuf_t *msg, struct list_head *head) 
{
        struct list_head *tmp;
        idmef_additional_data_t *data;
        
        list_for_each(tmp, head) {
                data = list_entry(tmp, idmef_additional_data_t, list);
                idmef_send_additional_data(msg, data);
        }
}



void idmef_send_web_service_arg(prelude_msgbuf_t *msg, idmef_webservice_arg_t *arg) 
{
        idmef_send_string(msg, MSG_WEBSERVICE_ARG, &arg->arg);
}




void idmef_send_web_service(prelude_msgbuf_t *msg, idmef_webservice_t *web) 
{
        struct list_head *tmp;
        idmef_webservice_arg_t *arg;
        
        prelude_msgbuf_set(msg, MSG_WEBSERVICE_TAG, 0, NULL);

        idmef_send_string(msg, MSG_WEBSERVICE_URL, &web->url);
        idmef_send_string(msg, MSG_WEBSERVICE_CGI, &web->cgi);
        idmef_send_string(msg, MSG_WEBSERVICE_HTTP_METHOD, &web->http_method);

        list_for_each(tmp, &web->arg_list){
                arg = list_entry(tmp, idmef_webservice_arg_t, list);
                idmef_send_web_service_arg(msg, arg);
        }
        
        prelude_msgbuf_set(msg, MSG_END_OF_TAG, 0, NULL);
}



void idmef_send_snmp_service(prelude_msgbuf_t *msg, idmef_snmpservice_t *snmp) 
{
        prelude_msgbuf_set(msg, MSG_SNMPSERVICE_TAG, 0, NULL);

        idmef_send_string(msg, MSG_SNMPSERVICE_OID, &snmp->oid);
        idmef_send_string(msg, MSG_SNMPSERVICE_COMMUNITY, &snmp->community);
        idmef_send_string(msg, MSG_SNMPSERVICE_COMMAND, &snmp->command);

        prelude_msgbuf_set(msg, MSG_END_OF_TAG, 0, NULL);  
}





void idmef_send_service(prelude_msgbuf_t *msg, idmef_service_t *service) 
{
        if ( ! service )
                return;
        
        prelude_msgbuf_set(msg, MSG_SERVICE_TAG, 0, NULL);

        idmef_send_uint64(msg, MSG_SERVICE_IDENT, &service->ident);
        idmef_send_string(msg, MSG_SERVICE_NAME, &service->name);
        idmef_send_uint16(msg, MSG_SERVICE_PORT, service->port);
        idmef_send_string(msg, MSG_SERVICE_PORTLIST, &service->portlist);
        idmef_send_string(msg, MSG_SERVICE_PROTOCOL, &service->protocol);

        if ( service->type == web_service )
                idmef_send_web_service(msg, service->specific.web);

        else if ( service->type == snmp_service )
                idmef_send_snmp_service(msg, service->specific.snmp);
        
        prelude_msgbuf_set(msg, MSG_END_OF_TAG, 0, NULL);
}




void idmef_send_address(prelude_msgbuf_t *msg, idmef_address_t *address) 
{
        prelude_msgbuf_set(msg, MSG_ADDRESS_TAG, 0, NULL);
        idmef_send_uint32(msg, MSG_ADDRESS_CATEGORY, address->category);
        idmef_send_string(msg, MSG_ADDRESS_VLAN_NAME, &address->vlan_name);
        idmef_send_uint32(msg, MSG_ADDRESS_VLAN_NUM, address->vlan_num);
        idmef_send_string(msg, MSG_ADDRESS_ADDRESS, &address->address);
        idmef_send_string(msg, MSG_ADDRESS_NETMASK, &address->netmask);
        prelude_msgbuf_set(msg, MSG_END_OF_TAG, 0, NULL);
}




void idmef_send_address_list(prelude_msgbuf_t *msg, struct list_head *address_list) 
{
        struct list_head *tmp;
        idmef_address_t *address;
        
        list_for_each(tmp, address_list) {
                address = list_entry(tmp, idmef_address_t, list);
                idmef_send_address(msg, address);
        }
}



void idmef_send_string_list(prelude_msgbuf_t *msg, uint8_t tag, struct list_head *head)
{
        struct list_head *tmp;
        idmef_string_item_t *string;
        
        list_for_each(tmp, head) {
                string = list_entry(tmp, idmef_string_item_t, list);
                idmef_send_string(msg, tag, &string->string);
        }
}




void idmef_send_process(prelude_msgbuf_t *msg, idmef_process_t *process) 
{
        if ( ! process )
                return;
        
        prelude_msgbuf_set(msg, MSG_PROCESS_TAG, 0, NULL);

        idmef_send_string(msg, MSG_PROCESS_NAME, &process->name);
        idmef_send_uint32(msg, MSG_PROCESS_PID, process->pid);
        idmef_send_string(msg, MSG_PROCESS_PATH, &process->path);
        idmef_send_string_list(msg, MSG_PROCESS_ARG, &process->arg_list);
        idmef_send_string_list(msg, MSG_PROCESS_ENV, &process->env_list);
        
        prelude_msgbuf_set(msg, MSG_END_OF_TAG, 0, NULL);
}




void idmef_send_node(prelude_msgbuf_t *msg, idmef_node_t *node) 
{
        if ( ! node )
                return;
        
        prelude_msgbuf_set(msg, MSG_NODE_TAG, 0, NULL);

        idmef_send_uint32(msg, MSG_NODE_CATEGORY, node->category);
        idmef_send_string(msg, MSG_NODE_LOCATION, &node->location);
        idmef_send_string(msg, MSG_NODE_NAME, &node->name);
        idmef_send_address_list(msg, &node->address_list);
        
        prelude_msgbuf_set(msg, MSG_END_OF_TAG, 0, NULL);
}




void idmef_send_userid(prelude_msgbuf_t *msg, idmef_userid_t *uid) 
{
        prelude_msgbuf_set(msg, MSG_USERID_TAG, 0, NULL);
        idmef_send_uint32(msg, MSG_USERID_TYPE, uid->type);
        idmef_send_string(msg, MSG_USERID_NAME, &uid->name);
        idmef_send_uint32(msg, MSG_USERID_NUMBER, uid->number);
        prelude_msgbuf_set(msg, MSG_END_OF_TAG, 0, NULL);
}




void idmef_send_userid_list(prelude_msgbuf_t *msg, struct list_head *head) 
{
        idmef_userid_t *uid;
        struct list_head *tmp;

        list_for_each(tmp, head) {
                uid = list_entry(tmp, idmef_userid_t, list);
                idmef_send_userid(msg, uid);
        }
}




void idmef_send_user(prelude_msgbuf_t *msg, idmef_user_t *user) 
{
        if ( ! user )
                return;
        
        prelude_msgbuf_set(msg, MSG_USER_TAG, 0, NULL);
        idmef_send_uint32(msg, MSG_USER_CATEGORY, user->category);
        idmef_send_userid_list(msg, &user->userid_list);
        prelude_msgbuf_set(msg, MSG_END_OF_TAG, 0, NULL);
}




void idmef_send_source(prelude_msgbuf_t *msg, idmef_source_t *source) 
{
        prelude_msgbuf_set(msg, MSG_SOURCE_TAG, 0, NULL);

        idmef_send_uint64(msg, MSG_SOURCE_IDENT, &source->ident);
        idmef_send_uint32(msg, MSG_SOURCE_SPOOFED, source->spoofed);
        idmef_send_string(msg, MSG_SOURCE_INTERFACE, &source->interface);
        idmef_send_node(msg, source->node);
        idmef_send_user(msg, source->user);
        idmef_send_process(msg, source->process);
        idmef_send_service(msg, source->service);
        
        prelude_msgbuf_set(msg, MSG_END_OF_TAG, 0, NULL);
}



void idmef_send_source_list(prelude_msgbuf_t *msg, struct list_head *head) 
{
        struct list_head *tmp;
        idmef_source_t *source;

        list_for_each(tmp, head) {
                source = list_entry(tmp, idmef_source_t, list);
                idmef_send_source(msg, source);
        }               
}



void idmef_send_file_access(prelude_msgbuf_t *msg, idmef_file_access_t *access) 
{
        prelude_msgbuf_set(msg, MSG_ACCESS_TAG, 0, NULL);

        idmef_send_userid(msg, &access->userid);
        idmef_send_string_list(msg, MSG_ACCESS_PERMISSION, &access->permission_list);
        
        prelude_msgbuf_set(msg, MSG_END_OF_TAG, 0, NULL);
}




void idmef_send_file_access_list(prelude_msgbuf_t *msg, struct list_head *head) 
{
        struct list_head *tmp;
        idmef_file_access_t *access;

        list_for_each(tmp, head) {

                access = list_entry(tmp, idmef_file_access_t, list);
                idmef_send_file_access(msg, access);
        }
}




void idmef_send_linkage(prelude_msgbuf_t *msg, idmef_linkage_t *linkage) 
{
        prelude_msgbuf_set(msg, MSG_LINKAGE_TAG, 0, NULL);

        idmef_send_uint32(msg, MSG_LINKAGE_CATEGORY, linkage->category);
        idmef_send_string(msg, MSG_LINKAGE_NAME, &linkage->name);
        idmef_send_string(msg, MSG_LINKAGE_PATH, &linkage->path);
        idmef_send_file(msg, linkage->file);
        
        prelude_msgbuf_set(msg, MSG_END_OF_TAG, 0, NULL);
}




void idmef_send_linkage_list(prelude_msgbuf_t *msg, struct list_head *head) 
{
        struct list_head *tmp;
        idmef_linkage_t *linkage;

        list_for_each(tmp, head) {
                linkage = list_entry(tmp, idmef_linkage_t, list);
                idmef_send_linkage(msg, linkage);
        }
}



void idmef_send_inode(prelude_msgbuf_t *msg, idmef_inode_t *inode) 
{
        if ( ! inode )
                return;
        
        prelude_msgbuf_set(msg, MSG_INODE_TAG, 0, NULL);
        
        idmef_send_time(msg, MSG_INODE_CHANGE_TIME, inode->change_time);
        idmef_send_uint32(msg, MSG_INODE_NUMBER, inode->number);
        idmef_send_uint32(msg, MSG_INODE_MAJOR_DEVICE, inode->major_device);
        idmef_send_uint32(msg, MSG_INODE_MINOR_DEVICE, inode->minor_device);
        idmef_send_uint32(msg, MSG_INODE_C_MAJOR_DEVICE, inode->c_major_device);
        idmef_send_uint32(msg, MSG_INODE_C_MINOR_DEVICE, inode->c_minor_device);

        prelude_msgbuf_set(msg, MSG_END_OF_TAG, 0, NULL);
}




void idmef_send_file(prelude_msgbuf_t *msg, idmef_file_t *file) 
{
        prelude_msgbuf_set(msg, MSG_FILE_TAG, 0, NULL);

        idmef_send_uint64(msg, MSG_FILE_IDENT, &file->ident);
        idmef_send_uint32(msg, MSG_FILE_CATEGORY, file->category);
        idmef_send_string(msg, MSG_FILE_FSTYPE, &file->fstype);
        idmef_send_string(msg, MSG_FILE_NAME, &file->name);
        idmef_send_string(msg, MSG_FILE_PATH, &file->path);
        idmef_send_time(msg, MSG_FILE_CREATE_TIME_TAG, file->create_time);
        idmef_send_time(msg, MSG_FILE_MODIFY_TIME_TAG, file->modify_time);
        idmef_send_time(msg, MSG_FILE_ACCESS_TIME_TAG, file->access_time);
        idmef_send_uint32(msg, MSG_FILE_DATASIZE, file->data_size);
        idmef_send_uint32(msg, MSG_FILE_DISKSIZE, file->disk_size);
        idmef_send_file_access_list(msg, &file->file_access_list);
        idmef_send_linkage_list(msg, &file->file_linkage_list);
        idmef_send_inode(msg, file->inode);
        
        prelude_msgbuf_set(msg, MSG_END_OF_TAG, 0, NULL);
}




void idmef_send_file_list(prelude_msgbuf_t *msg, struct list_head *head) 
{
        idmef_file_t *file;
        struct list_head *tmp;

        list_for_each(tmp, head) {

                file = list_entry(tmp, idmef_file_t, list);
                idmef_send_file(msg, file);
        }
}




void idmef_send_target(prelude_msgbuf_t *msg, idmef_target_t *target) 
{
        prelude_msgbuf_set(msg, MSG_TARGET_TAG, 0, NULL);

        idmef_send_uint64(msg, MSG_TARGET_IDENT, &target->ident);
        idmef_send_uint32(msg, MSG_TARGET_DECOY, target->decoy);
        idmef_send_string(msg, MSG_TARGET_INTERFACE, &target->interface);
        idmef_send_node(msg, target->node);
        idmef_send_user(msg, target->user);
        idmef_send_process(msg, target->process);
        idmef_send_service(msg, target->service);
        idmef_send_file_list(msg, &target->file_list);
        
        prelude_msgbuf_set(msg, MSG_END_OF_TAG, 0, NULL);
}




void idmef_send_target_list(prelude_msgbuf_t *msg, struct list_head *head) 
{
        struct list_head *tmp;
        idmef_target_t *target;

        list_for_each(tmp, head) {
                target = list_entry(tmp, idmef_target_t, list);
                idmef_send_target(msg, target);
        }               
}



void idmef_send_analyzer(prelude_msgbuf_t *msg, idmef_analyzer_t *analyzer) 
{
        prelude_msgbuf_set(msg, MSG_ANALYZER_TAG, 0, NULL);

        analyzer->analyzerid = prelude_client_get_analyzerid();
        
        idmef_send_uint64(msg, MSG_ANALYZER_ID, &analyzer->analyzerid);
        idmef_send_string(msg, MSG_ANALYZER_MANUFACTURER, &analyzer->manufacturer);
        idmef_send_string(msg, MSG_ANALYZER_MODEL, &analyzer->model);
        idmef_send_string(msg, MSG_ANALYZER_VERSION, &analyzer->version);
        idmef_send_string(msg, MSG_ANALYZER_CLASS, &analyzer->class);
        idmef_send_string(msg, MSG_ANALYZER_OSTYPE, &analyzer->ostype);
        idmef_send_string(msg, MSG_ANALYZER_OSVERSION, &analyzer->osversion);
        idmef_send_node(msg, analyzer->node);
        idmef_send_process(msg, analyzer->process);
        
        prelude_msgbuf_set(msg, MSG_END_OF_TAG, 0, NULL);
}




void idmef_send_create_time(prelude_msgbuf_t *msg, idmef_time_t *time) 
{
        idmef_send_time(msg, MSG_CREATE_TIME_TAG, time);
}


void idmef_send_detect_time(prelude_msgbuf_t *msg, idmef_time_t *time) 
{
        idmef_send_time(msg, MSG_DETECT_TIME_TAG, time);
}


void idmef_send_analyzer_time(prelude_msgbuf_t *msg, idmef_time_t *time) 
{
        idmef_send_time(msg, MSG_ANALYZER_TIME_TAG, time);
}


void idmef_send_classification(prelude_msgbuf_t *msg, idmef_classification_t *classification) 
{
        prelude_msgbuf_set(msg, MSG_CLASSIFICATION_TAG, 0, NULL);
        
        idmef_send_uint32(msg, MSG_CLASSIFICATION_ORIGIN, classification->origin);
        idmef_send_string(msg, MSG_CLASSIFICATION_NAME, &classification->name);
        idmef_send_string(msg, MSG_CLASSIFICATION_URL, &classification->url);

        prelude_msgbuf_set(msg, MSG_END_OF_TAG, 0, NULL);
}




void idmef_send_classification_list(prelude_msgbuf_t *msg, struct list_head *head) 
{
        struct list_head *tmp;
        idmef_classification_t *classification;

        list_for_each(tmp, head) {
                classification = list_entry(tmp, idmef_classification_t, list);
                idmef_send_classification(msg, classification);
        }
}



void idmef_send_confidence(prelude_msgbuf_t *msg, idmef_confidence_t *confidence) 
{
        if ( ! confidence )
                return;

        prelude_msgbuf_set(msg, MSG_CONFIDENCE_TAG, 0, NULL);

        idmef_send_uint32(msg, MSG_CONFIDENCE_RATING, confidence->rating);
        idmef_send_uint32(msg, MSG_CONFIDENCE_CONFIDENCE, confidence->confidence);
        
        prelude_msgbuf_set(msg, MSG_END_OF_TAG, 0, NULL);
}




void idmef_send_action(prelude_msgbuf_t *msg, idmef_action_t *action) 
{
        prelude_msgbuf_set(msg, MSG_ACTION_TAG, 0, NULL);

        idmef_send_uint32(msg, MSG_ACTION_CATEGORY, action->category);
        idmef_send_string(msg, MSG_ACTION_DESCRIPTION, &action->description);

        prelude_msgbuf_set(msg, MSG_END_OF_TAG, 0, NULL);
}




void idmef_send_action_list(prelude_msgbuf_t *msg, struct list_head *head) 
{
        struct list_head *tmp;
        idmef_action_t *action;

        list_for_each(tmp, head) {
                action = list_entry(tmp, idmef_action_t, list);
                idmef_send_action(msg, action);
        }
}




void idmef_send_impact(prelude_msgbuf_t *msg, idmef_impact_t *impact) 
{
        if ( ! impact )
                return;

        prelude_msgbuf_set(msg, MSG_IMPACT_TAG, 0, NULL);

        idmef_send_uint32(msg, MSG_IMPACT_SEVERITY, impact->severity);
        idmef_send_uint32(msg, MSG_IMPACT_COMPLETION, impact->completion);
        idmef_send_uint32(msg, MSG_IMPACT_TYPE, impact->type);
        idmef_send_string(msg, MSG_IMPACT_DESCRIPTION, &impact->description);

        prelude_msgbuf_set(msg, MSG_END_OF_TAG, 0, NULL);
}




void idmef_send_alert(prelude_msgbuf_t *msg, idmef_alert_t *alert) 
{
        prelude_msgbuf_set(msg, MSG_ALERT_TAG, 0, NULL);

        idmef_send_assessment(msg, alert->assessment);
        idmef_send_analyzer(msg, &alert->analyzer);
        idmef_send_create_time(msg, &alert->create_time);
        idmef_send_detect_time(msg, alert->detect_time);
        idmef_send_analyzer_time(msg, alert->analyzer_time);
        idmef_send_source_list(msg, &alert->source_list);
        idmef_send_target_list(msg, &alert->target_list);
        idmef_send_classification_list(msg, &alert->classification_list);
        idmef_send_additional_data_list(msg, &alert->additional_data_list);

        prelude_msgbuf_set(msg, MSG_END_OF_TAG, 0, NULL);
}




void idmef_send_heartbeat(prelude_msgbuf_t *msg, idmef_heartbeat_t *hb) 
{
        prelude_msgbuf_set(msg, MSG_HEARTBEAT_TAG, 0, NULL);

        idmef_send_analyzer(msg, &hb->analyzer);
        idmef_send_create_time(msg, &hb->create_time);
        idmef_send_analyzer_time(msg, hb->analyzer_time);
        idmef_send_additional_data_list(msg, &hb->additional_data_list);
        
        prelude_msgbuf_set(msg, MSG_END_OF_TAG, 0, NULL);
}





void idmef_send_assessment(prelude_msgbuf_t *msg, idmef_assessment_t *assessment) 
{       
        if ( ! assessment )
                return;
        
        prelude_msgbuf_set(msg, MSG_ASSESSMENT_TAG, 0, NULL);
        idmef_send_impact(msg, assessment->impact);
        idmef_send_action_list(msg, &assessment->action_list);
        idmef_send_confidence(msg, assessment->confidence);
        prelude_msgbuf_set(msg, MSG_END_OF_TAG, 0, NULL);
}




int idmef_msg_send(prelude_msgbuf_t *msgbuf, idmef_message_t *idmef, uint8_t priority)
{
        
        switch ( idmef->type ) {

        case idmef_alert_message:
                idmef_send_alert(msgbuf, idmef->message.alert);
                break;
                
        case idmef_heartbeat_message:
                idmef_send_heartbeat(msgbuf, idmef->message.heartbeat);                
                break;
        }

        prelude_msgbuf_mark_end(msgbuf);

        return 0;
}





