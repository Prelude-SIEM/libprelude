/*****
*
* Copyright (C) 2001, 2002, 2003, 2004, 2005 PreludeIDS Technologies. All Rights Reserved.
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
*****/

#ifndef _LIBPRELUDE_PRELUDE_IO_H
#define _LIBPRELUDE_PRELUDE_IO_H

#ifdef __cplusplus
  extern "C" {
#endif

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdio.h>
#include <unistd.h>
#include "prelude-inttypes.h"


typedef struct prelude_io prelude_io_t;

/*
 * Object creation / destruction functions.
 */
int prelude_io_new(prelude_io_t **ret);

void prelude_io_destroy(prelude_io_t *pio);

void prelude_io_set_file_io(prelude_io_t *pio, FILE *fd);

void prelude_io_set_tls_io(prelude_io_t *pio, void *tls);

void prelude_io_set_sys_io(prelude_io_t *pio, int fd);

int prelude_io_set_buffer_io(prelude_io_t *pio);


/*
 *
 */
void prelude_io_set_fdptr(prelude_io_t *pio, void *ptr);
void prelude_io_set_write_callback(prelude_io_t *pio, ssize_t (*write)(prelude_io_t *io, const void *buf, size_t count));
void prelude_io_set_read_callback(prelude_io_t *pio, ssize_t (*read)(prelude_io_t *io, void *buf, size_t count));
void prelude_io_set_pending_callback(prelude_io_t *pio, ssize_t (*pending)(prelude_io_t *io));


/*
 * IO operations.
 */
int prelude_io_close(prelude_io_t *pio);

ssize_t prelude_io_read(prelude_io_t *pio, void *buf, size_t count);

ssize_t prelude_io_read_wait(prelude_io_t *pio, void *buf, size_t count);

ssize_t prelude_io_read_delimited(prelude_io_t *pio, unsigned char **buf);


ssize_t prelude_io_write(prelude_io_t *pio, const void *buf, size_t count);

ssize_t prelude_io_write_delimited(prelude_io_t *pio, const void *buf, uint16_t count);


ssize_t prelude_io_forward(prelude_io_t *dst, prelude_io_t *src, size_t count);

int prelude_io_get_fd(prelude_io_t *pio);

void *prelude_io_get_fdptr(prelude_io_t *pio);

ssize_t prelude_io_pending(prelude_io_t *pio);

prelude_bool_t prelude_io_is_error_fatal(prelude_io_t *pio, int error);

#ifdef __cplusplus
  }
#endif

#endif /* _LIBPRELUDE_PRELUDE_IO_H */
