
/* Auto-generated by the GenerateIDMEFTreeData package */

typedef struct {
        const char *name;
        prelude_bool_t list;
        prelude_bool_t keyed_list;
        idmef_value_type_id_t type;
        idmef_class_id_t class;
        int union_id;
} children_list_t;

const children_list_t idmef_additional_data_children_list[] = {
        { "meaning", 0, 0, IDMEF_VALUE_TYPE_STRING, 0, 0 },
        { "type", 0, 0, IDMEF_VALUE_TYPE_ENUM, IDMEF_CLASS_ID_ADDITIONAL_DATA_TYPE, 0 },
        { "data", 0, 0, IDMEF_VALUE_TYPE_DATA, 0, 0 },
};

const children_list_t idmef_reference_children_list[] = {
        { "origin", 0, 0, IDMEF_VALUE_TYPE_ENUM, IDMEF_CLASS_ID_REFERENCE_ORIGIN, 0 },
        { "name", 0, 0, IDMEF_VALUE_TYPE_STRING, 0, 0 },
        { "url", 0, 0, IDMEF_VALUE_TYPE_STRING, 0, 0 },
        { "meaning", 0, 0, IDMEF_VALUE_TYPE_STRING, 0, 0 },
};

const children_list_t idmef_classification_children_list[] = {
        { "ident", 0, 0, IDMEF_VALUE_TYPE_STRING, 0, 0 },
        { "text", 0, 0, IDMEF_VALUE_TYPE_STRING, 0, 0 },
        { "reference", 1, 0, IDMEF_VALUE_TYPE_CLASS, IDMEF_CLASS_ID_REFERENCE, 0 },
};

const children_list_t idmef_user_id_children_list[] = {
        { "ident", 0, 0, IDMEF_VALUE_TYPE_STRING, 0, 0 },
        { "type", 0, 0, IDMEF_VALUE_TYPE_ENUM, IDMEF_CLASS_ID_USER_ID_TYPE, 0 },
        { "tty", 0, 0, IDMEF_VALUE_TYPE_STRING, 0, 0 },
        { "name", 0, 0, IDMEF_VALUE_TYPE_STRING, 0, 0 },
        { "number", 0, 0, IDMEF_VALUE_TYPE_UINT32, 0, 0 },
};

const children_list_t idmef_user_children_list[] = {
        { "ident", 0, 0, IDMEF_VALUE_TYPE_STRING, 0, 0 },
        { "category", 0, 0, IDMEF_VALUE_TYPE_ENUM, IDMEF_CLASS_ID_USER_CATEGORY, 0 },
        { "user_id", 1, 0, IDMEF_VALUE_TYPE_CLASS, IDMEF_CLASS_ID_USER_ID, 0 },
};

const children_list_t idmef_address_children_list[] = {
        { "ident", 0, 0, IDMEF_VALUE_TYPE_STRING, 0, 0 },
        { "category", 0, 0, IDMEF_VALUE_TYPE_ENUM, IDMEF_CLASS_ID_ADDRESS_CATEGORY, 0 },
        { "vlan_name", 0, 0, IDMEF_VALUE_TYPE_STRING, 0, 0 },
        { "vlan_num", 0, 0, IDMEF_VALUE_TYPE_INT32, 0, 0 },
        { "address", 0, 0, IDMEF_VALUE_TYPE_STRING, 0, 0 },
        { "netmask", 0, 0, IDMEF_VALUE_TYPE_STRING, 0, 0 },
};

const children_list_t idmef_process_children_list[] = {
        { "ident", 0, 0, IDMEF_VALUE_TYPE_STRING, 0, 0 },
        { "name", 0, 0, IDMEF_VALUE_TYPE_STRING, 0, 0 },
        { "pid", 0, 0, IDMEF_VALUE_TYPE_UINT32, 0, 0 },
        { "path", 0, 0, IDMEF_VALUE_TYPE_STRING, 0, 0 },
        { "arg", 1, 0, IDMEF_VALUE_TYPE_STRING, 0, 0 },
        { "env", 1, 0, IDMEF_VALUE_TYPE_STRING, 0, 0 },
};

const children_list_t idmef_web_service_children_list[] = {
        { "url", 0, 0, IDMEF_VALUE_TYPE_STRING, 0, 0 },
        { "cgi", 0, 0, IDMEF_VALUE_TYPE_STRING, 0, 0 },
        { "http_method", 0, 0, IDMEF_VALUE_TYPE_STRING, 0, 0 },
        { "arg", 1, 0, IDMEF_VALUE_TYPE_STRING, 0, 0 },
};

const children_list_t idmef_snmp_service_children_list[] = {
        { "oid", 0, 0, IDMEF_VALUE_TYPE_STRING, 0, 0 },
        { "message_processing_model", 0, 0, IDMEF_VALUE_TYPE_UINT32, 0, 0 },
        { "security_model", 0, 0, IDMEF_VALUE_TYPE_UINT32, 0, 0 },
        { "security_name", 0, 0, IDMEF_VALUE_TYPE_STRING, 0, 0 },
        { "security_level", 0, 0, IDMEF_VALUE_TYPE_UINT32, 0, 0 },
        { "context_name", 0, 0, IDMEF_VALUE_TYPE_STRING, 0, 0 },
        { "context_engine_id", 0, 0, IDMEF_VALUE_TYPE_STRING, 0, 0 },
        { "command", 0, 0, IDMEF_VALUE_TYPE_STRING, 0, 0 },
};

