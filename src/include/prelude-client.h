#ifndef _LIBPRELUDE_ANALYZER_H
#define _LIBPRELUDE_ANALYZER_H


typedef enum {
        PRELUDE_CLIENT_CAPABILITY_RECV_IDMEF = 0x01,
        PRELUDE_CLIENT_CAPABILITY_SEND_IDMEF = 0x02,
        PRELUDE_CLIENT_CAPABILITY_RECV_ADMIN = 0x04,
        PRELUDE_CLIENT_CAPABILITY_SEND_ADMIN = 0x08,
        PRELUDE_CLIENT_CAPABILITY_RECV_CM    = 0x10,
        PRELUDE_CLIENT_CAPABILITY_SEND_CM    = 0x20,
} prelude_client_capability_t;

#define PRELUDE_CLIENT_ASYNC_SEND  0x01
#define PRELUDE_CLIENT_ASYNC_TIMER 0x02

typedef struct prelude_client prelude_client_t;

#include "prelude-ident.h"
#include "prelude-connection.h"
#include "prelude-connection-mgr.h"
#include "idmef.h"

prelude_ident_t *prelude_client_get_unique_ident(prelude_client_t *client);

prelude_connection_mgr_t *prelude_client_get_manager_list(prelude_client_t *client);

int prelude_client_init(prelude_client_t *client, const char *sname, const char *config, int argc, char **argv);

prelude_client_t *prelude_client_new(prelude_client_capability_t capability);

idmef_analyzer_t *prelude_client_get_analyzer(prelude_client_t *client);

uint64_t prelude_client_get_analyzerid(prelude_client_t *client);

void prelude_client_set_name(prelude_client_t *client, const char *name);

const char *prelude_client_get_name(prelude_client_t *client);

void prelude_client_set_uid(prelude_client_t *client, uid_t uid);

uid_t prelude_client_get_uid(prelude_client_t *client);

void prelude_client_set_gid(prelude_client_t *client, gid_t gid);

gid_t prelude_client_get_gid(prelude_client_t *client);

int prelude_client_get_flags(prelude_client_t *client);

prelude_client_capability_t prelude_client_get_capability(prelude_client_t *client);

void prelude_client_send_msg(prelude_client_t *client, prelude_msg_t *msg);

void prelude_client_set_heartbeat_cb(prelude_client_t *client, void (*cb)(prelude_client_t *client));

void prelude_client_destroy(prelude_client_t *client);

int prelude_client_set_flags(prelude_client_t *client, int flags);

void prelude_client_set_capability(prelude_client_t *client, prelude_client_capability_t capability);

void prelude_client_get_auth_filename(prelude_client_t *client, char *buf, size_t size);

void prelude_client_get_ssl_cert_filename(prelude_client_t *client, char *buf, size_t size);

void prelude_client_get_ssl_key_filename(prelude_client_t *client, char *buf, size_t size);

void prelude_client_get_backup_filename(prelude_client_t *client, char *buf, size_t size);

#endif
