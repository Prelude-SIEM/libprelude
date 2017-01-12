/*****
*
* Copyright (C) 2009-2017 CS-SI. All Rights Reserved.
* Author: Yoann Vandoorselaere <yoann@prelude-ids.com>
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
* You should have received a copy of the GNU General Public License along
* with this program; if not, write to the Free Software Foundation, Inc.,
* 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*
*****/

#ifndef _LIBPRELUDE_PRELUDE_LOG_HXX
#define _LIBPRELUDE_PRELUDE_LOG_HXX

#include "prelude.h"
#include "idmef-path.h"

#if (defined _WIN32 || defined __WIN32__) && !defined __CYGWIN__
# undef ERROR
#endif

namespace Prelude {
        class PreludeLog {
            public:
                enum LogLevelEnum {
                        DEBUG    = PRELUDE_LOG_DEBUG,
                        INFO     = PRELUDE_LOG_INFO,
                        WARNING  = PRELUDE_LOG_WARN,
                        ERROR    = PRELUDE_LOG_ERR,
                        CRITICAL = PRELUDE_LOG_CRIT
                };

                enum LogFlagsEnum {
                        QUIET    = PRELUDE_LOG_FLAGS_QUIET,
                        SYSLOG   = PRELUDE_LOG_FLAGS_SYSLOG
                };

                static void setLevel(int level);
                static void setDebugLevel(int level);
                static void setFlags(int flags);
                static int getFlags(void);
                static void setLogfile(const char *filename);
                static void setCallback(void (*log_cb)(int level, const char *log));
        };
};

#endif
