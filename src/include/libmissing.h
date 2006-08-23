/*****
*
* Copyright (C) 2004, 2005 PreludeIDS Technologies. All Rights Reserved.
* Author: Yoann Vandoorselaere <yoann.v@prelude-ids.com>
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
* You should have received a copy of the GNU General Public License
* along with this program; see the file COPYING.  If not, write to
* the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
*
*****/

/*
 * should be in $(top_srcdir)/libmissing, but since the Makefile.am
 * is generated dynamically by gnulib-tool, it can't go there.
 */

#ifndef _LIBPRELUDE_LIBMISSING_H
#define _LIBPRELUDE_LIBMISSING_H

#include "config.h"

#include <alloca.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <time.h>
#include "ftw_.h"
#include "getaddrinfo.h"
#include "gettext.h"
#include "inet_ntop.h"
#include "minmax.h"
#include "pathmax.h"
#include "regex.h"
#include "size_max.h"
#include "snprintf.h"
#include "strcase.h"
#include "strcasestr.h"
#include "strdup.h"
#include "strndup.h"
#include "strnlen.h"
#include "strnlen1.h"
#include "strpbrk.h"
#include "strsep.h"
#include "time_r.h"
#include "timegm.h"
#include "vasnprintf.h"
#include "vsnprintf.h"
#include "wcwidth.h"
#include "xsize.h"
#if HAVE_WCHAR_H
# include "mbchar.h"
#endif
#if HAVE_MBRTOWC
# include "mbuiter.h"
#endif


#endif /* _LIBPRELUDE_LIBMISSING_H */
