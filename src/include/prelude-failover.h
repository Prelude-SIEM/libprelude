/*****
*
* Copyright (C) 2004-2006,2007 PreludeIDS Technologies. All Rights Reserved.
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

#ifndef _LIBPRELUDE_PRELUDE_FAILOVER_H
#define _LIBPRELUDE_PRELUDE_FAILOVER_H

#ifdef __cplusplus
 extern "C" {
#endif

typedef struct prelude_failover prelude_failover_t;

void prelude_failover_destroy(prelude_failover_t *failover);

int prelude_failover_new(prelude_failover_t **ret, const char *dirname);

void prelude_failover_set_quota(prelude_failover_t *failover, size_t limit);

int prelude_failover_save_msg(prelude_failover_t *failover, prelude_msg_t *msg);

ssize_t prelude_failover_get_saved_msg(prelude_failover_t *failover, prelude_msg_t **out);

unsigned long prelude_failover_get_deleted_msg_count(prelude_failover_t *failover);

unsigned long prelude_failover_get_available_msg_count(prelude_failover_t *failover);

void prelude_failover_enable_transaction(prelude_failover_t *failover);

void prelude_failover_disable_transaction(prelude_failover_t *failover);

int prelude_failover_commit(prelude_failover_t *failover, prelude_msg_t *msg);

int prelude_failover_rollback(prelude_failover_t *failover, prelude_msg_t *msg);

#ifdef __cplusplus
 }
#endif

#endif
