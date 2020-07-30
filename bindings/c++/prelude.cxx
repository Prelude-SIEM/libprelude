/*****
*
* Copyright (C) 2009-2020 CS GROUP - France. All Rights Reserved.
* Author: Yoann Vandoorselaere <yoann@prelude-ids.com>
*
* This file is part of the Prelude library.
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU Lesser General Public License as published by
* the Free Software Foundation; either version 2.1, or (at your option)
* any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU Lesser General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public License along
* with this program; if not, write to the Free Software Foundation, Inc.,
* 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*
*****/

#include <prelude.h>
#include "prelude.hxx"

const char *Prelude::checkVersion(const char *wanted)
{
        const char *ret;

        ret = prelude_check_version(wanted);
        if ( wanted && ! ret ) {
                std::string s = "libprelude ";
                s += wanted;
                s += " or higher is required (";
                s += prelude_check_version(NULL);
                s += " found).";
                throw PreludeError(s);
        }

        return ret;
}

