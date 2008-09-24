/*****
*
* Copyright (C) 2004, 2005 PreludeIDS Technologies. All Rights Reserved.
* Author: Yoann Vandoorselaere <yoann.v@prelude-ids.com>
*
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

#ifndef _LIBPRELUDE_CLIENT_PROFILE_H
#define _LIBPRELUDE_CLIENT_PROFILE_H

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <unistd.h>
#include <sys/types.h>

#include "prelude-config.h"
#include "prelude-inttypes.h"

#ifdef __cplusplus
 extern "C" {
#endif


#ifdef HAVE_UID_T
typedef uid_t prelude_uid_t;
#else
typedef int prelude_uid_t;
#endif

#ifdef HAVE_GID_T
typedef gid_t prelude_gid_t;
#else
typedef int prelude_gid_t;
#endif


typedef struct prelude_client_profile prelude_client_profile_t;

int _prelude_client_profile_init(prelude_client_profile_t *cp);

int _prelude_client_profile_new(prelude_client_profile_t **ret);

int prelude_client_profile_new(prelude_client_profile_t **ret, const char *name);

prelude_client_profile_t *prelude_client_profile_ref(prelude_client_profile_t *cp);

void prelude_client_profile_destroy(prelude_client_profile_t *cp);

void prelude_client_profile_get_config_filename(const prelude_client_profile_t *cp, char *buf, size_t size);

void prelude_client_profile_get_default_config_dirname(const prelude_client_profile_t *cp, char *buf, size_t size);

void prelude_client_profile_get_analyzerid_filename(const prelude_client_profile_t *cp, char *buf, size_t size);

void prelude_client_profile_get_tls_key_filename(const prelude_client_profile_t *cp, char *buf, size_t size);

void prelude_client_profile_get_tls_server_ca_cert_filename(const prelude_client_profile_t *cp, char *buf, size_t size);

void prelude_client_profile_get_tls_server_keycert_filename(const prelude_client_profile_t *cp, char *buf, size_t size);

void prelude_client_profile_get_tls_server_crl_filename(const prelude_client_profile_t *cp, char *buf, size_t size);

void prelude_client_profile_get_tls_client_keycert_filename(const prelude_client_profile_t *cp, char *buf, size_t size);

void prelude_client_profile_get_tls_client_trusted_cert_filename(const prelude_client_profile_t *cp, char *buf, size_t size);

void prelude_client_profile_get_backup_dirname(const prelude_client_profile_t *cp, char *buf, size_t size);

void prelude_client_profile_get_profile_dirname(const prelude_client_profile_t *cp, char *buf, size_t size);

void prelude_client_profile_set_uid(prelude_client_profile_t *cp, prelude_uid_t uid);

prelude_uid_t prelude_client_profile_get_uid(const prelude_client_profile_t *cp);

void prelude_client_profile_set_gid(prelude_client_profile_t *cp, prelude_uid_t gid);

prelude_gid_t prelude_client_profile_get_gid(const prelude_client_profile_t *cp);

int prelude_client_profile_set_name(prelude_client_profile_t *cp, const char *name);

const char *prelude_client_profile_get_name(const prelude_client_profile_t *cp);

uint64_t prelude_client_profile_get_analyzerid(const prelude_client_profile_t *cp);

void prelude_client_profile_set_analyzerid(prelude_client_profile_t *cp, uint64_t analyzerid);

int prelude_client_profile_get_credentials(prelude_client_profile_t *cp, void **credentials);

int prelude_client_profile_set_prefix(prelude_client_profile_t *cp, const char *prefix);

void prelude_client_profile_get_prefix(const prelude_client_profile_t *cp, char *buf, size_t size);

#ifdef __cplusplus
 }
#endif

#endif
