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

void idmef_send_string(prelude_msg_t *msg, uint8_t tag, idmef_string_t *string);
void idmef_send_uint64(prelude_msg_t *msg, uint8_t tag, uint64_t *data);
void idmef_send_uint32(prelude_msg_t *msg, uint8_t tag, uint32_t data);
void idmef_send_uint16(prelude_msg_t *msg, uint8_t tag, uint16_t data);
void idmef_send_time(prelude_msg_t *msg, uint8_t tag, idmef_time_t *time);
void idmef_send_additional_data(prelude_msg_t *msg, idmef_additional_data_t *data);
void idmef_send_additional_data_list(prelude_msg_t *msg, struct list_head *head);
void idmef_send_web_service(prelude_msg_t *msg, idmef_webservice_t *web) 
;
void idmef_send_snmp_service(prelude_msg_t *msg, idmef_snmpservice_t *snmp);
void idmef_send_service(prelude_msg_t *msg, idmef_service_t *service);

void idmef_send_address(prelude_msg_t *msg, idmef_address_t *address);
void idmef_send_address_list(prelude_msg_t *msg, struct list_head *address_list);
void idmef_send_string_list(prelude_msg_t *msg, uint8_t tag, struct list_head *head);
void idmef_send_process(prelude_msg_t *msg, idmef_process_t *process);
void idmef_send_node(prelude_msg_t *msg, idmef_node_t *node);
void idmef_send_userid(prelude_msg_t *msg, idmef_userid_t *uid);
void idmef_send_userid_list(prelude_msg_t *msg, struct list_head *head);
void idmef_send_user(prelude_msg_t *msg, idmef_user_t *user);
void idmef_send_source(prelude_msg_t *msg, idmef_source_t *source);
void idmef_send_source_list(prelude_msg_t *msg, struct list_head *head);
void idmef_send_file_access(prelude_msg_t *msg, idmef_file_access_t *access);
void idmef_send_file_access_list(prelude_msg_t *msg, struct list_head *head);
void idmef_send_linkage(prelude_msg_t *msg, idmef_linkage_t *linkage);
void idmef_send_linkage_list(prelude_msg_t *msg, struct list_head *head);
void idmef_send_inode(prelude_msg_t *msg, idmef_inode_t *inode);
void idmef_send_file(prelude_msg_t *msg, idmef_file_t *file);
void idmef_send_file_list(prelude_msg_t *msg, struct list_head *head);
void idmef_send_target(prelude_msg_t *msg, idmef_target_t *target);
void idmef_send_target_list(prelude_msg_t *msg, struct list_head *head);
void idmef_send_analyzer(prelude_msg_t *msg, idmef_analyzer_t *analyzer);
void idmef_send_create_time(prelude_msg_t *msg, idmef_time_t *time);
void idmef_send_classification(prelude_msg_t *msg, idmef_classification_t *classification);
void idmef_send_classification_list(prelude_msg_t *msg, struct list_head *head);
void idmef_send_confidence(prelude_msg_t *msg, idmef_confidence_t *confidence);
void idmef_send_action(prelude_msg_t *msg, idmef_action_t *action);
void idmef_send_action_list(prelude_msg_t *msg, struct list_head *head);
void idmef_send_impact(prelude_msg_t *msg, idmef_impact_t *impact);
void idmef_send_alert(prelude_msg_t *msg, idmef_alert_t *alert);
void idmef_send_heartbeat(prelude_msg_t *msg, idmef_heartbeat_t *hb);
void idmef_send_assessment(prelude_msg_t *msg, idmef_assessment_t *assessment);

int idmef_msg_send(idmef_message_t *idmef, uint8_t priority);
