/*****
*
* Copyright (C) 2001-2002 Yoann Vandoorselaere <yoann@prelude-ids.org>
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
#include <stdlib.h>
#include <assert.h>
#include <time.h>
#include <sys/time.h>
#include <sys/utsname.h>
#include <inttypes.h>

#include "list.h"
#include "prelude-log.h"
#include "idmef-tree.h"
#include "idmef-tree-func.h"
#include "prelude-io.h"
#include "prelude-message.h"
#include "prelude-client.h"

#define generic_free_list(type, head) do {           \
        type *decl;                                  \
        struct list_head *tmp;                       \
                                                     \
        for (tmp = (head)->next; tmp != (head); ) {  \
                decl = list_entry(tmp, type, list);  \
                tmp = tmp->next;                     \
                free(decl);                          \
        }                                            \
} while (0)



static void free_file(idmef_file_t *file);



static idmef_heartbeat_t heartbeat;

static idmef_alert_t alert;
static idmef_tool_alert_t tool_alert;
static idmef_overflow_alert_t overflow_alert;
static idmef_correlation_alert_t correlation_alert;


static idmef_node_t analyzer_node;
static idmef_process_t analyzer_process;

static idmef_detect_time_t static_detect_time;
static idmef_analyzer_time_t static_analyzer_time;

static idmef_impact_t static_impact;
static idmef_confidence_t static_confidence;
static idmef_assessment_t static_assessment;


static void free_service(idmef_service_t *service) 
{
        switch (service->type) {

        case web_service:
                free(service->specific.web);
                break;

        case snmp_service:
                free(service->specific.snmp);
                break;

        default:
                break;
        }

        free(service);
}



static void free_time(idmef_time_t *time) 
{
        if ( ! time )
                return;

        free(time);
}



static void free_inode(idmef_inode_t *inode) 
{
        if ( ! inode )
                return;

        free_time(inode->change_time);
        free(inode);
}



static void free_linkage(idmef_linkage_t *linkage)
{
        if ( ! linkage )
                return;
        
        free_file(linkage->file);
        free(linkage);
}


static void free_access(idmef_file_access_t *access) 
{
        if ( ! access )
                return;
        
        generic_free_list(idmef_string_item_t, &access->permission_list);
        free(access);
}



static void free_file(idmef_file_t *file) 
{
        idmef_linkage_t *linkage;
        idmef_file_access_t *access;
        struct list_head *tmp, *bkp;
        
        list_for_each_safe(tmp, bkp, &file->file_access_list) {
                access = list_entry(tmp, idmef_file_access_t, list);
                free_access(access);
        }

        list_for_each_safe(tmp, bkp, &file->file_linkage_list) {
                linkage = list_entry(tmp, idmef_linkage_t, list);
                free_linkage(linkage);
        }

        free_inode(file->inode);
        free_time(file->create_time);
        free_time(file->modify_time);
        free_time(file->access_time);

        free(file);
}



static void free_source_or_target(char type, struct list_head *source_list) 
{
        struct list_head *tmp;
        idmef_source_t *source;

        for ( tmp = source_list->next; tmp != source_list; ) {
                source = list_entry(tmp, idmef_source_t, list);

                if ( source->user ) {
                        generic_free_list(idmef_userid_t, &source->user->userid_list);
                        free(source->user);
                }
                
                if ( source->node ) {
                        generic_free_list(idmef_address_t, &source->node->address_list);
                        free(source->node);
                }
                
                if ( source->process ) {
                        generic_free_list(idmef_process_env_t, &source->process->env_list);
                        generic_free_list(idmef_process_arg_t, &source->process->arg_list);
                        free(source->process);
                }

                if ( source->service )
                        free_service(source->service);

                if ( type == 'T' )
                        generic_free_list(idmef_file_t, &((idmef_target_t *) source)->file_list);
                
                tmp = tmp->next;
                free(source);
        }
}



static void free_analyzer(idmef_analyzer_t *analyzer) 
{        
        if ( analyzer->node ) 
                generic_free_list(idmef_address_t, &analyzer->node->address_list);
        
        if ( analyzer->process ) {
                generic_free_list(idmef_process_env_t, &analyzer->process->env_list);
                generic_free_list(idmef_process_arg_t, &analyzer->process->arg_list);
        }
}



static void free_assessment(idmef_assessment_t *assessment) 
{
        generic_free_list(idmef_action_t, &assessment->action_list);
}



static void free_alert(idmef_alert_t *alert) 
{
        if ( alert->assessment )
                free_assessment(alert->assessment);
        
        free_source_or_target('S', &alert->source_list);
        free_source_or_target('T', &alert->target_list);
        
        generic_free_list(idmef_classification_t, &alert->classification_list);
        generic_free_list(idmef_additional_data_t, &alert->additional_data_list);
        
        switch (alert->type) {
                
        case idmef_correlation_alert:
                generic_free_list(idmef_alertident_t, &alert->detail.correlation_alert->alertident_list);
                break;

        case idmef_tool_alert:
                generic_free_list(idmef_alertident_t, &alert->detail.correlation_alert->alertident_list);
                break;

        default:
                break;
        }

        free_analyzer(&alert->analyzer);
}



static void free_heartbeat(idmef_heartbeat_t *heartbeat) 
{
        free_analyzer(&heartbeat->analyzer);
        generic_free_list(idmef_additional_data_t, &heartbeat->additional_data_list);
}




idmef_alertident_t *idmef_tool_alert_alertident_new(idmef_tool_alert_t *tool_alert) 
{
        idmef_alertident_t *new;

        new = calloc(1, sizeof(*new));
        if ( ! new ) {
                log(LOG_ERR, "memory exhausted.\n");
                return NULL;
        }

        list_add_tail(&new->list, &tool_alert->alertident_list);

        return new;
}




void idmef_tool_alert_new(idmef_alert_t *alert) 
{
        alert->type = idmef_tool_alert;
        alert->detail.tool_alert = &tool_alert;
}




/*
 * Correlation Alert stuff
 */
