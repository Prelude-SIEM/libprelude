/*****
*
* Copyright (C) 2001 Yoann Vandoorselaere <yoann@prelude-ids.org>
* All Rights Reserved
*
* This file is part of the Prelude program.
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

#ifndef _LIBPRELUDE_PRELUDE_IDENT_H
#define _LIBPRELUDE_PRELUDE_IDENT_H

#include <prelude-inttypes.h>

typedef struct prelude_ident prelude_ident_t;

uint64_t prelude_ident_inc(prelude_ident_t *ident);

uint64_t prelude_ident_dec(prelude_ident_t *ident);

void prelude_ident_destroy(prelude_ident_t *ident);

prelude_ident_t *prelude_ident_new(void);

#endif /* _LIBPRELUDE_PRELUDE_IDENT_H */
