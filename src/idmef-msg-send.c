/*****
*
* Copyright (C) 2002 Yoann Vandoorselaere <yoann@mandrakesoft.com>
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
#include <sys/time.h>

#include "list.h"
#include "prelude-io.h"
#include "prelude-message.h"
#include "prelude-message-id.h"
#include "idmef-message-id.h"
#include "idmef-tree.h"
#include "idmef-msg-send.h"
#include "sensor.h"

static void send_file(prelude_msg_t *msg, idmef_file_t *file);


/*
 * If you wonder why we do this, and why life is complicated,
 * then wonder why the hell the guys that wrote IDMEF choose to use XML.
 * XML is dog slow. And XML'll never achieve performance needed for real time IDS.
 *
 * Here we are trying to communicate using a home made, binary version of IDMEF.
 */


static void send_string(prelude_msg_t *msg, uint8_t tag, const char *string)
{
        if ( ! string )
                return;
        
        prelude_msg_set(msg, tag, strlen(string) + 1, string);
}



static void send_uint64(prelude_msg_t *msg, uint8_t tag, uint64_t *data) 
{
        uint64_t dst;
        
        if ( *data == 0 )
                return;
        
        ((uint32_t *) &dst)[0] = ntohl(((uint32_t *) data)[1]);
        ((uint32_t *) &dst)[1] = ntohl(((uint32_t *) data)[0]);
        
        prelude_msg_set(msg, tag, sizeof(*data), &dst);
}



static void send_uint32(prelude_msg_t *msg, uint8_t tag, uint32_t data) 
{        
        if ( data == 0 )
                return;
        
        data = htonl(data);
        prelude_msg_set(msg, tag, sizeof(data), &data);
}



static void send_uint16(prelude_msg_t *msg, uint8_t tag, uint16_t data) 
{
        if ( data == 0 )
                return;
        
        data = htons(data);
        prelude_msg_set(msg, tag, sizeof(data), &data);
}




static void send_time(prelude_msg_t *msg, uint8_t tag, idmef_time_t *time) 
{
        if (! time )
                return;

        prelude_msg_set(msg, tag, 0, NULL);        
        send_uint32(msg, MSG_TIME_SEC, time->sec);
        send_uint32(msg, MSG_TIME_USEC, time->usec);
        prelude_msg_set(msg, MSG_END_OF_TAG, 0, NULL);
}

       

static void send_additional_data(prelude_msg_t *msg, struct list_head *head) 
{
        struct list_head *tmp;
        idmef_additional_data_t *data;
        
        list_for_each(tmp, head) {
                data = list_entry(tmp, idmef_additional_data_t, list);

                prelude_msg_set(msg, MSG_ADDITIONALDATA_TAG, 0, NULL);
                send_uint32(msg, MSG_ADDITIONALDATA_TYPE, data->type);
                send_string(msg, MSG_ADDITIONALDATA_MEANING, data->meaning);
                send_string(msg, MSG_ADDITIONALDATA_DATA, data->data);
                prelude_msg_set(msg, MSG_END_OF_TAG, 0, NULL);
        }
}



static void send_web_service(prelude_msg_t *msg, idmef_webservice_t *web) 
{
        prelude_msg_set(msg, MSG_WEBSERVICE_TAG, 0, NULL);

        send_string(msg, MSG_WEBSERVICE_URL, web->url);
        send_string(msg, MSG_WEBSERVICE_CGI, web->cgi);
        send_string(msg, MSG_WEBSERVICE_HTTP_METHOD, web->http_method);
        send_string(msg, MSG_WEBSERVICE_ARG, web->arg);

        prelude_msg_set(msg, MSG_END_OF_TAG, 0, NULL);
}



static void send_snmp_service(prelude_msg_t *msg, idmef_snmpservice_t *snmp) 
{
        prelude_msg_set(msg, MSG_SNMPSERVICE_TAG, 0, NULL);

        send_string(msg, MSG_SNMPSERVICE_OID, snmp->oid);
        send_string(msg, MSG_SNMPSERVICE_COMMUNITY, snmp->community);
        send_string(msg, MSG_SNMPSERVICE_COMMAND, snmp->command);

        prelude_msg_set(msg, MSG_END_OF_TAG, 0, NULL);  
}





