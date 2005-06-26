/*****
*
* Copyright (C) 2005 PreludeIDS Technologies. All Rights Reserved.
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

#ifndef _LIBPRELUDE_PRELUDE_LOG_H
#define _LIBPRELUDE_PRELUDE_LOG_H

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdarg.h>

#ifdef __cplusplus
 extern "C" {
#endif

#define prelude_log(level, ...) \
        _prelude_log(level, __FILE__, __FUNCTION__, __LINE__, __VA_ARGS__)

#define prelude_log_debug(level, ...) \
        _prelude_log(PRELUDE_LOG_DEBUG + level, __FILE__, __FUNCTION__, __LINE__, __VA_ARGS__)


typedef enum {
        PRELUDE_LOG_ERR  =  0,
        PRELUDE_LOG_WARN =  1,
        PRELUDE_LOG_INFO =  2,
        PRELUDE_LOG_DEBUG = 3
} prelude_log_t;


typedef enum {
        PRELUDE_LOG_FLAGS_QUIET  = 0x01, /* Drop PRELUDE_LOG_PRIORITY_INFO */
        PRELUDE_LOG_FLAGS_SYSLOG = 0x02
} prelude_log_flags_t;


void prelude_log_set_level(prelude_log_t level);

void prelude_log_set_debug_level(int level);

prelude_log_flags_t prelude_log_get_flags(void);

void prelude_log_set_flags(prelude_log_flags_t flags);

char *prelude_log_get_prefix(void);

void prelude_log_set_prefix(char *prefix);

void prelude_log_set_callback(void log_cb(prelude_log_t level, const char *str));
         
void prelude_log_v(prelude_log_t level, const char *file,
                   const char *function, int line, const char *fmt, va_list ap);

void _prelude_log(prelude_log_t level, const char *file,
                  const char *function, int line, const char *fmt, ...);

#ifdef __cplusplus
 }
#endif
         
#endif /* _LIBPRELUDE_PRELUDE_LOG_H */
