/*****
*
* Copyright (C) 2001 Yoann Vandoorselaere <yoann@mandrakesoft.com>
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
 * IDMEF message work...
 */
#define MSG_END_OF_TAG 254

/*
 * IDMEF alert
 */
#define MSG_ALERT_TAG 0
#define MSG_ALERT_IDENT 1
#define MSG_ALERT_ACTION 2
#define MSG_ALERT_IMPACT 3


/*
 * Create time 
 */
#define MSG_CREATE_TIME_TAG 4
#define MSG_DETECT_TIME_TAG 5
#define MSG_ANALYZER_TIME_TAG 6

/*
 * IDMEF source / target
 */
#define MSG_SOURCE_TAG 7
#define MSG_SOURCE_IDENT 8
#define MSG_SOURCE_SPOOFED 9
#define MSG_SOURCE_INTERFACE 10


/*
 * IDMEF node
 */
#define MSG_NODE_TAG 11
#define MSG_NODE_IDENT 12
#define MSG_NODE_CATEGORY 13
#define MSG_NODE_LOCATION 14
#define MSG_NODE_NAME 15


/*
 * IDMEF address
 */
#define MSG_ADDRESS_TAG 16
#define MSG_ADDRESS_IDENT 17
#define MSG_ADDRESS_CATEGORY 18
#define MSG_ADDRESS_VLAN_NAME 19
#define MSG_ADDRESS_VLAN_NUM 20
#define MSG_ADDRESS_ADDRESS 21
#define MSG_ADDRESS_NETMASK 22



/*
 * IDMEF user
 */
#define MSG_USER_TAG 23
#define MSG_USER_IDENT 24
#define MSG_USER_CATEGORY 25


/*
 * IDMEF user Id
 */
#define MSG_USERID_TAG 26
#define MSG_USERID_IDENT 27
#define MSG_USERID_TYPE 28
#define MSG_USERID_NAME 29
#define MSG_USERID_NUMBER 30

/*
 * IDMEF process
 */
#define MSG_PROCESS_TAG 31
#define MSG_PROCESS_IDENT 32
#define MSG_PROCESS_NAME 33
#define MSG_PROCESS_PID 34
#define MSG_PROCESS_PATH 35
#define MSG_PROCESS_ARG 36
#define MSG_PROCESS_ENV 37


/*
 * IDMEF service
 */
#define MSG_SERVICE_TAG 38
#define MSG_SERVICE_IDENT 39
#define MSG_SERVICE_NAME 40
#define MSG_SERVICE_PORT 41
#define MSG_SERVICE_PORTLIST 42
#define MSG_SERVICE_PROTOCOL 43


/*
 * IDMEF web service
 */
#define MSG_WEBSERVICE_TAG 44
#define MSG_WEBSERVICE_URL 45
#define MSG_WEBSERVICE_CGI 46
#define MSG_WEBSERVICE_METHOD 47
#define MSG_WEBSERVICE_ARG 48

/*
 * IDMEF snmp service
 */
#define MSG_SNMPSERVICE_TAG 49
#define MSG_SNMPSERVICE_OID 50
#define MSG_SNMPSERVICE_COMMUNITY 51
#define MSG_SNMPSERVICE_COMMAND 52


/*
 * IDMEF classification
 */
#define MSG_CLASSIFICATION_TAG 53
#define MSG_CLASSIFICATION_ORIGIN 54
#define MSG_CLASSIFICATION_NAME 55
#define MSG_CLASSIFICATION_URL 56

/*
 * IDMEF additional data
 */
#define MSG_ADDITIONALDATA_TAG 57
#define MSG_ADDITIONALDATA_TYPE 58
#define MSG_ADDITIONALDATA_MEANING 59
#define MSG_ADDITIONALDATA_DATA 60


/*
 * IDMEF analyzer
 */
#define MSG_ANALYZER_TAG 61
#define MSG_ANALYZER_ID  62
#define MSG_ANALYZER_MANUFACTURER 63
#define MSG_ANALYZER_MODEL 64
#define MSG_ANALYZER_VERSION 65
#define MSG_ANALYZER_CLASS 66


/*
 * IDMEF target
 */
#define MSG_TARGET_TAG 67
#define MSG_TARGET_IDENT 68
#define MSG_TARGET_DECOY 69
#define MSG_TARGET_INTERFACE 70

/*
 * IDMEF heartbeat
 */
#define MSG_HEARTBEAT_TAG 71
#define MSG_HEARTBEAT_IDENT 72

/*
 * IDMEF tool alert
 */
#define MSG_TOOL_ALERT_TAG 73
#define MSG_TOOL_ALERT_NAME 74
#define MSG_TOOL_ALERT_COMMAND 75
#define MSG_TOOL_ALERT_ANALYZER_ID 76


/*
 * IDMEF correlation alert
 */
#define MSG_CORRELATION_ALERT_TAG 76
#define MSG_CORRELATION_ALERT_NAME 77 
#define MSG_CORRELATION_ALERT_IDENT 78


/*
 * IDMEF Overflow alert
 */
#define MSG_OVERFLOW_ALERT_TAG 79
#define MSG_OVERFLOW_ALERT_PROGRAM 80
#define MSG_OVERFLOW_ALERT_SIZE 81
#define MSG_OVERFLOW_ALERT_BUFFER 82

/*
 * Creattime / Detect Time / Analyzer time
 */
#define MSG_TIME_SEC 83
#define MSG_TIME_USEC 84

/*
 * Carry an IDENT + an analyzer
 */
#define MSG_ALERTIDENT_TAG 85
#define MSG_ALERTIDENT_IDENT 86
#define MSG_ALERTIDENT_ANALYZER_IDENT 87

/*
 * Other
 */
#define MSG_OWN_FORMAT 88

/*
 * Possible tag behind MSG_OWN_FORMAT
 */
#define MSG_FORMAT_PRELUDE_NIDS 89


