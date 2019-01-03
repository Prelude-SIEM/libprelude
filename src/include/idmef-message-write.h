
/*****
*
* Copyright (C) 2001-2019 CS-SI. All Rights Reserved.
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
* You should have received a copy of the GNU General Public License along
* with this program; if not, write to the Free Software Foundation, Inc.,
* 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*
*****/

/* Auto-generated by the GenerateIDMEFMessageWriteH package */

#ifndef _LIBPRELUDE_IDMEF_MESSAGE_WRITE_H
#define _LIBPRELUDE_IDMEF_MESSAGE_WRITE_H

#include "prelude-inttypes.h"
#include "idmef-time.h"
#include "prelude-string.h"
#include "prelude-msgbuf.h"

#ifdef __cplusplus
 extern "C" {
#endif

int idmef_additional_data_write(idmef_additional_data_t *additional_data, prelude_msgbuf_t *msg);
int idmef_reference_write(idmef_reference_t *reference, prelude_msgbuf_t *msg);
int idmef_classification_write(idmef_classification_t *classification, prelude_msgbuf_t *msg);
int idmef_user_id_write(idmef_user_id_t *user_id, prelude_msgbuf_t *msg);
int idmef_user_write(idmef_user_t *user, prelude_msgbuf_t *msg);
int idmef_address_write(idmef_address_t *address, prelude_msgbuf_t *msg);
int idmef_process_write(idmef_process_t *process, prelude_msgbuf_t *msg);
int idmef_web_service_write(idmef_web_service_t *web_service, prelude_msgbuf_t *msg);
int idmef_snmp_service_write(idmef_snmp_service_t *snmp_service, prelude_msgbuf_t *msg);
int idmef_service_write(idmef_service_t *service, prelude_msgbuf_t *msg);
int idmef_node_write(idmef_node_t *node, prelude_msgbuf_t *msg);
int idmef_source_write(idmef_source_t *source, prelude_msgbuf_t *msg);
int idmef_file_access_write(idmef_file_access_t *file_access, prelude_msgbuf_t *msg);
int idmef_inode_write(idmef_inode_t *inode, prelude_msgbuf_t *msg);
int idmef_checksum_write(idmef_checksum_t *checksum, prelude_msgbuf_t *msg);
int idmef_file_write(idmef_file_t *file, prelude_msgbuf_t *msg);
int idmef_linkage_write(idmef_linkage_t *linkage, prelude_msgbuf_t *msg);
int idmef_target_write(idmef_target_t *target, prelude_msgbuf_t *msg);
int idmef_analyzer_write(idmef_analyzer_t *analyzer, prelude_msgbuf_t *msg);
int idmef_alertident_write(idmef_alertident_t *alertident, prelude_msgbuf_t *msg);
int idmef_impact_write(idmef_impact_t *impact, prelude_msgbuf_t *msg);
int idmef_action_write(idmef_action_t *action, prelude_msgbuf_t *msg);
int idmef_confidence_write(idmef_confidence_t *confidence, prelude_msgbuf_t *msg);
int idmef_assessment_write(idmef_assessment_t *assessment, prelude_msgbuf_t *msg);
int idmef_tool_alert_write(idmef_tool_alert_t *tool_alert, prelude_msgbuf_t *msg);
int idmef_correlation_alert_write(idmef_correlation_alert_t *correlation_alert, prelude_msgbuf_t *msg);
int idmef_overflow_alert_write(idmef_overflow_alert_t *overflow_alert, prelude_msgbuf_t *msg);
int idmef_alert_write(idmef_alert_t *alert, prelude_msgbuf_t *msg);
int idmef_heartbeat_write(idmef_heartbeat_t *heartbeat, prelude_msgbuf_t *msg);
int idmef_message_write(idmef_message_t *message, prelude_msgbuf_t *msg);


#ifdef __cplusplus
 }
#endif

#endif /* _LIBPRELUDE_IDMEF_MESSAGE_WRITE_H */
