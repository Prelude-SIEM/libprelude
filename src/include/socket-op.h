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

typedef ssize_t (read_func_t)(int fd, void *buf, size_t count);
typedef ssize_t (write_func_t)(int fd, const void *buf, const size_t count);

ssize_t socket_read_nowait(int fd, void *buf, size_t count, read_func_t *myread);
ssize_t socket_read(int fd, void *buf, size_t count, read_func_t *myread);
ssize_t socket_write(int fd, void *buf, size_t count, write_func_t *mywrite);

ssize_t socket_read_delimited(int sock, void **buf, read_func_t *myread);
ssize_t socket_write_delimited(int sock, char *buf, size_t count, write_func_t *mywrite);
