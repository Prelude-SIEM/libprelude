/*****
*
* Copyright (C) 2001 Yoann Vandoorselaere <yoann@mandrakesoft.com>
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

#ifdef SWIG

%module prelude_io

typedef void FILE;
typedef int size_t;
typedef int ssize_t;
typedef unsigned short int uint16_t;

%{
	#include "prelude-io.h"
%}

#endif

typedef struct prelude_io prelude_io_t;

/*
 * Object creation / destruction functions.
 */
prelude_io_t *prelude_io_new(void);

void prelude_io_destroy(prelude_io_t *pio);

void prelude_io_set_file_io(prelude_io_t *pio, FILE *fd);

void prelude_io_set_ssl_io(prelude_io_t *pio, void *ssl);

void prelude_io_set_sys_io(prelude_io_t *pio, int fd);




/*
 * IO operations.
 */
int prelude_io_close(prelude_io_t *pio);

ssize_t prelude_io_read(prelude_io_t *pio, void *buf, size_t count);

ssize_t prelude_io_read_wait(prelude_io_t *pio, void *buf, size_t count);

ssize_t prelude_io_read_delimited(prelude_io_t *pio, void **buf);


ssize_t prelude_io_write(prelude_io_t *pio, const void *buf, size_t count);

int prelude_io_write_delimited(prelude_io_t *pio, const void *buf, uint16_t count);


ssize_t prelude_io_forward(prelude_io_t *dst, prelude_io_t *src, size_t count);

int prelude_io_get_fd(prelude_io_t *pio);

void *prelude_io_get_fdptr(prelude_io_t *pio);
