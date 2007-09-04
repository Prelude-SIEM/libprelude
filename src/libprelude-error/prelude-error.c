#include "config.h"

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <gettext.h>
#include <errno.h>

#include "prelude.h"
#include "prelude-log.h"
#include "prelude-error.h"

#include "code-to-errno.h"
#include "code-from-errno.h"


#define PRELUDE_ERROR_CODE_DIM     65536
#define PRELUDE_ERROR_SOURCE_DIM     256

#define PRELUDE_ERROR_SOURCE_SHIFT    23
#define PRELUDE_ERROR_VERBOSE_SHIFT   22

#define PRELUDE_ERROR_CODE_MASK       (PRELUDE_ERROR_CODE_DIM - 1)
#define PRELUDE_ERROR_SOURCE_MASK     (PRELUDE_ERROR_SOURCE_DIM - 1)
#define PRELUDE_ERROR_VERBOSE_MASK    (1)


/**
 * prelude_error_make:
 * @source: Error source.
 * @code: Error code.
 *
 * Create a new #prelude_error_t error using @source and @code.
 *
 * Returns: The created #prelude_error_t integer.
 */
prelude_error_t prelude_error_make(prelude_error_source_t source, prelude_error_code_t code)
{
        return (code == PRELUDE_ERROR_NO_ERROR) ? code : -((source << PRELUDE_ERROR_SOURCE_SHIFT) | code);
}


/**
 * prelude_error_make_from_errno:
 * @source: Error source.
 * @err: errno value.
 *
 * Create a new #prelude_error_t error using @source and @errno.
 *
 * Returns: The created #prelude_error_t integer.
 */
prelude_error_t prelude_error_make_from_errno(prelude_error_source_t source, int err)
{
        prelude_error_code_t code = prelude_error_code_from_errno(err);
        return prelude_error_make(source, code);
}



/**
 * prelude_error_verbose_make_v:
 * @source: Error source.
 * @code: Error code.
 * @fmt: Format string.
 * @ap: Argument list.
 *
 * Create a new error using @source and @code, using the detailed error message
 * specified within @fmt.
 *
 * Returns: The created #prelude_error_t integer.
 */
prelude_error_t prelude_error_verbose_make_v(prelude_error_source_t source,
                                             prelude_error_code_t code, const char *fmt, va_list ap)
{
        int ret;
        prelude_string_t *str;

        ret = prelude_string_new(&str);
        if ( ret < 0 )
                return ret;

        ret = prelude_string_vprintf(str, fmt, ap);
        if ( ret < 0 ) {
                prelude_string_destroy(str);
                return ret;
        }

        ret = _prelude_thread_set_error(prelude_string_get_string(str));
        prelude_string_destroy(str);

        if ( ret < 0 )
                return ret;

        ret = prelude_error_make(source, code);
        ret = -ret;
        ret |= (1 << PRELUDE_ERROR_VERBOSE_SHIFT);

        return -ret;
}



/**
 * prelude_error_verbose_make:
 * @source: Error source.
 * @code: Error code.
 * @fmt: Format string.
 * @...: Argument list.
 *
 * Create a new error using @source and @code, using the detailed error message
 * specified within @fmt.
 *
 * Returns: The created #prelude_error_t integer.
 */
prelude_error_t prelude_error_verbose_make(prelude_error_source_t source,
                                           prelude_error_code_t code, const char *fmt, ...)
{
        int ret;
        va_list ap;

        va_start(ap, fmt);
        ret = prelude_error_verbose_make_v(source, code, fmt, ap);
        va_end(ap);

        return ret;
}


/**
 * prelude_error_get_code:
 * @error: A #prelude_error_t return value.
 *
 * Returns: the #prelude_code_t code contained within the @prelude_error_t integer.
 */
prelude_error_code_t prelude_error_get_code(prelude_error_t error)
{
        error = -error;
        return (prelude_error_code_t) (error & PRELUDE_ERROR_CODE_MASK);
}


/**
 * prelude_error_get_source:
 * @error: A #prelude_error_t return value.
 *
 * Returns: the #prelude_source_t source contained within the @prelude_error_t integer.
 */
prelude_error_source_t prelude_error_get_source(prelude_error_t error)
{
        error = -error;
        return (prelude_error_source_t) ((error >> PRELUDE_ERROR_SOURCE_SHIFT) & PRELUDE_ERROR_SOURCE_MASK);
}


/**
 * prelude_error_is_verbose:
 * @error: A #prelude_error_t return value.
 *
 * Returns: #PRELUDE_BOOL_TRUE if there is a detailed message for this error, #PRELUDE_BOOL_FALSE otherwise.
 */
prelude_bool_t prelude_error_is_verbose(prelude_error_t error)
{
        error = -error;
        return ((error >> PRELUDE_ERROR_VERBOSE_SHIFT) & PRELUDE_ERROR_VERBOSE_MASK) ? PRELUDE_BOOL_TRUE : PRELUDE_BOOL_FALSE;
}


/**
 * prelude_error_code_from_errno:
 * @err: errno value.
 *
 * Returns: the #prelude_error_code_t value corresponding to @err.
 */
prelude_error_code_t prelude_error_code_from_errno(int err)
{
        int idx;

        if ( ! err )
                return PRELUDE_ERROR_NO_ERROR;

        idx = errno_to_idx(err);
        if ( idx < 0 )
                return PRELUDE_ERROR_UNKNOWN_ERRNO;

        return PRELUDE_ERROR_SYSTEM_ERROR | err_code_from_index[idx];
}


/**
 * prelude_error_code_to_errno:
 * @code: Error code.
 *
 * Returns: the errno value corresponding to @code.
 */
int prelude_error_code_to_errno(prelude_error_code_t code)
{
        if ( ! (code & PRELUDE_ERROR_SYSTEM_ERROR) )
                return 0;

        code &= ~PRELUDE_ERROR_SYSTEM_ERROR;

        if ( code < sizeof(err_code_to_errno) / sizeof(err_code_to_errno[0]) )
                return err_code_to_errno[code];
        else
                return 0;
}



/**
 * prelude_perror:
 * @error: A #prelude_error_t return value.
 * @fmt: Format string.
 * @...: Argument list.
 *
 * Print the error to stderr, or to syslog() in case stderr is unavailable.
 */
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
