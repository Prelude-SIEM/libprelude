/*****
*
* Copyright (C) 1998,1999,2000 Vandoorselaere Yoann
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

#include "syslog.h"


void prelude_log_use_syslog(void);

char *prelude_log_get_prefix(void);

void prelude_log_set_prefix(char *prefix);

void prelude_log(int priority, const char *file, const char *function, int line, const char *fmt, ...);


#define log(priority, args...) \
        prelude_log(priority, __FILE__, __FUNCTION__, __LINE__, args)


#define do_init(func, name) do {                         \
        char *old_prefix = prelude_log_get_prefix();     \
        prelude_log_set_prefix(NULL);                    \
        log(LOG_INFO, "- %s\n", name);                   \
        prelude_log_set_prefix(old_prefix);              \
        if ( (func) < 0 )                                \
                exit(1);                                 \
        prelude_log_set_prefix(old_prefix);              \
} while(0);


#define do_init_nofail(func, name) do {                  \
        char *old_prefix = prelude_log_get_prefix();     \
        prelude_log_set_prefix(NULL);                    \
        log(LOG_INFO, "- %s\n", name);                   \
        prelude_log_set_prefix(old_prefix);              \
        (func);                                          \
} while(0);
