/*****
*
* Copyright (C) 2003 Krzysztof Zaraska <kzaraska@student.uci.agh.edu.pl>
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

#ifndef _LIBPRELUDEDB_STRBUF_H
#define _LIBPRELUDEDB_STRBUF_H

typedef struct prelude_strbuf prelude_strbuf_t;

prelude_strbuf_t *prelude_strbuf_new(void);

int prelude_strbuf_sprintf(prelude_strbuf_t *s, const char *fmt, ...);

int prelude_strbuf_vprintf(prelude_strbuf_t *s, const char *fmt, va_list ap);

void prelude_strbuf_dont_own(prelude_strbuf_t *s);

size_t prelude_strbuf_get_len(prelude_strbuf_t *s);

char *prelude_strbuf_get_string(prelude_strbuf_t *s);

int prelude_strbuf_is_empty(prelude_strbuf_t *s);

void prelude_strbuf_clear(prelude_strbuf_t *s);

void prelude_strbuf_destroy(prelude_strbuf_t *s);

#endif /* _LIBPRELUDEDB_STRBUF_H */