static void send_service(prelude_msg_t *msg, idmef_service_t *service) 
{
        if ( ! service )
                return;
        
        prelude_msg_set(msg, MSG_SERVICE_TAG, 0, NULL);

        send_uint64(msg, MSG_SERVICE_IDENT, &service->ident);
        send_string(msg, MSG_SERVICE_NAME, service->name);
        send_uint16(msg, MSG_SERVICE_PORT, service->port);
        send_string(msg, MSG_SERVICE_PORTLIST, service->portlist);
        send_string(msg, MSG_SERVICE_PROTOCOL, service->protocol);

        if ( service->type == web_service )
                send_web_service(msg, service->specific.web);

        else if ( service->type == snmp_service )
                send_snmp_service(msg, service->specific.snmp);
        
        prelude_msg_set(msg, MSG_END_OF_TAG, 0, NULL);
}




static void send_address(prelude_msg_t *msg, struct list_head *address_list) 
{
        struct list_head *tmp;
        idmef_address_t *address;
        
        list_for_each(tmp, address_list) {
                address = list_entry(tmp, idmef_address_t, list);

                prelude_msg_set(msg, MSG_ADDRESS_TAG, 0, NULL);
                send_uint32(msg, MSG_ADDRESS_CATEGORY, address->category);
                send_string(msg, MSG_ADDRESS_VLAN_NAME, address->vlan_name);
                send_uint32(msg, MSG_ADDRESS_VLAN_NUM, address->vlan_num);
                send_string(msg, MSG_ADDRESS_ADDRESS, address->address);
                send_string(msg, MSG_ADDRESS_NETMASK, address->netmask);
                prelude_msg_set(msg, MSG_END_OF_TAG, 0, NULL);
        }
}



static void send_string_list(prelude_msg_t *msg, uint8_t tag, struct list_head *head)
{
        struct list_head *tmp;
        idmef_string_t *string;
        
        list_for_each(tmp, head) {
                string = list_entry(tmp, idmef_string_t, list);
                send_string(msg, tag, string->string);
        }
}




static void send_process(prelude_msg_t *msg, idmef_process_t *process) 
{
        if ( ! process )
                return;
        
        prelude_msg_set(msg, MSG_PROCESS_TAG, 0, NULL);

        send_string(msg, MSG_PROCESS_NAME, process->name);
        send_uint32(msg, MSG_PROCESS_PID, process->pid);
        send_string(msg, MSG_PROCESS_PATH, process->path);
        send_string_list(msg, MSG_PROCESS_ARG, &process->arg_list);
        send_string_list(msg, MSG_PROCESS_ENV, &process->env_list);
        
        prelude_msg_set(msg, MSG_END_OF_TAG, 0, NULL);
}




static void send_node(prelude_msg_t *msg, idmef_node_t *node) 
{
        if ( ! node )
                return;
        
        prelude_msg_set(msg, MSG_NODE_TAG, 0, NULL);

        send_uint32(msg, MSG_NODE_CATEGORY, node->category);
        send_string(msg, MSG_NODE_LOCATION, node->location);
        send_string(msg, MSG_NODE_NAME, node->name);
        send_address(msg, &node->address_list);
        
        prelude_msg_set(msg, MSG_END_OF_TAG, 0, NULL);
}


static void send_userid(prelude_msg_t *msg, idmef_userid_t *uid) 
{
        prelude_msg_set(msg, MSG_USERID_TAG, 0, NULL);
        send_uint32(msg, MSG_USERID_TYPE, uid->type);
        send_string(msg, MSG_USERID_NAME, uid->name);
        send_uint32(msg, MSG_USERID_NUMBER, uid->number);
        prelude_msg_set(msg, MSG_END_OF_TAG, 0, NULL);
}




static void send_userid_list(prelude_msg_t *msg, struct list_head *head) 
{
        idmef_userid_t *uid;
        struct list_head *tmp;

        list_for_each(tmp, head) {
                uid = list_entry(tmp, idmef_userid_t, list);
                send_userid(msg, uid);
        }
}




