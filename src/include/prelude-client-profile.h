#ifndef _LIBPRELUDE_CLIENT_PROFILE_H
#define _LIBPRELUDE_CLIENT_PROFILE_H

#include <unistd.h>
#include <gnutls/gnutls.h>

#include "prelude-inttypes.h"

typedef struct prelude_client_profile prelude_client_profile_t;

int _prelude_client_profile_init(prelude_client_profile_t *cp);

int _prelude_client_profile_new(prelude_client_profile_t **ret);

int prelude_client_profile_new(prelude_client_profile_t **ret, const char *name);

void prelude_client_profile_destroy(prelude_client_profile_t *cp);

void prelude_client_profile_get_analyzerid_filename(prelude_client_profile_t *cp, char *buf, size_t size);

void prelude_client_profile_get_tls_key_filename(prelude_client_profile_t *cp, char *buf, size_t size);

void prelude_client_profile_get_tls_server_ca_cert_filename(prelude_client_profile_t *cp, char *buf, size_t size);

void prelude_client_profile_get_tls_server_keycert_filename(prelude_client_profile_t *cp, char *buf, size_t size);

void prelude_client_profile_get_tls_server_trusted_cert_filename(prelude_client_profile_t *cp, char *buf, size_t size);

void prelude_client_profile_get_tls_client_keycert_filename(prelude_client_profile_t *cp, char *buf, size_t size);

void prelude_client_profile_get_tls_client_trusted_cert_filename(prelude_client_profile_t *cp, char *buf, size_t size);

void prelude_client_profile_get_backup_dirname(prelude_client_profile_t *cp, char *buf, size_t size);

void prelude_client_profile_set_uid(prelude_client_profile_t *cp, uid_t uid);

uid_t prelude_client_profile_get_uid(prelude_client_profile_t *cp);

void prelude_client_profile_set_gid(prelude_client_profile_t *cp, uid_t gid);

gid_t prelude_client_profile_get_gid(prelude_client_profile_t *cp);

int prelude_client_profile_set_name(prelude_client_profile_t *cp, const char *name);

const char *prelude_client_profile_get_name(prelude_client_profile_t *cp);

uint64_t prelude_client_profile_get_analyzerid(prelude_client_profile_t *cp);

void prelude_client_profile_set_analyzerid(prelude_client_profile_t *cp, uint64_t analyzerid);

gnutls_certificate_credentials prelude_client_profile_get_credentials(prelude_client_profile_t *cp);

#endif
