/*
 *  Copyright (C) 2000 Yoann Vandoorselaere.
 *
 *  This program is free software; you can redistribute it and/or modify 
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *  Authors: Yoann Vandoorselaere <yoann@mandrakesoft.com>
 *
 */
#ifndef COMPAT_H
#define COMPAT_H

#ifnef PACKAGE
 #error "compat.h require <libprelude/config.h> in order to compile"
#endif

#ifndef HAVE_GETOPT_LONG

#define no_argument 0
#define optional_argument 1
#define required_argument 2

struct option {
        const char *name;
        int has_arg;
        int *flag;
        int val;
};

int getopt_long(int argc, char * const argv[],
                const char *optstring,
                const struct option *longopts, int *longindex);

#else
#include <getopt.h>
#endif


#endif