static void send_user(prelude_msg_t *msg, idmef_user_t *user) 
{
        if ( ! user )
                return;
        
        prelude_msg_set(msg, MSG_USER_TAG, 0, NULL);
        send_uint32(msg, MSG_USER_CATEGORY, user->category);
        send_userid_list(msg, &user->userid_list);
        prelude_msg_set(msg, MSG_END_OF_TAG, 0, NULL);
}




static void send_source(prelude_msg_t *msg, struct list_head *head) 
{
        struct list_head *tmp;
        idmef_source_t *source;

        list_for_each(tmp, head) {
                source = list_entry(tmp, idmef_source_t, list);

                prelude_msg_set(msg, MSG_SOURCE_TAG, 0, NULL);

                send_uint64(msg, MSG_SOURCE_IDENT, &source->ident);
                send_uint32(msg, MSG_SOURCE_SPOOFED, source->spoofed);
                send_string(msg, MSG_SOURCE_INTERFACE, source->interface);
                send_node(msg, source->node);
                send_user(msg, source->user);
                send_process(msg, source->process);
                send_service(msg, source->service);

                prelude_msg_set(msg, MSG_END_OF_TAG, 0, NULL);
        }               
}



static void send_access_list(prelude_msg_t *msg, struct list_head *head) 
{
        struct list_head *tmp;
        idmef_file_access_t *access;

        list_for_each(tmp, head) {

                access = list_entry(tmp, idmef_file_access_t, list);
                
                prelude_msg_set(msg, MSG_ACCESS_TAG, 0, NULL);

                send_userid(msg, &access->userid);
                send_string(msg, MSG_ACCESS_PERMISSION, access->permission);

                prelude_msg_set(msg, MSG_END_OF_TAG, 0, NULL);
        }
}



static void send_linkage_list(prelude_msg_t *msg, struct list_head *head) 
{
        struct list_head *tmp;
        idmef_linkage_t *linkage;

        list_for_each(tmp, head) {
                linkage = list_entry(tmp, idmef_linkage_t, list);

                prelude_msg_set(msg, MSG_LINKAGE_TAG, 0, NULL);
                send_uint32(msg, MSG_LINKAGE_CATEGORY, linkage->category);
                send_string(msg, MSG_LINKAGE_NAME, linkage->name);
                send_string(msg, MSG_LINKAGE_PATH, linkage->path);
                send_file(msg, linkage->file);
                prelude_msg_set(msg, MSG_END_OF_TAG, 0, NULL);
        }
}



static void send_inode(prelude_msg_t *msg, idmef_inode_t *inode) 
{
        prelude_msg_set(msg, MSG_INODE_TAG, 0, NULL);
        
        send_time(msg, MSG_INODE_CHANGE_TIME, &inode->change_time);
        send_uint32(msg, MSG_INODE_NUMBER, inode->number);
        send_uint32(msg, MSG_INODE_MAJOR_DEVICE, inode->major_device);
        send_uint32(msg, MSG_INODE_MINOR_DEVICE, inode->minor_device);
        send_uint32(msg, MSG_INODE_C_MAJOR_DEVICE, inode->c_major_device);
        send_uint32(msg, MSG_INODE_C_MINOR_DEVICE, inode->c_minor_device);

        prelude_msg_set(msg, MSG_END_OF_TAG, 0, NULL);
}




static void send_file(prelude_msg_t *msg, idmef_file_t *file) 
{
        prelude_msg_set(msg, MSG_FILE_TAG, 0, NULL);

        send_uint64(msg, MSG_FILE_IDENT, &file->ident);
        send_uint32(msg, MSG_FILE_CATEGORY, file->category);
        send_string(msg, MSG_FILE_FSTYPE, file->fstype);
        send_string(msg, MSG_FILE_NAME, file->name);
        send_string(msg, MSG_FILE_PATH, file->path);
        send_time(msg, MSG_FILE_CREATE_TIME_TAG, file->create_time);
        send_time(msg, MSG_FILE_MODIFY_TIME_TAG, file->modify_time);
        send_time(msg, MSG_FILE_ACCESS_TIME_TAG, file->access_time);
        send_uint32(msg, MSG_FILE_DATASIZE, file->data_size);
        send_uint32(msg, MSG_FILE_DISKSIZE, file->disk_size);
        send_access_list(msg, &file->file_access_list);
        send_linkage_list(msg, &file->file_linkage_list);
        send_inode(msg, file->inode);
        
        prelude_msg_set(msg, MSG_END_OF_TAG, 0, NULL);
}




