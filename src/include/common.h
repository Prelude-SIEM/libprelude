/*****
*
* Copyright (C) 2002, 2003, 2004, 2005 PreludeIDS Technologies. All Rights Reserved.
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
* You should have received a copy of the GNU General Public License
* along with this program; see the file COPYING.  If not, write to
* the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
*
*****/

#ifndef _LIBPRELUDE_COMMON_H
#define _LIBPRELUDE_COMMON_H

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "idmef.h"
#include "prelude-msg.h"
#include "prelude-inttypes.h"
#include "prelude-log.h"
#include <sys/types.h>

#ifdef WIN32
# include <winsock2.h>
#else
# include <sys/socket.h>
# include <netinet/in.h>
#endif

#include <time.h>

#ifdef __cplusplus
 extern "C" {
#endif

#define prelude_return_val_if_fail(cond, val) do {                               \
        if ( ! (cond) ) {                                                        \
                prelude_log(PRELUDE_LOG_CRIT, "assertion '%s' failed\n", #cond); \
                return val;                                                      \
        }                                                                        \
} while(0)


#define prelude_return_if_fail(cond) do {                                        \
        if ( ! (cond) ) {                                                        \
                prelude_log(PRELUDE_LOG_CRIT, "assertion '%s' failed\n", #cond); \
                return;                                                          \
        }                                                                        \
} while(0)


int prelude_parse_address(const char *str, char **addr, unsigned int *port);

uint64_t prelude_hton64(uint64_t val);

uint32_t prelude_htonf(float fval);

time_t prelude_timegm(struct tm *tm);

int prelude_get_gmt_offset(long *gmt_offset);

int prelude_get_gmt_offset_from_tm(struct tm *tm, long *gmtoff);

int prelude_get_gmt_offset_from_time(const time_t *utc, long *gmtoff);

int prelude_read_multiline(FILE *fd, unsigned int *line, char *buf, size_t size);

int prelude_read_multiline2(FILE *fd, unsigned int *line, prelude_string_t *out);

void *prelude_sockaddr_get_inaddr(struct sockaddr *sa);

void *_prelude_realloc(void *ptr, size_t size);

int _prelude_get_file_name_and_path(const char *str, char **name, char **path);

prelude_msg_priority_t _idmef_impact_severity_to_msg_priority(idmef_impact_severity_t severity);

int _idmef_message_assign_missing(prelude_client_t *client, idmef_message_t *msg);

int _prelude_load_file(const char *filename, unsigned char **fdata, size_t *outsize);

void _prelude_unload_file(unsigned char *fdata, size_t size);

#ifdef __cplusplus
 }
#endif

#endif /* _LIBPRELUDE_COMMON_H */
