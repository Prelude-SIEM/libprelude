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

#include "idmef.h"
#include "prelude-msg.h"
#include "prelude-inttypes.h"
#include <netinet/in.h>
#include <time.h>

void *prelude_realloc(void *ptr, size_t size);

int prelude_resolve_addr(const char *hostname, struct in_addr *addr);

int prelude_open_persistant_tmpfile(const char *filename, int flags, mode_t mode);

int prelude_read_multiline(FILE *fd, int *line, char *buf, size_t size);

uint64_t prelude_hton64(uint64_t val);

int prelude_get_file_name_and_path(const char *str, char **name, char **path);

int prelude_get_gmt_offset(int32_t *gmt_offset);

time_t prelude_timegm(struct tm *tm);

prelude_msg_priority_t idmef_impact_severity_to_msg_priority(idmef_impact_severity_t severity);

#endif /* _LIBPRELUDE_COMMON_H */