static void send_filelist(prelude_msg_t *msg, struct list_head *head) 
{
        idmef_file_t *file;
        struct list_head *tmp;

        list_for_each(tmp, head) {

                file = list_entry(tmp, idmef_file_t, list);
                
                prelude_msg_set(msg, MSG_FILE_TAG, 0, NULL);
                send_file(msg, file);
                prelude_msg_set(msg, MSG_END_OF_TAG, 0, NULL);
        }
}




static void send_target(prelude_msg_t *msg, struct list_head *head) 
{
        struct list_head *tmp;
        idmef_target_t *target;

        list_for_each(tmp, head) {
                target = list_entry(tmp, idmef_target_t, list);

                prelude_msg_set(msg, MSG_TARGET_TAG, 0, NULL);

                send_uint64(msg, MSG_TARGET_IDENT, &target->ident);
                send_uint32(msg, MSG_TARGET_DECOY, target->decoy);
                send_string(msg, MSG_TARGET_INTERFACE, target->interface);
                send_node(msg, target->node);
                send_user(msg, target->user);
                send_process(msg, target->process);
                send_service(msg, target->service);
                send_filelist(msg, &target->file_list);
                
                prelude_msg_set(msg, MSG_END_OF_TAG, 0, NULL);
        }               
}



static void send_analyzer(prelude_msg_t *msg, idmef_analyzer_t *analyzer) 
{
        prelude_msg_set(msg, MSG_ANALYZER_TAG, 0, NULL);
        
        send_string(msg, MSG_ANALYZER_MANUFACTURER, analyzer->manufacturer);
        send_string(msg, MSG_ANALYZER_MODEL, analyzer->model);
        send_string(msg, MSG_ANALYZER_VERSION, analyzer->version);
        send_string(msg, MSG_ANALYZER_CLASS, analyzer->class);
        send_string(msg, MSG_ANALYZER_OSTYPE, analyzer->ostype);
        send_string(msg, MSG_ANALYZER_OSVERSION, analyzer->osversion);
        
        send_node(msg, analyzer->node);
        send_process(msg, analyzer->process);
        
        prelude_msg_set(msg, MSG_END_OF_TAG, 0, NULL);
}




static void send_create_time(prelude_msg_t *msg, idmef_time_t *time) 
{
        struct timeval tv;

        gettimeofday(&tv, NULL);
        time->sec = tv.tv_sec;
        time->usec = tv.tv_usec;

        send_time(msg, MSG_CREATE_TIME_TAG, time);
}



static void send_classification(prelude_msg_t *msg, struct list_head *head) 
{
        struct list_head *tmp;
        idmef_classification_t *classification;

        list_for_each(tmp, head) {
                classification = list_entry(tmp, idmef_classification_t, list);
                prelude_msg_set(msg, MSG_CLASSIFICATION_TAG, 0, NULL);

                send_uint32(msg, MSG_CLASSIFICATION_ORIGIN, classification->origin);
                send_string(msg, MSG_CLASSIFICATION_NAME, classification->name);
                send_string(msg, MSG_CLASSIFICATION_URL, classification->url);

                prelude_msg_set(msg, MSG_END_OF_TAG, 0, NULL);
        }
}



static void send_confidence(prelude_msg_t *msg, idmef_confidence_t *confidence) 
{
        if ( ! confidence )
                return;

        prelude_msg_set(msg, MSG_CONFIDENCE_TAG, 0, NULL);

        send_uint32(msg, MSG_CONFIDENCE_RATING, confidence->rating);
        send_uint32(msg, MSG_CONFIDENCE_CONFIDENCE, confidence->confidence);
        
        prelude_msg_set(msg, MSG_END_OF_TAG, 0, NULL);
}