idmef_alertident_t *idmef_correlation_alert_alertident_new(idmef_correlation_alert_t *correlation_alert) 
{
        idmef_alertident_t *new;

        new = calloc(1, sizeof(*new));
        if ( ! new ) {
                log(LOG_ERR, "memory exhausted.\n");
                return NULL;
        }

        list_add_tail(&new->list, &correlation_alert->alertident_list);

        return new;
}


void idmef_correlation_alert_new(idmef_alert_t *alert) 
{
        alert->type = idmef_correlation_alert;
        alert->detail.correlation_alert = &correlation_alert;
}



/*
 * Overflow alert stuff
 */
void idmef_overflow_alert_new(idmef_alert_t *alert) 
{
        alert->type = idmef_overflow_alert;
        alert->detail.overflow_alert = &overflow_alert;
}


/*
 * IDMEF webservice
 */
idmef_webservice_arg_t *idmef_webservice_arg_new(idmef_webservice_t *w)
{
        idmef_webservice_arg_t *new;

        new = calloc(1, sizeof(*new));
        if ( ! new ) {
                log(LOG_ERR, "memory exhausted.\n");
                return NULL;
        }
        
        list_add(&new->list, &w->arg_list);

        return new;
}



/*
 * IDMEF Service.
 */
idmef_webservice_t *idmef_service_webservice_new(idmef_service_t *service)
{
        idmef_webservice_t *new;
        
        new = calloc(1, sizeof(*new));
        if ( ! new ) {
                log(LOG_ERR, "memory exhausted.\n");
                return NULL;
        }

        INIT_LIST_HEAD(&new->arg_list);
        
        service->type = web_service;
        service->specific.web = new;

        return new;
}



idmef_snmpservice_t *idmef_service_snmpservice_new(idmef_service_t *service)
{
        idmef_snmpservice_t *new;
        
        new = calloc(1, sizeof(*new));
        if ( ! new ) {
                log(LOG_ERR, "memory exhausted.\n");
                return NULL;
        }

        service->type = snmp_service;
        service->specific.snmp = new;

        return new;
}



/*
 * IDMEF Node.
 */
idmef_address_t *idmef_node_address_new(idmef_node_t *node) 
{
        idmef_address_t *new;

        new = calloc(1, sizeof(*new));
        if ( ! new ) {
                log(LOG_ERR, "memory exhausted.\n");
                return NULL;
        }

        new->category = unknown;
        list_add_tail(&new->list, &node->address_list);

        return new;
}




/*
 * IDMEF Process.
 */
idmef_process_arg_t *idmef_process_arg_new(idmef_process_t *process) 
{
        idmef_process_arg_t *new;

        new = calloc(1, sizeof(*new));
        if ( ! new ) {
                log(LOG_ERR, "memory exhausted.\n");
                return NULL;
        }

        list_add_tail(&new->list, &process->arg_list);

        return new;
}




