/*****
*
* Copyright (C) 2004-2020 CS GROUP - France. All Rights Reserved.
* Author: Yoann Vandoorselaere <yoann.v@prelude-ids.com>
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

/*
 * should be in $(top_srcdir)/libmissing, but since the Makefile.am
 * is generated dynamically by gnulib-tool, it can't go there.
 */

#ifndef _LIBPRELUDE_LIBMISSING_H
#define _LIBPRELUDE_LIBMISSING_H

#include "config.h"

#include "ftw_.h"
#include "getpass.h"
#include "minmax.h"
#include "pathmax.h"

/*
 * GNULib pathmax module does not cover system which have no limit on filename length (like GNU Hurd).
 */
#ifndef PATH_MAX
# define PATH_MAX 8192
#endif

#endif /* _LIBPRELUDE_LIBMISSING_H */