const children_list_t idmef_service_children_list[] = {
        { "ident", 0, 0, IDMEF_VALUE_TYPE_STRING, 0, 0 },
        { "ip_version", 0, 0, IDMEF_VALUE_TYPE_UINT8, 0, 0 },
        { "iana_protocol_number", 0, 0, IDMEF_VALUE_TYPE_UINT8, 0, 0 },
        { "iana_protocol_name", 0, 0, IDMEF_VALUE_TYPE_STRING, 0, 0 },
        { "name", 0, 0, IDMEF_VALUE_TYPE_STRING, 0, 0 },
        { "port", 0, 0, IDMEF_VALUE_TYPE_UINT16, 0, 0 },
        { "portlist", 0, 0, IDMEF_VALUE_TYPE_STRING, 0, 0 },
        { "protocol", 0, 0, IDMEF_VALUE_TYPE_STRING, 0, 0 },
        { "web_service", 0, 0, IDMEF_VALUE_TYPE_CLASS, IDMEF_CLASS_ID_WEB_SERVICE, /* union ID */ 1 },
        { "snmp_service", 0, 0, IDMEF_VALUE_TYPE_CLASS, IDMEF_CLASS_ID_SNMP_SERVICE, /* union ID */ 1 },
};

const children_list_t idmef_node_children_list[] = {
        { "ident", 0, 0, IDMEF_VALUE_TYPE_STRING, 0, 0 },
        { "category", 0, 0, IDMEF_VALUE_TYPE_ENUM, IDMEF_CLASS_ID_NODE_CATEGORY, 0 },
        { "location", 0, 0, IDMEF_VALUE_TYPE_STRING, 0, 0 },
        { "name", 0, 0, IDMEF_VALUE_TYPE_STRING, 0, 0 },
        { "address", 1, 0, IDMEF_VALUE_TYPE_CLASS, IDMEF_CLASS_ID_ADDRESS, 0 },
};

const children_list_t idmef_source_children_list[] = {
        { "ident", 0, 0, IDMEF_VALUE_TYPE_STRING, 0, 0 },
        { "spoofed", 0, 0, IDMEF_VALUE_TYPE_ENUM, IDMEF_CLASS_ID_SOURCE_SPOOFED, 0 },
        { "interface", 0, 0, IDMEF_VALUE_TYPE_STRING, 0, 0 },
        { "node", 0, 0, IDMEF_VALUE_TYPE_CLASS, IDMEF_CLASS_ID_NODE, 0 },
        { "user", 0, 0, IDMEF_VALUE_TYPE_CLASS, IDMEF_CLASS_ID_USER, 0 },
        { "process", 0, 0, IDMEF_VALUE_TYPE_CLASS, IDMEF_CLASS_ID_PROCESS, 0 },
        { "service", 0, 0, IDMEF_VALUE_TYPE_CLASS, IDMEF_CLASS_ID_SERVICE, 0 },
};

const children_list_t idmef_file_access_children_list[] = {
        { "user_id", 0, 0, IDMEF_VALUE_TYPE_CLASS, IDMEF_CLASS_ID_USER_ID, 0 },
        { "permission", 1, 0, IDMEF_VALUE_TYPE_STRING, 0, 0 },
};

const children_list_t idmef_inode_children_list[] = {
        { "change_time", 0, 0, IDMEF_VALUE_TYPE_TIME, 0, 0 },
        { "number", 0, 0, IDMEF_VALUE_TYPE_UINT32, 0, 0 },
        { "major_device", 0, 0, IDMEF_VALUE_TYPE_UINT32, 0, 0 },
        { "minor_device", 0, 0, IDMEF_VALUE_TYPE_UINT32, 0, 0 },
        { "c_major_device", 0, 0, IDMEF_VALUE_TYPE_UINT32, 0, 0 },
        { "c_minor_device", 0, 0, IDMEF_VALUE_TYPE_UINT32, 0, 0 },
};

const children_list_t idmef_checksum_children_list[] = {
        { "value", 0, 0, IDMEF_VALUE_TYPE_STRING, 0, 0 },
        { "key", 0, 0, IDMEF_VALUE_TYPE_STRING, 0, 0 },
        { "algorithm", 0, 0, IDMEF_VALUE_TYPE_ENUM, IDMEF_CLASS_ID_CHECKSUM_ALGORITHM, 0 },
};