idmef_process_env_t *idmef_process_env_new(idmef_process_t *process) 
{
        idmef_process_env_t *new;

        new = calloc(1, sizeof(*new));
        if ( ! new ) {
                log(LOG_ERR, "memory exhausted.\n");
                return NULL;
        }

        list_add_tail(&new->list, &process->env_list);

        return new;
}




/*
 * IDMEF User.
 */
idmef_userid_t *idmef_user_userid_new(idmef_user_t *user) 
{
        idmef_userid_t *new;

        new = calloc(1, sizeof(*new));
        if ( ! new ) {
                log(LOG_ERR, "memory exhausted.\n");
                return NULL;
        }

        new->type = original_user;
        list_add_tail(&new->list, &user->userid_list);

        return new;
}




/*
 * IDMEF Source.
 * we don't use statically allocated data for data within the source
 * structure, cause there can be several source within an alert.
 */
idmef_service_t *idmef_source_service_new(idmef_source_t *source) 
{
        idmef_service_t *new;
        
        new = calloc(1, sizeof(*new));
        if ( ! new ) {
                log(LOG_ERR, "memory exhausted.\n");
                return NULL;
        }

        source->service = new;
        new->type = no_specific_service;

        return new;
}



idmef_user_t *idmef_source_user_new(idmef_source_t *source) 
{
        idmef_user_t *new;

        new = calloc(1, sizeof(*new));
        if ( ! new ) {
                log(LOG_ERR, "memory exhausted.\n");
                return NULL;
        }

        source->user = new;
        new->category = unknown;
        INIT_LIST_HEAD(&new->userid_list);

        return new;
}


idmef_node_t *idmef_source_node_new(idmef_source_t *source) 
{
        idmef_node_t *new;

        new = calloc(1, sizeof(*new));
        if ( ! new ) {
                log(LOG_ERR, "memory exhausted.\n");
                return NULL;
        }

        source->node = new;
        new->category = unknown;
        INIT_LIST_HEAD(&new->address_list);

        return new;
}


idmef_process_t *idmef_source_process_new(idmef_source_t *source) 
{
        idmef_process_t *new;

        new = calloc(1, sizeof(*new));
        if ( ! new ) {
                log(LOG_ERR, "memory exhausted.\n");
                return NULL;
        }

        source->process = new;
        INIT_LIST_HEAD(&new->arg_list);
        INIT_LIST_HEAD(&new->env_list);
        
        return new;
}



/*
 * IDMEF Analyzer.
 */
void idmef_analyzer_node_new(idmef_analyzer_t *analyzer) 
{
        analyzer->node = &analyzer_node;
        analyzer_node.category = unknown;
        INIT_LIST_HEAD(&analyzer_node.address_list);
}



void idmef_analyzer_process_new(idmef_analyzer_t *analyzer) 
{
        analyzer->process = &analyzer_process;
        INIT_LIST_HEAD(&analyzer_process.arg_list);
        INIT_LIST_HEAD(&analyzer_process.env_list);
}



/*
 * IDMEF assessment
 */
void idmef_assessment_confidence_new(idmef_assessment_t *assessment) 
{
        assessment->confidence = &static_confidence;
        memset(&static_confidence, 0, sizeof(static_confidence));
}



idmef_action_t *idmef_assessment_action_new(idmef_assessment_t *assessment) 
{
        idmef_action_t *new;

        new = calloc(1, sizeof(*new));
        if ( ! new ) {
                log(LOG_ERR, "memory exhausted.\n");
                return NULL;
        }

        list_add_tail(&new->list, &assessment->action_list);

        return new;
}


void idmef_assessment_impact_new(idmef_assessment_t *assessment) 
{
        memset(&static_impact, 0, sizeof(static_impact));
        assessment->impact = &static_impact;
}


/*
 * IDMEF Inode.
 */
idmef_time_t *idmef_inode_change_time_new(idmef_inode_t *inode) 
{
        idmef_time_t *new;

        assert(inode->change_time == NULL);
        
        new = calloc(1, sizeof(*new));
        if ( ! new ) {
                log(LOG_ERR, "memory exhausted.\n");
                return NULL;
        }

        inode->change_time = new;

        return new;
}




/*
 * IDMEF Linkage.
 */
