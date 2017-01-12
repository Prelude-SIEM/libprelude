/*****
*
* Copyright (C) 2009-2017 CS-SI. All Rights Reserved.
* Author: Yoann Vandoorselaere <yoannv@gmail.com>
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
