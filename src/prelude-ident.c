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
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <inttypes.h>

#include "common.h"
#include "prelude-ident.h"


struct prelude_ident {
        int fd;
        uint64_t *ident;       
};
        



static int setup_filedes_if_needed(prelude_ident_t *new) 
{
        int ret;
        struct stat st;
        uint64_t value = 0;
        
        ret = fstat(new->fd, &st);
        if ( ret < 0 ) {
                log(LOG_ERR, "couldn't stat FD %d.\n", new->fd);
                return -1;
        }

        /*
         * our IDENT file is the good size.
         * No need to initialize it.
         */
        if ( st.st_size == sizeof(*new->ident) )
                return 0;

        /*
         * Need to write a default value, to set the file size.
         * (unless we want a SIGBUS when writing to *new->ident).
         */
        ret = write(new->fd, &value, sizeof(value));
        if ( ret < 0 ) {
                log(LOG_ERR, "couldn't write %d bytes to ident fd.\n", sizeof(value));
                return -1;
        }
        
        return 0;
}




/**
 * prelude_ident_new:
 * @filename: Pointer to a filename where the ident should be stored.
 *
 * Create a new #prelude_ident_t object. The current ident is set to 0
 * if there was no ident associated with this file, or the current ident.
 *
 * Returns: a new #prelude_ident_t object, or NULL if an error occured.
 */
prelude_ident_t *prelude_ident_new(const char *filename) 
{
        int ret;
        prelude_ident_t *new;

        new = malloc(sizeof(*new));
        if ( ! new ) {
                log(LOG_ERR, "memory exhausted.\n");
                return NULL;
        }
        
        new->fd = open(filename, O_CREAT|O_RDWR, S_IRUSR|S_IWUSR);
        if ( new->fd < 0 ) {
                log(LOG_ERR, "couldn't open %s.\n", filename);
                free(new);
                return NULL;
        }

        ret = setup_filedes_if_needed(new);
        if ( ret < 0 ) {
                close(new->fd);
                free(new);
                return NULL;
        }
        
        new->ident = mmap(0, sizeof(*new->ident), PROT_READ|PROT_WRITE, MAP_SHARED, new->fd, 0);
        if ( ! new->ident ) {
                log(LOG_ERR, "mmap failed.\n");
                close(new->fd);
                free(new);
                return NULL;
        }
        
        return new;
}




/**
 * prelude_ident_inc:
 * @ident: Pointer to a #prelude_ident_t object.
 *
 * Increment the ident associated with the #prelude_ident_t object.
 *
 * Returns: the new ident.
 */
uint64_t prelude_ident_inc(prelude_ident_t *ident) 
{
        return ++(*ident->ident);
}



/**
 * prelude_ident_dec:
 * @ident: Pointer to a #prelude_ident_t object.
 *
 * Decrement the ident associated with the #prelude_ident_t object.
 *
 * Returns: the new ident.
 */
uint64_t prelude_ident_dec(prelude_ident_t *ident) 
{
        return --(*ident->ident);
}



/**
 * prelude_ident_destroy:
 * @ident: Pointer to a #prelude_ident_t object.
 *
 * Destroy a #prelude_ident_t object.
 */
void prelude_ident_destroy(prelude_ident_t *ident) 
{
        int ret;
        
        ret = munmap(ident->ident, sizeof(*ident->ident));
        if ( ret < 0 ) {
                log(LOG_ERR, "couldn't unmap ident %p\n", ident->ident);
                return;
        }
        
        ret = close(ident->fd);
        if ( ret < 0 ) {
                log(LOG_ERR, "couldn't close ident fd %d\n", ident->fd);
                return;
        }
        
        free(ident);
}



