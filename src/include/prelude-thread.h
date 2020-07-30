/*****
*
* Copyright (C) 2005-2020 CS GROUP - France. All Rights Reserved.
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

#ifndef _LIBPRELUDE_THREAD_H
#define _LIBPRELUDE_THREAD_H

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#ifdef __cplusplus
 extern "C" {
#endif

/*
 *
 */
int prelude_thread_init(void *nil);

int _prelude_thread_set_error(const char *error);

const char *_prelude_thread_get_error(void);

void _prelude_thread_deinit(void);
         
#ifdef __cplusplus
 }
#endif

#endif
