/*****
*
* Copyright (C) 2008-2012 CS-SI. All Rights Reserved.
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
* You should have received a copy of the GNU General Public License
* along with this program; see the file COPYING.  If not, write to
* the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
*
*****/

#include <stdio.h>

#ifndef _LIBPRELUDE_PRELUDE_HXX
#define _LIBPRELUDE_PRELUDE_HXX

#include "prelude-client.hxx"
#include "prelude-client-easy.hxx"
#include "prelude-connection.hxx"
#include "prelude-connection-pool.hxx"

#include "idmef.hxx"
#include "idmef-path.hxx"
#include "idmef-value.hxx"
#include "idmef-criteria.hxx"

const char *CheckVersion(const char *version = NULL);

#endif