static void send_action_list(prelude_msg_t *msg, struct list_head *head) 
{
        struct list_head *tmp;
        idmef_action_t *action;

        list_for_each(tmp, head) {
                action = list_entry(tmp, idmef_action_t, list);

                prelude_msg_set(msg, MSG_ACTION_TAG, 0, NULL);
                send_uint32(msg, MSG_ACTION_CATEGORY, action->category);
                send_string(msg, MSG_ACTION_DESCRIPTION, action->description);
                prelude_msg_set(msg, MSG_END_OF_TAG, 0, NULL);
        }
}





static void send_impact(prelude_msg_t *msg, idmef_impact_t *impact) 
{
        if ( ! impact )
                return;

        prelude_msg_set(msg, MSG_IMPACT_TAG, 0, NULL);

        send_uint32(msg, MSG_IMPACT_SEVERITY, impact->severity);
        send_uint32(msg, MSG_IMPACT_COMPLETION, impact->completion);
        send_uint32(msg, MSG_IMPACT_TYPE, impact->type);
        send_string(msg, MSG_IMPACT_DESCRIPTION, impact->description);

        prelude_msg_set(msg, MSG_END_OF_TAG, 0, NULL);
}




static void send_assessment(prelude_msg_t *msg, idmef_assessment_t *assessment) 
{       
        if ( ! assessment )
                return;
        
        prelude_msg_set(msg, MSG_ASSESSMENT_TAG, 0, NULL);
        send_impact(msg, assessment->impact);
        send_action_list(msg, &assessment->action_list);
        send_confidence(msg, assessment->confidence);
        prelude_msg_set(msg, MSG_END_OF_TAG, 0, NULL);
}




static void send_alert(prelude_msg_t *msg, idmef_alert_t *alert) 
{
        prelude_msg_set(msg, MSG_ALERT_TAG, 0, NULL);

        send_assessment(msg, alert->assessment);
        send_analyzer(msg, &alert->analyzer);
        send_create_time(msg, &alert->create_time);
        send_time(msg, MSG_DETECT_TIME_TAG, alert->detect_time);
        send_time(msg, MSG_ANALYZER_TIME_TAG, alert->analyzer_time);
        send_source(msg, &alert->source_list);
        send_target(msg, &alert->target_list);
        send_classification(msg, &alert->classification_list);
        send_additional_data(msg, &alert->additional_data_list);

        prelude_msg_set(msg, MSG_END_OF_TAG, 0, NULL);
}




static void send_heartbeat(prelude_msg_t *msg, idmef_heartbeat_t *hb) 
{
        prelude_msg_set(msg, MSG_HEARTBEAT_TAG, 0, NULL);

        send_analyzer(msg, &hb->analyzer);
        send_create_time(msg, &hb->create_time);
        send_time(msg, MSG_ANALYZER_TIME_TAG, hb->analyzer_time);
        send_additional_data(msg, &hb->additional_data_list);
        
        prelude_msg_set(msg, MSG_END_OF_TAG, 0, NULL);
}


extern int msgcount, msglen;



void idmef_adjust_msg_size(size_t size) 
{
        msgcount += 1;
        msglen += size;
}




int idmef_msg_send(idmef_message_t *idmef, uint8_t priority)
{
        prelude_msg_t *msg;

        msgcount += 100;
        msglen += 2000;
        
        
        /*
         * we set create time ourself
         */
        msgcount += 2;
        msglen += 2 * sizeof(uint32_t);
        
        
        msg = prelude_msg_new(msgcount, msglen, PRELUDE_MSG_IDMEF, priority);
        if ( ! msg )
                return -1;
        
        switch ( idmef->type ) {

        case idmef_alert_message:
                send_alert(msg, idmef->message.alert);
                break;
                
        case idmef_heartbeat_message:
                send_heartbeat(msg, idmef->message.heartbeat);                
                break;
        }
        
        prelude_sensor_send_alert(msg);
                
        return 0;
}


