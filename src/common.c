#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>

#include "common.h"


static int config_quiet = 0;



static void syslog_log(int priority, const char *file,
                       const char *function, int line, const char *fmt, va_list *ap) 
{        
        if ( priority == LOG_ERR ) 
                syslog(priority, "%s:%s:%d : (errno=%s) : ", file, function, line, strerror(errno));
        syslog(priority, fmt, va_arg(*ap, char *));
}



static void standard_log(int priority, const char *file,
                         const char *function, int line, const char *fmt, va_list *ap)
{
        FILE *out;
        
        if ( priority == LOG_ERR ) {
                out = stderr;
                fprintf(out, "%s:%s:%d : (errno=%s) : ", file, function, line, strerror(errno));
        } else
                out = stdout;
        
        fprintf(out, fmt, va_arg(*ap, char *));
}




/**
 * prelude_log:
 * @priority: LOG_INFO or LOG_ERR.
 * @file: The caller filename.
 * @function: The caller function name.
 * @line: The caller line number.
 * @fmt: Format string.
 *
 * This function should not be called directly.
 * Use the #log macro defined in common.h
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
