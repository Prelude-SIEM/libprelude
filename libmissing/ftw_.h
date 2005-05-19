/* Copyright (C) 2005 Free Software Foundation, Inc.
 * Written by Yoann Vandoorselaere <yoann@prelude-ids.org>
 *
 * The file is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This file is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this file; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA.
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#if HAVE_FTW
# include <ftw.h>
#else

#define FTW_F   1
#define FTW_D   2
#define FTW_DNR 3
#define FTW_SL  4
#define FTW_NS  5

extern int ftw(const  char *dir, int (*fn)(const char *file, const struct stat *sb, int flag), int nopenfd);

#endif
