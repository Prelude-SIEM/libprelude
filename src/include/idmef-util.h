/*****
*
* Copyright (C) 2002, 2003, 2004 Yoann Vandoorselaere <yoann@mandrakesoft.com>
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

#ifndef _LIBPRELUDE_IDMEF_UTIL_H
#define _LIBPRELUDE_IDMEF_UTIL_H

/*
 * call func with buffer and size depending of offset
 * handle error case and update offset
 */

#define MY_CONCAT(func, res, buffer, size, offset)		\
do {								\
	int __retval;						\
								\
	__retval = func(res, buffer + offset, size - offset);	\
	if ( __retval < 0 || __retval >= size - offset )	\
		return -1;					\
								\
	offset += __retval;					\
} while ( 0 )



/*
 * call snprintf depending on offset and update offset
 */

#define MY_SNPRINTF(buffer, size, offset, ...)			\
do {									\
	int __retval;							\
									\
	__retval = snprintf(buffer + offset, size - offset, __VA_ARGS__);	\
	if ( __retval < 0 || __retval >= size - offset )		\
		return -1;						\
									\
	offset += __retval;						\
} while ( 0 )

#endif /* _LIBPRELUDE_IDMEF_UTIL_H */