const children_list_t idmef_file_children_list[] = {
        { "ident", 0, 0, IDMEF_VALUE_TYPE_STRING, 0, 0 },
        { "name", 0, 0, IDMEF_VALUE_TYPE_STRING, 0, 0 },
        { "path", 0, 0, IDMEF_VALUE_TYPE_STRING, 0, 0 },
        { "create_time", 0, 0, IDMEF_VALUE_TYPE_TIME, 0, 0 },
        { "modify_time", 0, 0, IDMEF_VALUE_TYPE_TIME, 0, 0 },
        { "access_time", 0, 0, IDMEF_VALUE_TYPE_TIME, 0, 0 },
        { "data_size", 0, 0, IDMEF_VALUE_TYPE_UINT64, 0, 0 },
        { "disk_size", 0, 0, IDMEF_VALUE_TYPE_UINT64, 0, 0 },
        { "file_access", 1, 0, IDMEF_VALUE_TYPE_CLASS, IDMEF_CLASS_ID_FILE_ACCESS, 0 },
        { "linkage", 1, 0, IDMEF_VALUE_TYPE_CLASS, IDMEF_CLASS_ID_LINKAGE, 0 },
        { "inode", 0, 0, IDMEF_VALUE_TYPE_CLASS, IDMEF_CLASS_ID_INODE, 0 },
        { "checksum", 1, 0, IDMEF_VALUE_TYPE_CLASS, IDMEF_CLASS_ID_CHECKSUM, 0 },
        { "category", 0, 0, IDMEF_VALUE_TYPE_ENUM, IDMEF_CLASS_ID_FILE_CATEGORY, 0 },
        { "fstype", 0, 0, IDMEF_VALUE_TYPE_ENUM, IDMEF_CLASS_ID_FILE_FSTYPE, 0 },
        { "file_type", 0, 0, IDMEF_VALUE_TYPE_STRING, 0, 0 },
};

const children_list_t idmef_linkage_children_list[] = {
        { "category", 0, 0, IDMEF_VALUE_TYPE_ENUM, IDMEF_CLASS_ID_LINKAGE_CATEGORY, 0 },
        { "name", 0, 0, IDMEF_VALUE_TYPE_STRING, 0, 0 },
        { "path", 0, 0, IDMEF_VALUE_TYPE_STRING, 0, 0 },
        { "file", 0, 0, IDMEF_VALUE_TYPE_CLASS, IDMEF_CLASS_ID_FILE, 0 },
};

const children_list_t idmef_target_children_list[] = {
        { "ident", 0, 0, IDMEF_VALUE_TYPE_STRING, 0, 0 },
        { "decoy", 0, 0, IDMEF_VALUE_TYPE_ENUM, IDMEF_CLASS_ID_TARGET_DECOY, 0 },
        { "interface", 0, 0, IDMEF_VALUE_TYPE_STRING, 0, 0 },
        { "node", 0, 0, IDMEF_VALUE_TYPE_CLASS, IDMEF_CLASS_ID_NODE, 0 },
        { "user", 0, 0, IDMEF_VALUE_TYPE_CLASS, IDMEF_CLASS_ID_USER, 0 },
        { "process", 0, 0, IDMEF_VALUE_TYPE_CLASS, IDMEF_CLASS_ID_PROCESS, 0 },
        { "service", 0, 0, IDMEF_VALUE_TYPE_CLASS, IDMEF_CLASS_ID_SERVICE, 0 },
        { "file", 1, 0, IDMEF_VALUE_TYPE_CLASS, IDMEF_CLASS_ID_FILE, 0 },
};

const children_list_t idmef_analyzer_children_list[] = {
        { "analyzerid", 0, 0, IDMEF_VALUE_TYPE_STRING, 0, 0 },
        { "name", 0, 0, IDMEF_VALUE_TYPE_STRING, 0, 0 },
        { "manufacturer", 0, 0, IDMEF_VALUE_TYPE_STRING, 0, 0 },
        { "model", 0, 0, IDMEF_VALUE_TYPE_STRING, 0, 0 },
        { "version", 0, 0, IDMEF_VALUE_TYPE_STRING, 0, 0 },
        { "class", 0, 0, IDMEF_VALUE_TYPE_STRING, 0, 0 },
        { "ostype", 0, 0, IDMEF_VALUE_TYPE_STRING, 0, 0 },
        { "osversion", 0, 0, IDMEF_VALUE_TYPE_STRING, 0, 0 },
        { "node", 0, 0, IDMEF_VALUE_TYPE_CLASS, IDMEF_CLASS_ID_NODE, 0 },
        { "process", 0, 0, IDMEF_VALUE_TYPE_CLASS, IDMEF_CLASS_ID_PROCESS, 0 },
};

const children_list_t idmef_alertident_children_list[] = {
        { "alertident", 0, 0, IDMEF_VALUE_TYPE_STRING, 0, 0 },
        { "analyzerid", 0, 0, IDMEF_VALUE_TYPE_STRING, 0, 0 },
};

const children_list_t idmef_impact_children_list[] = {
        { "severity", 0, 0, IDMEF_VALUE_TYPE_ENUM, IDMEF_CLASS_ID_IMPACT_SEVERITY, 0 },
        { "completion", 0, 0, IDMEF_VALUE_TYPE_ENUM, IDMEF_CLASS_ID_IMPACT_COMPLETION, 0 },
        { "type", 0, 0, IDMEF_VALUE_TYPE_ENUM, IDMEF_CLASS_ID_IMPACT_TYPE, 0 },
        { "description", 0, 0, IDMEF_VALUE_TYPE_STRING, 0, 0 },
};

const children_list_t idmef_action_children_list[] = {
        { "category", 0, 0, IDMEF_VALUE_TYPE_ENUM, IDMEF_CLASS_ID_ACTION_CATEGORY, 0 },
        { "description", 0, 0, IDMEF_VALUE_TYPE_STRING, 0, 0 },
};

