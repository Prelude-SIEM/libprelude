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

#include "libmissing.h"

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>
#include <syslog.h>

#include "prelude-log.h"
#include "prelude-error.h"
#include "prelude-inttypes.h"


static void do_log_print(prelude_log_t level, const char *str);



static FILE *global_logfile = NULL;
static char *global_prefix = NULL;
static int log_level = PRELUDE_LOG_INFO;
static prelude_log_flags_t log_flags = 0;
static void (*global_log_cb)(prelude_log_t level, const char *str) = do_log_print;


static inline FILE *get_out_fd(prelude_log_t level)
{        
        return (level < PRELUDE_LOG_INFO) ? stderr : stdout;
}



static void do_log_print(prelude_log_t level, const char *str)
{
        FILE *out = get_out_fd(level);
        fprintf(out, "%s", str);
}



static void do_log_syslog(prelude_log_t level, const char *str)
{
        while (*str == '\n' ) str++;
        syslog(level, "%s", str);
}



static inline prelude_bool_t need_to_log(prelude_log_t level)
{
        return (level > log_level) ? FALSE : TRUE;
}



static void do_log_v(prelude_log_t level, const char *file,
                     const char *function, int line, const char *fmt, va_list ap)
{
        char buf[1024];
        ssize_t ret, len;
        
        if ( level >= PRELUDE_LOG_DEBUG || level == PRELUDE_LOG_ERR ) {

                len = snprintf(buf, sizeof(buf), "%s%s:%s:%d: ", (global_prefix) ?
                               global_prefix : "", file, function, line);
                if ( len < 0 || len >= sizeof(buf) )
                        return;
                
                vsnprintf(buf + len, sizeof(buf) - len, fmt, ap);
        }

        else {
                len = snprintf(buf, sizeof(buf), "%s", (global_prefix) ? global_prefix : "");
                if ( len < 0 || len >= sizeof(buf) )
                        return;
                
                ret = vsnprintf(buf + len, sizeof(buf) - len, fmt, ap);
                if ( ret < 0 || (ret + len) >= sizeof(buf) )
                        return;
        }

        global_log_cb(level, buf);
        
        if ( global_logfile )
                fprintf(global_logfile, "%s", buf);
}



void _prelude_log_v(prelude_log_t level, const char *file,
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

        if ( flags & PRELUDE_LOG_FLAGS_SYSLOG )
                global_log_cb = do_log_syslog;
        else
                global_log_cb = do_log_print;
        
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



/**
 * prelude_log_set_callback:
 * @log_cb: Callback function.
 *
 * @log_cb() will be called in place of the prelude function usally
 * used for loging.
 */
void prelude_log_set_callback(void log_cb(prelude_log_t level, const char *str))
{
        global_log_cb = log_cb;
}



int prelude_log_set_logfile(const char *filename)
{
        if ( ! filename && global_logfile ) {
                fclose(global_logfile);
                global_logfile = NULL;
        }

        else {        
                global_logfile = fopen(filename, "a");
                if ( ! global_logfile )
                        return prelude_error_from_errno(errno);
        }
        
        return 0;
}