idmef_file_t *idmef_linkage_file_new(idmef_linkage_t *linkage) 
{
        assert(linkage->file == NULL);
        
        linkage->file = calloc(1, sizeof(*linkage->file));
        if ( ! linkage->file ) {
                log(LOG_ERR, "memory exhausted.\n");
                return NULL;
        }

        INIT_LIST_HEAD(&linkage->file->file_access_list);
        INIT_LIST_HEAD(&linkage->file->file_linkage_list);

        return linkage->file;
}



/*
 * IDMEF Access.
 */
idmef_file_access_permission_t *idmef_file_access_permission_new(idmef_file_access_t *access) 
{
        idmef_file_access_permission_t *new;

        new = calloc(1, sizeof(*new));
        if ( ! new ) {
                log(LOG_ERR, "memory exhausted.\n");
                return NULL;
        }

        list_add_tail(&new->list, &access->permission_list);

        return 0;
}



/*
 * IDMEF File.
 */
idmef_time_t *idmef_file_create_time_new(idmef_file_t *file) 
{
        idmef_time_t *new;

        assert(file->create_time == NULL);
        
        new = calloc(1, sizeof(*new));
        if ( ! new ) {
                log(LOG_ERR, "memory exhausted.\n");
                return NULL;
        }

        file->create_time = new;

        return new;
}



idmef_time_t *idmef_file_modify_time_new(idmef_file_t *file) 
{
        idmef_time_t *new;
        
        assert(file->modify_time == NULL);

        new = calloc(1, sizeof(*new));
        if ( ! new ) {
                log(LOG_ERR, "memory exhausted.\n");
                return NULL;
        }

        file->modify_time = new;

        return new;
}



idmef_time_t *idmef_file_access_time_new(idmef_file_t *file) 
{
        idmef_time_t *new;
        
        assert(file->access_time == NULL);

        new = calloc(1, sizeof(*new));
        if ( ! new ) {
                log(LOG_ERR, "memory exhausted.\n");
                return NULL;
        }

        file->access_time = new;

        return new;
}



idmef_inode_t *idmef_file_inode_new(idmef_file_t *file) 
{
        idmef_inode_t *new;

        new = calloc(1, sizeof(*new));
        if ( ! new ) {
                log(LOG_ERR, "memory exhausted.\n");
                return NULL;
        }

        assert(file->inode == NULL);
        file->inode = new;

        return new;
}



idmef_file_access_t *idmef_file_access_new(idmef_file_t *file) 
{
        idmef_file_access_t *new;

        new = calloc(1, sizeof(*new));
        if ( ! new ) {
                log(LOG_ERR, "memory exhausted.\n");
                return NULL;
        }

        list_add_tail(&new->list, &file->file_access_list);

        return 0;
}




idmef_linkage_t *idmef_file_linkage_new(idmef_file_t *file) 
{
        idmef_linkage_t *new;

        new = calloc(1, sizeof(*new));
        if ( ! new ) {
                log(LOG_ERR, "memory exhausted.\n");
                return NULL;
        }

        list_add_tail(&new->list, &file->file_linkage_list);

        return 0;
}




/*
 * IDMEF Target
 */
idmef_file_t *idmef_target_file_new(idmef_target_t *target) 
{
        idmef_file_t *new;

        new = calloc(1, sizeof(*new));
        if ( ! new ) {
                log(LOG_ERR, "memory exhausted.\n");
                return NULL;
        }

        INIT_LIST_HEAD(&new->file_access_list);
        INIT_LIST_HEAD(&new->file_linkage_list);
        
        list_add_tail(&new->list, &target->file_list);

        return new;
}



/*
 * IDMEF Alert
 */
void idmef_alert_assessment_new(idmef_alert_t *alert) 
{
        alert->assessment = &static_assessment;
}



void idmef_alert_detect_time_new(idmef_alert_t *alert) 
{
        alert->detect_time = &static_detect_time;
}



void idmef_alert_analyzer_time_new(idmef_alert_t *alert) 
{
        alert->analyzer_time = &static_analyzer_time;
}



idmef_target_t *idmef_alert_target_new(idmef_alert_t *alert) 
{
        idmef_target_t *new;

        new = calloc(1, sizeof(*new));
        if ( ! new ) {
                log(LOG_ERR, "memory exhausted.\n");
                return NULL;
        }

        new->decoy = unknown;
        INIT_LIST_HEAD(&new->file_list);
        list_add_tail(&new->list, &alert->target_list);
        
        return new;
}




