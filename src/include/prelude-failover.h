/*****
*
* Copyright (C) 2004 Yoann Vandoorselaere <yoann@prelude-ids.org>
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

#ifndef _LIBPRELUDE_PRELUDE_FAILOVER_H
#define _LIBPRELUDE_PRELUDE_FAILOVER_H

typedef struct prelude_failover prelude_failover_t;

void prelude_failover_destroy(prelude_failover_t *failover);

prelude_failover_t *prelude_failover_new(const char *dirname);

void prelude_failover_set_quota(prelude_failover_t *failover, size_t limit);

int prelude_failover_save_msg(prelude_failover_t *failover, prelude_msg_t *msg);

ssize_t prelude_failover_get_saved_msg(prelude_failover_t *failover, prelude_msg_t **out);

unsigned int prelude_failover_get_deleted_msg_count(prelude_failover_t *failover);

unsigned int prelude_failover_get_available_msg_count(prelude_failover_t *failover);

#endif
