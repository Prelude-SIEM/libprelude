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

#ifndef _LIBPRELUDE_IDMEF_MESSAGE_ID_H
#define _LIBPRELUDE_IDMEF_MESSAGE_ID_H

/*
 * Tag value
 */
#define MSG_ALERT_TAG                                    0
#define MSG_CREATE_TIME_TAG                              1
#define MSG_DETECT_TIME_TAG                              2
#define MSG_ANALYZER_TIME_TAG                            3
#define MSG_SOURCE_TAG                                   4
#define MSG_NODE_TAG                                     5
#define MSG_ADDRESS_TAG                                  6
#define MSG_USER_TAG                                     7
#define MSG_USERID_TAG                                   8
#define MSG_PROCESS_TAG                                  9
#define MSG_SERVICE_TAG                                 10
#define MSG_WEBSERVICE_TAG                              11
#define MSG_SNMPSERVICE_TAG                             12
#define MSG_CLASSIFICATION_TAG                          13
#define MSG_ADDITIONALDATA_TAG                          14
#define MSG_ANALYZER_TAG                                15
#define MSG_TARGET_TAG                                  16
#define MSG_HEARTBEAT_TAG                               17
#define MSG_TOOL_ALERT_TAG                              18
#define MSG_CORRELATION_ALERT_TAG                       19
#define MSG_OVERFLOW_ALERT_TAG                          20
#define MSG_ALERTIDENT_TAG                              21
#define MSG_FILE_TAG                                    22
#define MSG_ACCESS_TAG                                  23
#define MSG_LINKAGE_TAG                                 24
#define MSG_INODE_TAG                                   25
#define MSG_CONFIDENCE_TAG                              26
#define MSG_ACTION_TAG                                  27
#define MSG_IMPACT_TAG                                  28
#define MSG_ASSESSMENT_TAG                              29


/*
 * Other
 */
#define MSG_OWN_FORMAT                                 253

#define MSG_END_OF_TAG                                 254



/*
 * Possible tag behind MSG_OWN_FORMAT
 */
#define MSG_FORMAT_PRELUDE_NIDS 1


/*
 * IDMEF confidence
 */
#define MSG_CONFIDENCE_RATING 0
#define MSG_CONFIDENCE_CONFIDENCE 1

/*
 * IDMEF action
 */
#define MSG_ACTION_CATEGORY 0
#define MSG_ACTION_DESCRIPTION 1

/*
 * IDMEF impact
 */
#define MSG_IMPACT_SEVERITY 0
#define MSG_IMPACT_COMPLETION 1
#define MSG_IMPACT_TYPE 2
#define MSG_IMPACT_DESCRIPTION 3

/*
 * IDMEF File
 */
#define MSG_FILE_IDENT 0
#define MSG_FILE_CATEGORY 1
#define MSG_FILE_FSTYPE 2
#define MSG_FILE_NAME 3
#define MSG_FILE_PATH 4
#define MSG_FILE_CREATE_TIME_TAG 5
#define MSG_FILE_MODIFY_TIME_TAG 6
#define MSG_FILE_ACCESS_TIME_TAG 7
#define MSG_FILE_DATASIZE 8
#define MSG_FILE_DISKSIZE 9

/*
 * IDMEF Linkage
 */
#define MSG_LINKAGE_CATEGORY 0
#define MSG_LINKAGE_NAME 1
#define MSG_LINKAGE_PATH 2
#define MSG_LINKAGE_FILE 3

/*
 * IDMEF Inode
 */
#define MSG_INODE_CHANGE_TIME 0
#define MSG_INODE_NUMBER 1
#define MSG_INODE_MAJOR_DEVICE 2
#define MSG_INODE_MINOR_DEVICE 3
#define MSG_INODE_C_MAJOR_DEVICE 4
#define MSG_INODE_C_MINOR_DEVICE 5

/*
 * IDMEF access
 */
#define MSG_ACCESS_PERMISSION 0

/*
 * IDMEF alert
 */
#define MSG_ALERT_IDENT  0


/*
 * IDMEF source / target
 */
#define MSG_SOURCE_IDENT     0
#define MSG_SOURCE_SPOOFED   1
#define MSG_SOURCE_INTERFACE 2


/*
 * IDMEF node
 */
