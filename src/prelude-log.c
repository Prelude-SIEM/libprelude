/*****
*
* Copyright (C) 2001, 2002 Yoann Vandoorselaere <yoann@mandrakesoft.com>
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

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>

#include "prelude-log.h"


static int config_quiet = 0;
static char *global_prefix = NULL;


static void syslog_log(int priority, const char *file,
                       const char *function, int line, const char *fmt, va_list *ap) 
{
        int len;
        char buf[256];
                
        if ( priority == LOG_ERR ) {
                vsnprintf(buf, sizeof(buf), fmt, *ap);

                syslog(priority, "%s%s:%s:%d : (errno=%s) : %s", (global_prefix) ? global_prefix : "",
                       file, function, line, strerror(errno), buf);
        }

        else {
                len = snprintf(buf, sizeof(buf), "%s", (global_prefix) ? global_prefix : "");
                len += vsnprintf(buf + len, sizeof(buf) - len, fmt, *ap);
                syslog(priority, "%s", buf);
        }
}




static void standard_log(int priority, const char *file,
                         const char *function, int line, const char *fmt, va_list *ap)
{
        FILE *out;
        
        if ( priority == LOG_ERR ) {
                out = stderr;                
                if ( global_prefix ) fprintf(out, "%s", global_prefix);
                fprintf(out, "%s:%s:%d : (errno=%s) : ", file, function, line, strerror(errno));
        } else {
                out = stdout;
                if ( global_prefix ) fprintf(out, "%s", global_prefix);
        }
        
        vfprintf(out, fmt, *ap);
}




/**
 * prelude_log:
 * @priority: LOG_INFO or LOG_ERR.
 * @file: The caller filename.
 * @function: The caller function name.
 * @line: The caller line number.
 * @fmt: Format string.
 * @...: Variable argument list.
 *
 * This function should not be called directly.
 * Use the #log macro defined in prelude-log.h
 */
void prelude_log(int priority, const char *file,
                 const char *function, int line, const char *fmt, ...) 
{
        va_list ap;
        
        va_start(ap, fmt);
        
        if ( config_quiet )
                syslog_log(priority, file, function, line, fmt, &ap);
        else
                standard_log(priority, file, function, line, fmt, &ap);
        
        va_end(ap);
}
        


/**
 * prelude_log_use_syslog:
 *
 * Tell the Prelude standard logger to log throught syslog.
 * (this is usefull in case the program using the log function
 * is a daemon).
 */
void prelude_log_use_syslog(void) 
{
        config_quiet = 1;
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


