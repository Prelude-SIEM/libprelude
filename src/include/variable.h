/*****
*
* Copyright (C) 2001, 2002, 2003, 2004 PreludeIDS Technologies. All Rights Reserved.
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
* Written by Yoann Vandoorselaere <yoann@prelude-ids.org>
*
*****/

#ifndef _LIBPRELUDE_RULES_VARIABLE_H
#define _LIBPRELUDE_RULES_VARIABLE_H

int variable_set(const char *variable, const char *value);
int variable_unset(const char *variable);
char *variable_get(const char *variable);
void variable_unset_all(void);

#endif /* _LIBPRELUDE_RULES_VARIABLE_H */