const children_list_t idmef_confidence_children_list[] = {
        { "rating", 0, 0, IDMEF_VALUE_TYPE_ENUM, IDMEF_CLASS_ID_CONFIDENCE_RATING, 0 },
        { "confidence", 0, 0, IDMEF_VALUE_TYPE_FLOAT, 0, 0 },
};

const children_list_t idmef_assessment_children_list[] = {
        { "impact", 0, 0, IDMEF_VALUE_TYPE_CLASS, IDMEF_CLASS_ID_IMPACT, 0 },
        { "action", 1, 0, IDMEF_VALUE_TYPE_CLASS, IDMEF_CLASS_ID_ACTION, 0 },
        { "confidence", 0, 0, IDMEF_VALUE_TYPE_CLASS, IDMEF_CLASS_ID_CONFIDENCE, 0 },
};

const children_list_t idmef_tool_alert_children_list[] = {
        { "name", 0, 0, IDMEF_VALUE_TYPE_STRING, 0, 0 },
        { "command", 0, 0, IDMEF_VALUE_TYPE_STRING, 0, 0 },
        { "alertident", 1, 0, IDMEF_VALUE_TYPE_CLASS, IDMEF_CLASS_ID_ALERTIDENT, 0 },
};

const children_list_t idmef_correlation_alert_children_list[] = {
        { "name", 0, 0, IDMEF_VALUE_TYPE_STRING, 0, 0 },
        { "alertident", 1, 0, IDMEF_VALUE_TYPE_CLASS, IDMEF_CLASS_ID_ALERTIDENT, 0 },
};

const children_list_t idmef_overflow_alert_children_list[] = {
        { "program", 0, 0, IDMEF_VALUE_TYPE_STRING, 0, 0 },
        { "size", 0, 0, IDMEF_VALUE_TYPE_UINT32, 0, 0 },
        { "buffer", 0, 0, IDMEF_VALUE_TYPE_DATA, 0, 0 },
};

const children_list_t idmef_alert_children_list[] = {
        { "messageid", 0, 0, IDMEF_VALUE_TYPE_STRING, 0, 0 },
        { "analyzer", 1, 0, IDMEF_VALUE_TYPE_CLASS, IDMEF_CLASS_ID_ANALYZER, 0 },
        { "create_time", 0, 0, IDMEF_VALUE_TYPE_TIME, 0, 0 },
        { "classification", 0, 0, IDMEF_VALUE_TYPE_CLASS, IDMEF_CLASS_ID_CLASSIFICATION, 0 },
        { "detect_time", 0, 0, IDMEF_VALUE_TYPE_TIME, 0, 0 },
        { "analyzer_time", 0, 0, IDMEF_VALUE_TYPE_TIME, 0, 0 },
        { "source", 1, 0, IDMEF_VALUE_TYPE_CLASS, IDMEF_CLASS_ID_SOURCE, 0 },
        { "target", 1, 0, IDMEF_VALUE_TYPE_CLASS, IDMEF_CLASS_ID_TARGET, 0 },
        { "assessment", 0, 0, IDMEF_VALUE_TYPE_CLASS, IDMEF_CLASS_ID_ASSESSMENT, 0 },
        { "additional_data", 1, 1, IDMEF_VALUE_TYPE_CLASS, IDMEF_CLASS_ID_ADDITIONAL_DATA, 0 },
        { "tool_alert", 0, 0, IDMEF_VALUE_TYPE_CLASS, IDMEF_CLASS_ID_TOOL_ALERT, /* union ID */ 1 },
        { "correlation_alert", 0, 0, IDMEF_VALUE_TYPE_CLASS, IDMEF_CLASS_ID_CORRELATION_ALERT, /* union ID */ 1 },
        { "overflow_alert", 0, 0, IDMEF_VALUE_TYPE_CLASS, IDMEF_CLASS_ID_OVERFLOW_ALERT, /* union ID */ 1 },
};

const children_list_t idmef_heartbeat_children_list[] = {
        { "messageid", 0, 0, IDMEF_VALUE_TYPE_STRING, 0, 0 },
        { "analyzer", 1, 0, IDMEF_VALUE_TYPE_CLASS, IDMEF_CLASS_ID_ANALYZER, 0 },
        { "create_time", 0, 0, IDMEF_VALUE_TYPE_TIME, 0, 0 },
        { "analyzer_time", 0, 0, IDMEF_VALUE_TYPE_TIME, 0, 0 },
        { "heartbeat_interval", 0, 0, IDMEF_VALUE_TYPE_UINT32, 0, 0 },
        { "additional_data", 1, 1, IDMEF_VALUE_TYPE_CLASS, IDMEF_CLASS_ID_ADDITIONAL_DATA, 0 },
};

const children_list_t idmef_message_children_list[] = {
        { "version", 0, 0, IDMEF_VALUE_TYPE_STRING, 0, 0 },
        { "alert", 0, 0, IDMEF_VALUE_TYPE_CLASS, IDMEF_CLASS_ID_ALERT, /* union ID */ 1 },
        { "heartbeat", 0, 0, IDMEF_VALUE_TYPE_CLASS, IDMEF_CLASS_ID_HEARTBEAT, /* union ID */ 1 },
};


