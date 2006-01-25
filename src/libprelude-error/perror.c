#include <stdio.h>
#include <stdarg.h>

#include "prelude-log.h"
#include "prelude-error.h"


void prelude_perror(prelude_error_t error, const char *fmt, ...)
{
	va_list ap;
	char buf[1024];
        
	va_start(ap, fmt);
	vsnprintf(buf, sizeof(buf), fmt, ap);
	va_end(ap);

        if ( prelude_error_get_source(error) )
                prelude_log(PRELUDE_LOG_WARN, "%s: %s: %s.\n", prelude_strsource(error), buf, prelude_strerror(error));
        else
                prelude_log(PRELUDE_LOG_WARN, "%s: %s.\n", buf, prelude_strerror(error));
}
