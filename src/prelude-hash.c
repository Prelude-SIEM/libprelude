/*****
*
* Copyright (C) 2003,2004,2005 PreludeIDS Technologies. All Rights Reserved.
* Author: Nicolas Delon <nicolas.delon@prelude-ids.com>
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

#include "config.h"

#include <string.h>
#include <stdlib.h>

#include "prelude-log.h"
#include "prelude-list.h"
#include "prelude-error.h"
#include "prelude-hash.h"


#define HASH_DEFAULT_SIZE       128



typedef struct hash_elem {
        prelude_list_t list;
        void *key;
        void *value;
} hash_elem_t;


struct prelude_hash {
        size_t lists_size;
        prelude_list_t *lists;

        unsigned int (*hash_func)(const void *key);
        int (*key_cmp_func)(const void *key1, const void *key2);
        void (*key_destroy_func)(void *key);
        void (*value_destroy_func)(void *value);
};



/*
 * This function's code was taken from glib
 */
static unsigned int default_hash_func(const void *key)
{
        const char *ptr = key;
        unsigned int hv = *ptr;

        if ( hv )
                for ( ptr += 1; *ptr; ptr++ )
                        hv = (hv << 5) - hv + *ptr;

        return hv;

}



static int default_key_cmp_func(const void *key1, const void *key2)
{
        return strcmp((const char *) key1, (const char *) key2);
}



static unsigned int hash_value_calc(prelude_hash_t *hash, const void *key)
{
        return (hash->hash_func(key) % hash->lists_size);
}



static hash_elem_t *hash_elem_get(prelude_hash_t *hash, const void *key)
{
        prelude_list_t *list;
        prelude_list_t *ptr;
        hash_elem_t *hash_elem;

        list = hash->lists + hash_value_calc(hash, key);

        prelude_list_for_each(list, ptr) {
                hash_elem = prelude_list_entry(ptr, hash_elem_t, list);
                if ( hash->key_cmp_func(key, hash_elem->key) == 0 )
                        return hash_elem;
        }

        return NULL;
}



static  void hash_elem_key_destroy(prelude_hash_t *hash, hash_elem_t *hash_elem)
{
        if ( hash->key_destroy_func )
                hash->key_destroy_func(hash_elem->key);
}



static void hash_elem_value_destroy(prelude_hash_t *hash, hash_elem_t *hash_elem)
{
        if ( hash->value_destroy_func )
                hash->value_destroy_func(hash_elem->value);
}



int prelude_hash_new(prelude_hash_t **nhash,
                     unsigned int (*hash_func)(const void *),
                     int (*key_cmp_func)(const void *, const void *),
                     void (*key_destroy_func)(void *),
                     void (*value_destroy_func)(void *))
{
        return prelude_hash_new2(nhash, HASH_DEFAULT_SIZE, hash_func, key_cmp_func, key_destroy_func, value_destroy_func);
}



int prelude_hash_new2(prelude_hash_t **nhash, size_t size,
                      unsigned int (*hash_func)(const void *),
                      int (*key_cmp_func)(const void *, const void *),
                      void (*key_destroy_func)(void *),
                      void (*value_destroy_func)(void *))
{
        size_t i;
        prelude_hash_t *hash;

        *nhash = hash = calloc(1, sizeof (*hash));
        if ( ! hash )
                return prelude_error_from_errno(errno);

        hash->lists_size = size;

        hash->lists = malloc(hash->lists_size * sizeof(*hash->lists));
        if ( ! hash->lists ) {
                free(hash);
                return prelude_error_from_errno(errno);
        }

        hash->hash_func = hash_func ? hash_func : default_hash_func;
        hash->key_cmp_func = key_cmp_func ? key_cmp_func : default_key_cmp_func;
        hash->key_destroy_func = key_destroy_func;
        hash->value_destroy_func = value_destroy_func;

        for ( i = 0; i < hash->lists_size; i++ )
                prelude_list_init(hash->lists + i);

        return 0;
}



void prelude_hash_destroy(prelude_hash_t *hash)
{
        size_t cnt;
        prelude_list_t *list;
        prelude_list_t *ptr;
        prelude_list_t *tmp;
        hash_elem_t *hash_elem;

        for ( cnt = 0; cnt < hash->lists_size; cnt++ ) {
                list = hash->lists + cnt;

                prelude_list_for_each_safe(list, ptr, tmp) {
                        hash_elem = prelude_list_entry(ptr, hash_elem_t, list);

                        hash_elem_key_destroy(hash, hash_elem);
                        hash_elem_value_destroy(hash, hash_elem);
                        prelude_list_del(&hash_elem->list);
                        free(hash_elem);
                }
        }

        free(hash->lists);
        free(hash);
}



int prelude_hash_set(prelude_hash_t *hash, void *key, void *value)
{
        hash_elem_t *hash_elem;
        prelude_list_t *list;

        hash_elem = hash_elem_get(hash, key);

        if ( hash_elem ) {
                hash_elem_key_destroy(hash, hash_elem);
                hash_elem_value_destroy(hash, hash_elem);
                hash_elem->key = key;
                hash_elem->value = value;
                return 0;
        }

        hash_elem = calloc(1, sizeof(*hash_elem));
        if ( ! hash_elem )
                return prelude_error_from_errno(errno);

        hash_elem->key = key;
        hash_elem->value = value;

        list = hash->lists + hash_value_calc(hash, key);
        prelude_list_add(list, &hash_elem->list);

        return 1;
}



void *prelude_hash_get(prelude_hash_t *hash, const void *key)
{
        hash_elem_t *hash_elem;

        return (hash_elem = hash_elem_get(hash, key)) ? hash_elem->value : NULL;
}



int prelude_hash_elem_destroy(prelude_hash_t *hash, const void *key)
{
        hash_elem_t *hash_elem;

        hash_elem = hash_elem_get(hash, key);

        if ( ! hash_elem )
                return -1;

        hash_elem_key_destroy(hash, hash_elem);
        hash_elem_value_destroy(hash, hash_elem);
        prelude_list_del(&hash_elem->list);
        free(hash_elem);

        return 0;
}



void prelude_hash_iterate(prelude_hash_t *hash, void (*cb)(void *data))
{
        unsigned int i;
        prelude_list_t *tmp;
        hash_elem_t *hash_elem;

        for ( i = 0; i < hash->lists_size; i++ ) {
                prelude_list_for_each(&hash->lists[i], tmp) {
                        hash_elem = prelude_list_entry(tmp, hash_elem_t, list);
                        cb(hash_elem->value);
                }
        }
}
