#include <prelude.h>

#include <prelude-log.hxx>


using namespace Prelude;

void PreludeLog::setLevel(int level)
{
        prelude_log_set_level((prelude_log_t) level);
}


void PreludeLog::setDebugLevel(int level)
{
        prelude_log_set_debug_level(level);
}


void PreludeLog::setFlags(int flags)
{
        prelude_log_set_flags((prelude_log_flags_t) flags);
}


int PreludeLog::getFlags()
{
        return prelude_log_get_flags();
}


void PreludeLog::setLogfile(const char *filename)
{
        prelude_log_set_logfile(filename);
}


void PreludeLog::setCallback(void (*log_cb)(int level, const char *log))
{

        prelude_log_set_callback((void (*)(prelude_log_t level, const char *log)) log_cb);
}
