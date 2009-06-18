#include <prelude.h>

#include <prelude-log.hxx>


using namespace Prelude;

void PreludeLog::SetLevel(int level)
{
        prelude_log_set_level((prelude_log_t) level);
}


void PreludeLog::SetDebugLevel(int level)
{
        prelude_log_set_debug_level(level);
}


void PreludeLog::SetFlags(int flags)
{
        prelude_log_set_flags((prelude_log_flags_t) flags);
}


int PreludeLog::GetFlags()
{
        return prelude_log_get_flags();
}


void PreludeLog::SetLogfile(const char *filename)
{
        prelude_log_set_logfile(filename);
}


void PreludeLog::SetCallback(void (*log_cb)(int level, const char *log))
{

        prelude_log_set_callback((void (*)(prelude_log_t level, const char *log)) log_cb);
}
