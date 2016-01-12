/*****
*
* Copyright (C) 2014-2016 CS-SI. All Rights Reserved.
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

#ifndef _LIBPRELUDE_PRELUDE_MACROS_H
#define _LIBPRELUDE_PRELUDE_MACROS_H

#define PRELUDE_CHECK_GNUC_VERSION(major, minor) \
     (__GNUC__ > (major) || (__GNUC__ == (major) && __GNUC_MINOR__ >= (minor)))

#if PRELUDE_CHECK_GNUC_VERSION(3, 1) && ! __STRICT_ANSI__
# define PRELUDE_DEPRECATED __attribute__((__deprecated__))
#else
# define PRELUDE_DEPRECATED
#endif

#if ! PRELUDE_CHECK_GNUC_VERSION(2, 5) || __STRICT_ANSI__
# define PRELUDE_FMT_SCANF(format_idx, arg_idx)
# define PRELUDE_FMT_PRINTF(format_idx, arg_idx)
#else

/* The __-protected variants of `format' and `printf' attributes
 * are accepted by gcc versions 2.6.4 (effectively 2.7) and later.  */
# if ! PRELUDE_CHECK_GNUC_VERSION(2, 7)
#  define __scanf__ scanf
#  define __printf__ printf
#  define __format__ format
# endif

# define PRELUDE_FMT_SCANF(format_idx, arg_idx) \
         __attribute__((__format__(__scanf__, format_idx, arg_idx)))
# define PRELUDE_FMT_PRINTF(format_idx, arg_idx) \
         __attribute__((__format__(__printf__, format_idx, arg_idx)))
#endif

#endif /* _LIBPRELUDE_PRELUDE_MACROS */