typedef struct {
        const char *name;
        size_t children_list_elem;
        const children_list_t *children_list;
        int (*get_child)(void *ptr, idmef_class_child_id_t child, void **ret);
        int (*new_child)(void *ptr, idmef_class_child_id_t child, int n, void **ret);
        int (*destroy_child)(void *ptr, idmef_class_child_id_t child, int n);
        int (*to_numeric)(const char *name);
        const char *(*to_string)(int val);
        int (*copy)(const void *src, void *dst);
        int (*clone)(const void *src, void **dst);
        int (*compare)(const void *obj1, const void *obj2);
        int (*print)(const void *obj, prelude_io_t *fd);
        void *(*ref)(void *src);
        void (*destroy)(void *obj);
        prelude_bool_t is_listed;
} object_data_t;


const object_data_t object_data[] = {
        { "(unassigned)", 0, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL }, /* ID: 0 */
        { "(unassigned)", 0, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL }, /* ID: 1 */
        { "(unassigned)", 0, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL }, /* ID: 2 */
        { "additional_data_type", 0, NULL, NULL, NULL, NULL, (void *) idmef_additional_data_type_to_numeric, (void *) idmef_additional_data_type_to_string, NULL, NULL, NULL, NULL, NULL, 0 }, /* ID: 3 */
        { "additional_data", sizeof(idmef_additional_data_children_list) / sizeof(*idmef_additional_data_children_list), idmef_additional_data_children_list, _idmef_additional_data_get_child, _idmef_additional_data_new_child, _idmef_additional_data_destroy_child, NULL, NULL, (void *) idmef_additional_data_copy, (void *) idmef_additional_data_clone, (void *) idmef_additional_data_compare, (void *) idmef_additional_data_print, (void *) idmef_additional_data_ref, (void *) idmef_additional_data_destroy, 1 },/* ID: 4 */
        { "reference_origin", 0, NULL, NULL, NULL, NULL, (void *) idmef_reference_origin_to_numeric, (void *) idmef_reference_origin_to_string, NULL, NULL, NULL, NULL, NULL, 0 }, /* ID: 5 */
        { "classification", sizeof(idmef_classification_children_list) / sizeof(*idmef_classification_children_list), idmef_classification_children_list, _idmef_classification_get_child, _idmef_classification_new_child, _idmef_classification_destroy_child, NULL, NULL, (void *) idmef_classification_copy, (void *) idmef_classification_clone, (void *) idmef_classification_compare, (void *) idmef_classification_print, (void *) idmef_classification_ref, (void *) idmef_classification_destroy, 0 },/* ID: 6 */
        { "user_id_type", 0, NULL, NULL, NULL, NULL, (void *) idmef_user_id_type_to_numeric, (void *) idmef_user_id_type_to_string, NULL, NULL, NULL, NULL, NULL, 0 }, /* ID: 7 */
        { "user_id", sizeof(idmef_user_id_children_list) / sizeof(*idmef_user_id_children_list), idmef_user_id_children_list, _idmef_user_id_get_child, _idmef_user_id_new_child, _idmef_user_id_destroy_child, NULL, NULL, (void *) idmef_user_id_copy, (void *) idmef_user_id_clone, (void *) idmef_user_id_compare, (void *) idmef_user_id_print, (void *) idmef_user_id_ref, (void *) idmef_user_id_destroy, 1 },/* ID: 8 */
        { "user_category", 0, NULL, NULL, NULL, NULL, (void *) idmef_user_category_to_numeric, (void *) idmef_user_category_to_string, NULL, NULL, NULL, NULL, NULL, 0 }, /* ID: 9 */
        { "user", sizeof(idmef_user_children_list) / sizeof(*idmef_user_children_list), idmef_user_children_list, _idmef_user_get_child, _idmef_user_new_child, _idmef_user_destroy_child, NULL, NULL, (void *) idmef_user_copy, (void *) idmef_user_clone, (void *) idmef_user_compare, (void *) idmef_user_print, (void *) idmef_user_ref, (void *) idmef_user_destroy, 0 },/* ID: 10 */
        { "address_category", 0, NULL, NULL, NULL, NULL, (void *) idmef_address_category_to_numeric, (void *) idmef_address_category_to_string, NULL, NULL, NULL, NULL, NULL, 0 }, /* ID: 11 */
        { "address", sizeof(idmef_address_children_list) / sizeof(*idmef_address_children_list), idmef_address_children_list, _idmef_address_get_child, _idmef_address_new_child, _idmef_address_destroy_child, NULL, NULL, (void *) idmef_address_copy, (void *) idmef_address_clone, (void *) idmef_address_compare, (void *) idmef_address_print, (void *) idmef_address_ref, (void *) idmef_address_destroy, 1 },/* ID: 12 */
        { "process", sizeof(idmef_process_children_list) / sizeof(*idmef_process_children_list), idmef_process_children_list, _idmef_process_get_child, _idmef_process_new_child, _idmef_process_destroy_child, NULL, NULL, (void *) idmef_process_copy, (void *) idmef_process_clone, (void *) idmef_process_compare, (void *) idmef_process_print, (void *) idmef_process_ref, (void *) idmef_process_destroy, 0 },/* ID: 13 */
        { "web_service", sizeof(idmef_web_service_children_list) / sizeof(*idmef_web_service_children_list), idmef_web_service_children_list, _idmef_web_service_get_child, _idmef_web_service_new_child, _idmef_web_service_destroy_child, NULL, NULL, (void *) idmef_web_service_copy, (void *) idmef_web_service_clone, (void *) idmef_web_service_compare, (void *) idmef_web_service_print, (void *) idmef_web_service_ref, (void *) idmef_web_service_destroy, 0 },/* ID: 14 */
        { "snmp_service", sizeof(idmef_snmp_service_children_list) / sizeof(*idmef_snmp_service_children_list), idmef_snmp_service_children_list, _idmef_snmp_service_get_child, _idmef_snmp_service_new_child, _idmef_snmp_service_destroy_child, NULL, NULL, (void *) idmef_snmp_service_copy, (void *) idmef_snmp_service_clone, (void *) idmef_snmp_service_compare, (void *) idmef_snmp_service_print, (void *) idmef_snmp_service_ref, (void *) idmef_snmp_service_destroy, 0 },/* ID: 15 */
        { "service_type", 0, NULL, NULL, NULL, NULL, (void *) idmef_service_type_to_numeric, (void *) idmef_service_type_to_string, NULL, NULL, NULL, NULL, NULL, 0 }, /* ID: 16 */
        { "service", sizeof(idmef_service_children_list) / sizeof(*idmef_service_children_list), idmef_service_children_list, _idmef_service_get_child, _idmef_service_new_child, _idmef_service_destroy_child, NULL, NULL, (void *) idmef_service_copy, (void *) idmef_service_clone, (void *) idmef_service_compare, (void *) idmef_service_print, (void *) idmef_service_ref, (void *) idmef_service_destroy, 0 },/* ID: 17 */
        { "node_category", 0, NULL, NULL, NULL, NULL, (void *) idmef_node_category_to_numeric, (void *) idmef_node_category_to_string, NULL, NULL, NULL, NULL, NULL, 0 }, /* ID: 18 */
        { "node", sizeof(idmef_node_children_list) / sizeof(*idmef_node_children_list), idmef_node_children_list, _idmef_node_get_child, _idmef_node_new_child, _idmef_node_destroy_child, NULL, NULL, (void *) idmef_node_copy, (void *) idmef_node_clone, (void *) idmef_node_compare, (void *) idmef_node_print, (void *) idmef_node_ref, (void *) idmef_node_destroy, 0 },/* ID: 19 */
        { "source_spoofed", 0, NULL, NULL, NULL, NULL, (void *) idmef_source_spoofed_to_numeric, (void *) idmef_source_spoofed_to_string, NULL, NULL, NULL, NULL, NULL, 0 }, /* ID: 20 */
        { "source", sizeof(idmef_source_children_list) / sizeof(*idmef_source_children_list), idmef_source_children_list, _idmef_source_get_child, _idmef_source_new_child, _idmef_source_destroy_child, NULL, NULL, (void *) idmef_source_copy, (void *) idmef_source_clone, (void *) idmef_source_compare, (void *) idmef_source_print, (void *) idmef_source_ref, (void *) idmef_source_destroy, 1 },/* ID: 21 */
        { "file_access", sizeof(idmef_file_access_children_list) / sizeof(*idmef_file_access_children_list), idmef_file_access_children_list, _idmef_file_access_get_child, _idmef_file_access_new_child, _idmef_file_access_destroy_child, NULL, NULL, (void *) idmef_file_access_copy, (void *) idmef_file_access_clone, (void *) idmef_file_access_compare, (void *) idmef_file_access_print, (void *) idmef_file_access_ref, (void *) idmef_file_access_destroy, 1 },/* ID: 22 */
        { "inode", sizeof(idmef_inode_children_list) / sizeof(*idmef_inode_children_list), idmef_inode_children_list, _idmef_inode_get_child, _idmef_inode_new_child, _idmef_inode_destroy_child, NULL, NULL, (void *) idmef_inode_copy, (void *) idmef_inode_clone, (void *) idmef_inode_compare, (void *) idmef_inode_print, (void *) idmef_inode_ref, (void *) idmef_inode_destroy, 0 },/* ID: 23 */
        { "file_category", 0, NULL, NULL, NULL, NULL, (void *) idmef_file_category_to_numeric, (void *) idmef_file_category_to_string, NULL, NULL, NULL, NULL, NULL, 0 }, /* ID: 24 */
        { "file_fstype", 0, NULL, NULL, NULL, NULL, (void *) idmef_file_fstype_to_numeric, (void *) idmef_file_fstype_to_string, NULL, NULL, NULL, NULL, NULL, 0 }, /* ID: 25 */
        { "file", sizeof(idmef_file_children_list) / sizeof(*idmef_file_children_list), idmef_file_children_list, _idmef_file_get_child, _idmef_file_new_child, _idmef_file_destroy_child, NULL, NULL, (void *) idmef_file_copy, (void *) idmef_file_clone, (void *) idmef_file_compare, (void *) idmef_file_print, (void *) idmef_file_ref, (void *) idmef_file_destroy, 1 },/* ID: 26 */
        { "linkage_category", 0, NULL, NULL, NULL, NULL, (void *) idmef_linkage_category_to_numeric, (void *) idmef_linkage_category_to_string, NULL, NULL, NULL, NULL, NULL, 0 }, /* ID: 27 */
        { "linkage", sizeof(idmef_linkage_children_list) / sizeof(*idmef_linkage_children_list), idmef_linkage_children_list, _idmef_linkage_get_child, _idmef_linkage_new_child, _idmef_linkage_destroy_child, NULL, NULL, (void *) idmef_linkage_copy, (void *) idmef_linkage_clone, (void *) idmef_linkage_compare, (void *) idmef_linkage_print, (void *) idmef_linkage_ref, (void *) idmef_linkage_destroy, 1 },/* ID: 28 */
        { "target_decoy", 0, NULL, NULL, NULL, NULL, (void *) idmef_target_decoy_to_numeric, (void *) idmef_target_decoy_to_string, NULL, NULL, NULL, NULL, NULL, 0 }, /* ID: 29 */
        { "target", sizeof(idmef_target_children_list) / sizeof(*idmef_target_children_list), idmef_target_children_list, _idmef_target_get_child, _idmef_target_new_child, _idmef_target_destroy_child, NULL, NULL, (void *) idmef_target_copy, (void *) idmef_target_clone, (void *) idmef_target_compare, (void *) idmef_target_print, (void *) idmef_target_ref, (void *) idmef_target_destroy, 1 },/* ID: 30 */
        { "analyzer", sizeof(idmef_analyzer_children_list) / sizeof(*idmef_analyzer_children_list), idmef_analyzer_children_list, _idmef_analyzer_get_child, _idmef_analyzer_new_child, _idmef_analyzer_destroy_child, NULL, NULL, (void *) idmef_analyzer_copy, (void *) idmef_analyzer_clone, (void *) idmef_analyzer_compare, (void *) idmef_analyzer_print, (void *) idmef_analyzer_ref, (void *) idmef_analyzer_destroy, 1 },/* ID: 31 */
        { "alertident", sizeof(idmef_alertident_children_list) / sizeof(*idmef_alertident_children_list), idmef_alertident_children_list, _idmef_alertident_get_child, _idmef_alertident_new_child, _idmef_alertident_destroy_child, NULL, NULL, (void *) idmef_alertident_copy, (void *) idmef_alertident_clone, (void *) idmef_alertident_compare, (void *) idmef_alertident_print, (void *) idmef_alertident_ref, (void *) idmef_alertident_destroy, 1 },/* ID: 32 */
        { "impact_severity", 0, NULL, NULL, NULL, NULL, (void *) idmef_impact_severity_to_numeric, (void *) idmef_impact_severity_to_string, NULL, NULL, NULL, NULL, NULL, 0 }, /* ID: 33 */
        { "impact_completion", 0, NULL, NULL, NULL, NULL, (void *) idmef_impact_completion_to_numeric, (void *) idmef_impact_completion_to_string, NULL, NULL, NULL, NULL, NULL, 0 }, /* ID: 34 */
        { "impact_type", 0, NULL, NULL, NULL, NULL, (void *) idmef_impact_type_to_numeric, (void *) idmef_impact_type_to_string, NULL, NULL, NULL, NULL, NULL, 0 }, /* ID: 35 */
        { "impact", sizeof(idmef_impact_children_list) / sizeof(*idmef_impact_children_list), idmef_impact_children_list, _idmef_impact_get_child, _idmef_impact_new_child, _idmef_impact_destroy_child, NULL, NULL, (void *) idmef_impact_copy, (void *) idmef_impact_clone, (void *) idmef_impact_compare, (void *) idmef_impact_print, (void *) idmef_impact_ref, (void *) idmef_impact_destroy, 0 },/* ID: 36 */
        { "action_category", 0, NULL, NULL, NULL, NULL, (void *) idmef_action_category_to_numeric, (void *) idmef_action_category_to_string, NULL, NULL, NULL, NULL, NULL, 0 }, /* ID: 37 */
        { "action", sizeof(idmef_action_children_list) / sizeof(*idmef_action_children_list), idmef_action_children_list, _idmef_action_get_child, _idmef_action_new_child, _idmef_action_destroy_child, NULL, NULL, (void *) idmef_action_copy, (void *) idmef_action_clone, (void *) idmef_action_compare, (void *) idmef_action_print, (void *) idmef_action_ref, (void *) idmef_action_destroy, 1 },/* ID: 38 */
        { "confidence_rating", 0, NULL, NULL, NULL, NULL, (void *) idmef_confidence_rating_to_numeric, (void *) idmef_confidence_rating_to_string, NULL, NULL, NULL, NULL, NULL, 0 }, /* ID: 39 */
        { "confidence", sizeof(idmef_confidence_children_list) / sizeof(*idmef_confidence_children_list), idmef_confidence_children_list, _idmef_confidence_get_child, _idmef_confidence_new_child, _idmef_confidence_destroy_child, NULL, NULL, (void *) idmef_confidence_copy, (void *) idmef_confidence_clone, (void *) idmef_confidence_compare, (void *) idmef_confidence_print, (void *) idmef_confidence_ref, (void *) idmef_confidence_destroy, 0 },/* ID: 40 */
        { "assessment", sizeof(idmef_assessment_children_list) / sizeof(*idmef_assessment_children_list), idmef_assessment_children_list, _idmef_assessment_get_child, _idmef_assessment_new_child, _idmef_assessment_destroy_child, NULL, NULL, (void *) idmef_assessment_copy, (void *) idmef_assessment_clone, (void *) idmef_assessment_compare, (void *) idmef_assessment_print, (void *) idmef_assessment_ref, (void *) idmef_assessment_destroy, 0 },/* ID: 41 */
        { "tool_alert", sizeof(idmef_tool_alert_children_list) / sizeof(*idmef_tool_alert_children_list), idmef_tool_alert_children_list, _idmef_tool_alert_get_child, _idmef_tool_alert_new_child, _idmef_tool_alert_destroy_child, NULL, NULL, (void *) idmef_tool_alert_copy, (void *) idmef_tool_alert_clone, (void *) idmef_tool_alert_compare, (void *) idmef_tool_alert_print, (void *) idmef_tool_alert_ref, (void *) idmef_tool_alert_destroy, 0 },/* ID: 42 */
        { "correlation_alert", sizeof(idmef_correlation_alert_children_list) / sizeof(*idmef_correlation_alert_children_list), idmef_correlation_alert_children_list, _idmef_correlation_alert_get_child, _idmef_correlation_alert_new_child, _idmef_correlation_alert_destroy_child, NULL, NULL, (void *) idmef_correlation_alert_copy, (void *) idmef_correlation_alert_clone, (void *) idmef_correlation_alert_compare, (void *) idmef_correlation_alert_print, (void *) idmef_correlation_alert_ref, (void *) idmef_correlation_alert_destroy, 0 },/* ID: 43 */
        { "overflow_alert", sizeof(idmef_overflow_alert_children_list) / sizeof(*idmef_overflow_alert_children_list), idmef_overflow_alert_children_list, _idmef_overflow_alert_get_child, _idmef_overflow_alert_new_child, _idmef_overflow_alert_destroy_child, NULL, NULL, (void *) idmef_overflow_alert_copy, (void *) idmef_overflow_alert_clone, (void *) idmef_overflow_alert_compare, (void *) idmef_overflow_alert_print, (void *) idmef_overflow_alert_ref, (void *) idmef_overflow_alert_destroy, 0 },/* ID: 44 */
        { "alert_type", 0, NULL, NULL, NULL, NULL, (void *) idmef_alert_type_to_numeric, (void *) idmef_alert_type_to_string, NULL, NULL, NULL, NULL, NULL, 0 }, /* ID: 45 */
        { "alert", sizeof(idmef_alert_children_list) / sizeof(*idmef_alert_children_list), idmef_alert_children_list, _idmef_alert_get_child, _idmef_alert_new_child, _idmef_alert_destroy_child, NULL, NULL, (void *) idmef_alert_copy, (void *) idmef_alert_clone, (void *) idmef_alert_compare, (void *) idmef_alert_print, (void *) idmef_alert_ref, (void *) idmef_alert_destroy, 0 },/* ID: 46 */
        { "heartbeat", sizeof(idmef_heartbeat_children_list) / sizeof(*idmef_heartbeat_children_list), idmef_heartbeat_children_list, _idmef_heartbeat_get_child, _idmef_heartbeat_new_child, _idmef_heartbeat_destroy_child, NULL, NULL, (void *) idmef_heartbeat_copy, (void *) idmef_heartbeat_clone, (void *) idmef_heartbeat_compare, (void *) idmef_heartbeat_print, (void *) idmef_heartbeat_ref, (void *) idmef_heartbeat_destroy, 0 },/* ID: 47 */
        { "message_type", 0, NULL, NULL, NULL, NULL, (void *) idmef_message_type_to_numeric, (void *) idmef_message_type_to_string, NULL, NULL, NULL, NULL, NULL, 0 }, /* ID: 48 */
        { "message", sizeof(idmef_message_children_list) / sizeof(*idmef_message_children_list), idmef_message_children_list, _idmef_message_get_child, _idmef_message_new_child, _idmef_message_destroy_child, NULL, NULL, (void *) idmef_message_copy, (void *) idmef_message_clone, (void *) idmef_message_compare, (void *) idmef_message_print, (void *) idmef_message_ref, (void *) idmef_message_destroy, 0 },/* ID: 49 */
        { "reference", sizeof(idmef_reference_children_list) / sizeof(*idmef_reference_children_list), idmef_reference_children_list, _idmef_reference_get_child, _idmef_reference_new_child, _idmef_reference_destroy_child, NULL, NULL, (void *) idmef_reference_copy, (void *) idmef_reference_clone, (void *) idmef_reference_compare, (void *) idmef_reference_print, (void *) idmef_reference_ref, (void *) idmef_reference_destroy, 1 },/* ID: 50 */
        { "(unassigned)", 0, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL }, /* ID: 51 */
        { "checksum", sizeof(idmef_checksum_children_list) / sizeof(*idmef_checksum_children_list), idmef_checksum_children_list, _idmef_checksum_get_child, _idmef_checksum_new_child, _idmef_checksum_destroy_child, NULL, NULL, (void *) idmef_checksum_copy, (void *) idmef_checksum_clone, (void *) idmef_checksum_compare, (void *) idmef_checksum_print, (void *) idmef_checksum_ref, (void *) idmef_checksum_destroy, 1 },/* ID: 52 */
        { "checksum_algorithm", 0, NULL, NULL, NULL, NULL, (void *) idmef_checksum_algorithm_to_numeric, (void *) idmef_checksum_algorithm_to_string, NULL, NULL, NULL, NULL, NULL, 0 }, /* ID: 53 */
        { NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL }
};
