/*****
*
* Copyright (C) 2001,2002,2003,2004,2005 PreludeIDS Technologies. All Rights Reserved.
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

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>
#include <syslog.h>

#include "libmissing.h"
#include "prelude-log.h"
#include "prelude-inttypes.h"


static char *global_prefix = NULL;
static int log_level = PRELUDE_LOG_INFO;
static prelude_log_flags_t log_flags = 0;



static char *strip_return(char *buf, size_t len)
{
        while ( buf[--len] == '\n' );
        
        buf[++len] = '\0';
        
        return buf;
}



static inline FILE *get_out_fd(prelude_log_t level)
{
        return (level < PRELUDE_LOG_INFO) ? stderr : stdout;
}



static inline prelude_bool_t need_to_log(prelude_log_t level)
{
        return (level > log_level) ? FALSE : TRUE;
}



static void syslog_log(prelude_log_t level, const char *file,
                       const char *function, int line, const char *fmt, va_list ap) 
{
        int len, ret;
        char buf[512];

        while (*fmt == '\n') fmt++;
        
        if ( level >= PRELUDE_LOG_DEBUG || level == PRELUDE_LOG_ERR ) {
                
                len = vsnprintf(buf, sizeof(buf), fmt, ap);
                if ( len < 0 || len >= sizeof(buf) )
                        return;
                
                syslog(level, "%s%s:%s:%d: %s", (global_prefix) ? global_prefix : "",
                       file, function, line, strip_return(buf, len));
        }

        else {
                len = snprintf(buf, sizeof(buf), "%s", (global_prefix) ? global_prefix : "");
                if ( len < 0 || len >= sizeof(buf) )
                        return;
                
                ret = vsnprintf(buf + len, sizeof(buf) - len, fmt, ap);
                if ( ret < 0 || (ret + len) >= sizeof(buf) )
                        return;
                
                syslog(level, "%s", strip_return(buf, ret + len));
        }
}




static void standard_log(prelude_log_t level, const char *file,
                         const char *function, int line, const char *fmt, va_list ap)
{
        FILE *out = get_out_fd(level);
        
        if ( global_prefix )
                fprintf(out, "%s", global_prefix);                

        if ( level >= PRELUDE_LOG_DEBUG || level == PRELUDE_LOG_ERR )
                fprintf(out, "%s:%s:%d: ", file, function, line);
        
        vfprintf(out, fmt, ap);
}



static void do_log_v(prelude_log_t level, const char *file,
                     const char *function, int line, const char *fmt, va_list ap)
{
        if ( log_flags & PRELUDE_LOG_FLAGS_SYSLOG )
                syslog_log(level, file, function, line, fmt, ap);
        else
                standard_log(level, file, function, line, fmt, ap);
}


void prelude_log_v(prelude_log_t level, const char *file,
                   const char *function, int line, const char *fmt, va_list ap) 
{
        if ( ! need_to_log(level) )
                return;

        do_log_v(level, file, function, line, fmt, ap);
}



/**
 * _prelude_log:
 * @level: PRELUDE_LOG_PRIORITY_INFO or PRELUDE_LOG_PRIORITY_ERROR.
 * @file: The caller filename.
 * @function: The caller function name.
 * @line: The caller line number.
 * @fmt: Format string.
 * @...: Variable argument list.
 *
 * This function should not be called directly.
 * Use the #log macro defined in prelude-log.h
 */
void _prelude_log(prelude_log_t level, const char *file,
                  const char *function, int line, const char *fmt, ...) 
{
        va_list ap;

        if ( ! need_to_log(level) )
                return;
                
        va_start(ap, fmt);
        do_log_v(level, file, function, line, fmt, ap);        
        va_end(ap);
}



/**
 * prelude_log_set_flags:
 * @flags:
 */
void prelude_log_set_flags(prelude_log_flags_t flags) 
{
        if ( flags & PRELUDE_LOG_FLAGS_QUIET )
                log_level = PRELUDE_LOG_WARN;
        
        log_flags = flags;
}


prelude_log_flags_t prelude_log_get_flags(void)
{
        return log_flags;
}



void prelude_log_set_level(prelude_log_t level)
{
        log_level = level;
}


void prelude_log_set_debug_level(int level)
{
        log_level = PRELUDE_LOG_DEBUG + level;
}



/**
 * prelude_log_set_prefix:
 * @prefix: Pointer to the prefix to use.
 *
 * Tell the Prelude standard logger to add @prefix before logging
 * a line.
 */
void prelude_log_set_prefix(char *prefix) 
{
        global_prefix = prefix;
}



char *prelude_log_get_prefix(void) 
{
        return global_prefix;
}


