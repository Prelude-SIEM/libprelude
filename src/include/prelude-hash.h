/*****
*
* Copyright (C) 2003 Nicolas Delon <delon.nicolas@wanadoo.fr>
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

#ifndef _LIBPRELUDE_HASH_H
#define _LIBPRELUDE_HASH_H

typedef struct prelude_hash prelude_hash_t;

int prelude_hash_new(prelude_hash_t **hash,
                     unsigned int (*hash_func)(const void *),
                     int (*key_cmp_func)(const void *, const void *),
                     void (*key_destroy_func)(void *),
                     void (*value_destroy_func)(void *));

void prelude_hash_destroy(prelude_hash_t *hash);

int prelude_hash_set(prelude_hash_t *hash, void *key, void *value);

void *prelude_hash_get(prelude_hash_t *hash, const void *key);

int prelude_hash_elem_destroy(prelude_hash_t *hash, const void *key);

#endif /* _LIBPRELUDE_HASH_H */
