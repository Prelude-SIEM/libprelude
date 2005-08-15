/* Copyright (C) 2005 Free Software Foundation, Inc.
   Written by Yoann Vandoorselaere <yoann@prelude-ids.org>

   This program is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by the
   Free Software Foundation; either version 2, or (at your option) any
   later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software Foundation,
   Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.  */


#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdio.h>
#include <string.h>

/* Needed for strncasecmp */
#include "strcase.h"


char *
strcasestr (const char *haystack, const char *needle)
{
  int ret;
  size_t len = strlen (needle);

  /*
   * Hack to convert from const without warning.
   */
  union
  {
    const char *ro;
    char *rw;
  } conv;
  conv.ro = haystack;

  while (*conv.ro)
    {

      ret = strncasecmp (conv.ro, needle, len);
      if (ret == 0)
	return conv.rw;

      conv.ro++;
    }

  return NULL;
}