idmef_source_t *idmef_alert_source_new(idmef_alert_t *alert) 
{
        idmef_source_t *new;

        new = calloc(1, sizeof(*new));
        if ( ! new ) {
                log(LOG_ERR, "memory exhausted.\n");
                return NULL;
        }

        new->spoofed = unknown;
        list_add_tail(&new->list, &alert->source_list);
        
        return new;
}



idmef_classification_t *idmef_alert_classification_new(idmef_alert_t *alert) 
{
        idmef_classification_t *new;

        new = calloc(1, sizeof(*new));
        if ( ! new ) {
                log(LOG_ERR, "memory exhausted.\n");
                return NULL;
        }

        new->origin = unknown;
        list_add_tail(&new->list, &alert->classification_list);
        
        return new;
}




idmef_additional_data_t *idmef_alert_additional_data_new(idmef_alert_t *alert)  
{
        idmef_additional_data_t *new;

        new = calloc(1, sizeof(*new));
        if ( ! new ) {
                log(LOG_ERR, "memory exhausted.\n");
                return NULL;
        }

        new->type = string;
        list_add_tail(&new->list, &alert->additional_data_list);
        
        return new;
}




void idmef_alert_new(idmef_message_t *message) 
{
        struct timeval tv;

        gettimeofday(&tv, NULL);
        alert.create_time.sec = tv.tv_sec;
        alert.create_time.usec = tv.tv_usec;
        
        message->message.alert = &alert;
        message->type = idmef_alert_message;
        alert.analyzer.analyzerid = prelude_client_get_analyzerid();
}




/*
 * IDMEF Heartbeat
 */
idmef_additional_data_t *idmef_heartbeat_additional_data_new(idmef_heartbeat_t *hb)  
{
        idmef_additional_data_t *new;

        new = calloc(1, sizeof(*new));
        if ( ! new ) {
                log(LOG_ERR, "memory exhausted.\n");
                return NULL;
        }

        list_add_tail(&new->list, &hb->additional_data_list);
        new->type = string;
        
        return new;
}



void idmef_heartbeat_analyzer_time_new(idmef_heartbeat_t *heartbeat) 
{
        heartbeat->analyzer_time = &static_analyzer_time;
}



void idmef_heartbeat_new(idmef_message_t *message) 
{
        message->message.heartbeat = &heartbeat;
        message->type = idmef_heartbeat_message;
}



/*
 * IDMEF message
 */
idmef_message_t *idmef_message_new(void) 
{
        static idmef_message_t msg;
         
        /*
         * set static variable data to 0
         */
        memset(&msg, 0, sizeof(msg));
        memset(&alert, 0, sizeof(alert));
        memset(&static_detect_time, 0, sizeof(static_detect_time));
        memset(&static_analyzer_time, 0, sizeof(static_analyzer_time));
        memset(&analyzer_node, 0, sizeof(analyzer_node));
        memset(&analyzer_process, 0, sizeof(analyzer_process));

        /*
         * assessment
         */
        memset(&static_assessment, 0, sizeof(static_assessment));
        INIT_LIST_HEAD(&static_assessment.action_list);
        
        /*
         * Re-initialize alert.
         */
        INIT_LIST_HEAD(&alert.source_list);
        INIT_LIST_HEAD(&alert.target_list);
        INIT_LIST_HEAD(&alert.classification_list);
        INIT_LIST_HEAD(&alert.additional_data_list);

        /*
         * tool alert.
         */
        memset(&tool_alert, 0, sizeof(tool_alert));        
        INIT_LIST_HEAD(&tool_alert.alertident_list);

        /*
         * overflow alert
         */
        memset(&overflow_alert, 0, sizeof(overflow_alert));

        /*
         * correlation alert
         */
        memset(&correlation_alert, 0, sizeof(correlation_alert));
        INIT_LIST_HEAD(&correlation_alert.alertident_list);

        /*
         * heartbeat.
         */
        memset(&heartbeat, 0, sizeof(heartbeat));
        INIT_LIST_HEAD(&heartbeat.additional_data_list);

        return &msg;
}



/**
 * idmef_message_free:
 * @msg: Pointer to an IDMEF message to free.
 *
 * free the @msg IDMEF message.
 */
void idmef_message_free(idmef_message_t *msg) 
{
        switch ( msg->type ) {
        case idmef_alert_message:
                free_alert(msg->message.alert);
                break;

        case idmef_heartbeat_message:
                free_heartbeat(msg->message.heartbeat);
                break;
        }
}
