/*****
*
* Copyright (C) 2003, 2004, 2005 PreludeIDS Technologies. All Rights Reserved.
* Author: Nicolas Delon <nicolas.delon@prelude-ids.com>
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
* You should have received a copy of the GNU General Public License
* along with this program; see the file COPYING.  If not, write to
* the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
*
*****/

%{
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <limits.h>

#include "prelude.h"
#include "prelude-log.h"
#include "prelude-msg.h"
#include "prelude-option.h"
#include "prelude-option-wide.h"
#include "idmef.h"
#include "idmef-message-write.h"
#include "idmef-message-print.h"
#include "idmef-additional-data.h"
#include "idmef-tree-wrap.h"
#include "prelude-inttypes.h"
%}


%constant int IDMEF_LIST_APPEND = IDMEF_LIST_APPEND;
%constant int IDMEF_LIST_PREPEND = IDMEF_LIST_PREPEND;


typedef char int8_t;
typedef unsigned char uint8_t;

typedef short int16_t;
typedef unsigned short uint16_t;

typedef int int32_t;
typedef unsigned int uint32_t;

typedef long long int64_t;
typedef unsigned long long uint64_t;


%typemap(in, numinputs=0) (uint64_t *source_id, uint32_t *request_id, void **value) (uint64_t tmp_source_id, uint32_t tmp_request_id, void *tmp_value) {
	tmp_source_id = 0;
	tmp_request_id = 0;
	$1 = &tmp_source_id;
	$2 = &tmp_request_id;
	$3 = &tmp_value;
};



%include idmef-value-class-mapping.i


#ifdef SWIGPYTHON
%include libprelude_python.i
#endif /* ! SWIGPYTHON */


#ifdef SWIGPERL
%include libprelude_perl.i
#endif /* ! SWIGPERL */

/*
 * Double pointer typemap
 */
%apply SWIGTYPE **OUTPARAM {
	prelude_client_t **,
	prelude_client_profile_t **, 
	prelude_msgbuf_t **,
	prelude_msg_t **,
	prelude_connection_t **,
	idmef_path_t **,
	idmef_value_t **,
	idmef_criteria_t **,
	idmef_time_t **,
        idmef_data_t **
};


%apply SWIGTYPE **OUTPARAM {
	idmef_additional_data_t **,
	idmef_reference_t **,
	idmef_classification_t **,
	idmef_user_id_t **,
	idmef_user_t **,
	idmef_address_t **,
	idmef_process_t **,
	idmef_web_service_t **,
	idmef_snmp_service_t **,
	idmef_service_t **,
	idmef_node_t **,
	idmef_source_t **,
	idmef_file_access_t **,
	idmef_inode_t **,
	idmef_linkage_t **,
	idmef_checksum_t **,
	idmef_file_t **,
	idmef_target_t **,
	idmef_analyzer_t **,
	idmef_alertident_t **,
	idmef_impact_t **,
	idmef_action_t **,
	idmef_confidence_t **,
	idmef_assessment_t **,
	idmef_tool_alert_t **,
	idmef_correlation_alert_t **,
	idmef_overflow_alert_t **,
	idmef_alert_t **,
	idmef_heartbeat_t **,
	idmef_message_t **
};


/*
 * Check for nil input
 */
%apply SWIGTYPE *INPARAM {
	idmef_additional_data_t *,
	idmef_reference_t *,
	idmef_classification_t *,
	idmef_user_id_t *,
	idmef_user_t *,
	idmef_address_t *,
	idmef_process_t *,
	idmef_web_service_t *,
	idmef_snmp_service_t *,
	idmef_service_t *,
	idmef_node_t *,
	idmef_source_t *,
	idmef_file_access_t *,
	idmef_inode_t *,
	idmef_linkage_t *,
	idmef_checksum_t *,
	idmef_file_t *,
	idmef_target_t *,
	idmef_analyzer_t *,
	idmef_alertident_t **,
	idmef_impact_t *,
	idmef_action_t *,
	idmef_confidence_t *,
	idmef_assessment_t *,
	idmef_tool_alert_t *,
	idmef_correlation_alert_t *,
	idmef_overflow_alert_t *,
	idmef_alert_t *,
	idmef_heartbeat_t *,
	idmef_message_t *,
	prelude_client_t *,
	prelude_client_profile_t *, 
	prelude_msgbuf_t *,
	prelude_msg_t *,
	prelude_connection_t *,
	idmef_path_t *,
	idmef_value_t *,
	idmef_criteria_t *,
	idmef_time_t *,
        idmef_data_t *
};


%ignore idmef_path_new_v;
%ignore prelude_string_vprintf;
%ignore _prelude_log_v;


%include "prelude.h"
%include "prelude-client.h"
%include "prelude-client-profile.h"
%include "idmef-tree-wrap.h"
%include "idmef-value.h"
%include "idmef-path.h"
%include "idmef-time.h"
%include "idmef-data.h"
%include "idmef-criteria.h"
%include "idmef-message-read.h"
%include "idmef-message-write.h"
%include "idmef-additional-data.h"
%include "idmef-value-type.h"
%include "idmef-class.h"
%include "prelude-connection.h"
%include "prelude-connection-pool.h"
%include "prelude-option.h"
%include "prelude-option-wide.h"
%include "prelude-msg.h"
%include "prelude-message-id.h"
%include "prelude-log.h"
%include "prelude-msgbuf.h"
%include "prelude-timer.h"

typedef signed int prelude_error_t;

const char *prelude_strerror(prelude_error_t err);
const char *prelude_strsource(prelude_error_t err);
int prelude_error_code_to_errno(prelude_error_code_t code);