#define MSG_NODE_IDENT       0
#define MSG_NODE_CATEGORY    1
#define MSG_NODE_LOCATION    2
#define MSG_NODE_NAME        3


/*
 * IDMEF address
 */
#define MSG_ADDRESS_IDENT     0
#define MSG_ADDRESS_CATEGORY  1
#define MSG_ADDRESS_VLAN_NAME 2
#define MSG_ADDRESS_VLAN_NUM  3
#define MSG_ADDRESS_ADDRESS   4
#define MSG_ADDRESS_NETMASK   5



/*
 * IDMEF user
 */
#define MSG_USER_IDENT    0
#define MSG_USER_CATEGORY 1


/*
 * IDMEF user Id
 */
#define MSG_USERID_IDENT  0
#define MSG_USERID_TYPE   1
#define MSG_USERID_NAME   2
#define MSG_USERID_NUMBER 3

/*
 * IDMEF process
 */
#define MSG_PROCESS_IDENT 0
#define MSG_PROCESS_NAME  1
#define MSG_PROCESS_PID   2
#define MSG_PROCESS_PATH  3
#define MSG_PROCESS_ARG   4
#define MSG_PROCESS_ENV   5


/*
 * IDMEF service
 */
#define MSG_SERVICE_IDENT     0
#define MSG_SERVICE_NAME      1
#define MSG_SERVICE_PORT      2
#define MSG_SERVICE_PORTLIST  3
#define MSG_SERVICE_PROTOCOL  4


/*
 * IDMEF web service
 */
#define MSG_WEBSERVICE_URL         0
#define MSG_WEBSERVICE_CGI         1
#define MSG_WEBSERVICE_HTTP_METHOD 2
#define MSG_WEBSERVICE_ARG         3

/*
 * IDMEF snmp service
 */
#define MSG_SNMPSERVICE_OID       0
#define MSG_SNMPSERVICE_COMMUNITY 1
#define MSG_SNMPSERVICE_COMMAND   2


/*
 * IDMEF classification
 */
#define MSG_CLASSIFICATION_ORIGIN 0
#define MSG_CLASSIFICATION_NAME   1
#define MSG_CLASSIFICATION_URL    2

/*
 * IDMEF additional data
 */
#define MSG_ADDITIONALDATA_TYPE    0
#define MSG_ADDITIONALDATA_MEANING 1
#define MSG_ADDITIONALDATA_DATA    2


/*
 * IDMEF analyzer
 */
#define MSG_ANALYZER_ID            0
#define MSG_ANALYZER_MANUFACTURER  1
#define MSG_ANALYZER_MODEL         2
#define MSG_ANALYZER_VERSION       3
#define MSG_ANALYZER_CLASS         4
/*
 * skip ID 5 used by node inside analyzed
 */
#define MSG_ANALYZER_OSTYPE        6
#define MSG_ANALYZER_OSVERSION     7



/*
 * IDMEF target
 */
#define MSG_TARGET_IDENT           0
#define MSG_TARGET_DECOY           1
#define MSG_TARGET_INTERFACE       2

/*
 * IDMEF heartbeat
 */
#define MSG_HEARTBEAT_IDENT        0

/*
 * IDMEF tool alert
 */
#define MSG_TOOL_ALERT_NAME        0
#define MSG_TOOL_ALERT_COMMAND     1
#define MSG_TOOL_ALERT_ANALYZER_ID 2


/*
 * IDMEF correlation alert
 */
#define MSG_CORRELATION_ALERT_NAME  0 
#define MSG_CORRELATION_ALERT_IDENT 1


/*
 * IDMEF Overflow alert
 */
#define MSG_OVERFLOW_ALERT_PROGRAM 0
#define MSG_OVERFLOW_ALERT_SIZE    1
#define MSG_OVERFLOW_ALERT_BUFFER  2

/*
 * Creattime / Detect Time / Analyzer time
 */
#define MSG_TIME_SEC  0
#define MSG_TIME_USEC 1

/*
 * Carry an IDENT + an analyzer
 */
#define MSG_ALERTIDENT_IDENT           0
#define MSG_ALERTIDENT_ANALYZER_IDENT  1

#endif /* _LIBPRELUDE_IDMEF_MESSAGE_ID_H */




