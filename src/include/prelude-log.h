/*****
*
* Copyright (C) 1998,1999,2000 Yoann Vandoorselaere
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

#ifndef _LIBPRELUDE_PRELUDE_LOG_H
#define _LIBPRELUDE_PRELUDE_LOG_H

#include <stdarg.h>
#include <syslog.h>

#define log(priority, ...) \
        prelude_log(priority, __FILE__, __FUNCTION__, __LINE__, __VA_ARGS__)

#ifndef LOG_ERR
 #define LOG_ERR 1
#endif

#ifndef LOG_INFO
 #define LOG_INFO 0
#endif

typedef enum {
        PRELUDE_LOG_PRIORITY_INFO   = LOG_INFO,
        PRELUDE_LOG_PRIORITY_ERROR  = LOG_ERR,
} prelude_log_priority_t;


typedef enum {
        PRELUDE_LOG_FLAGS_QUIET  = 0x01,
        PRELUDE_LOG_FLAGS_SYSLOG = 0x02
} prelude_log_flags_t;


prelude_log_flags_t prelude_log_get_flags(void);

void prelude_log_set_flags(prelude_log_flags_t flags);

void prelude_log_use_syslog(void);

char *prelude_log_get_prefix(void);

void prelude_log_set_prefix(char *prefix);

void prelude_log(prelude_log_priority_t priority, const char *file,
                 const char *function, int line, const char *fmt, ...);

void prelude_log_v(prelude_log_priority_t priority, const char *file,
                   const char *function, int line, const char *fmt, va_list ap);

#endif /* _LIBPRELUDE_PRELUDE_LOG_H */
