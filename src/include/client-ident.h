/*****
*
* Copyright (C) 2002 Yoann Vandoorselaere <yoann@prelude-ids.org>
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

#ifndef _LIBPRELUDE_CLIENT_IDENT_H
#define _LIBPRELUDE_CLIENT_IDENT_H

int prelude_client_ident_send(prelude_io_t *fd, int client_type);

int prelude_client_ident_init(const char *sname);

void prelude_client_set_analyzer_id(uint64_t id);

#endif /* _LIBPRELUDE_CLIENT_IDENT_H */
