/*****
*
* Copyright (C) 2001-2004 Yoann Vandoorselaere <yoann@prelude-ids.org>
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
#include <sys/time.h>

#include "prelude-log.h"
#include "prelude-error.h"
#include "prelude-inttypes.h"
#include "prelude-ident.h"


struct prelude_ident {
        volatile uint32_t no;
        uint32_t init_seconds;
};
        



/**
 * prelude_ident_new:
 * @ret: Pointer where to store the created object.
 *
 * Create a new #prelude_ident_t object with an unique value.
 *
 * Returns: 0 on success, or a negative value if an error occured.
 */
int prelude_ident_new(prelude_ident_t **ret)
{
        struct timeval tv;
        prelude_ident_t *new;

        gettimeofday(&tv, NULL);
        
        *ret = new = malloc(sizeof(*new));
        if ( ! new )
                return prelude_error_from_errno(errno);
        
        new->no = ~0;
        new->init_seconds = tv.tv_sec;
        
        return 0;
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
        if ( ident->no == (uint32_t) ~0 )
                ident->init_seconds++;
        
        ident->no++;
        
        return (uint64_t) ident->no << 32 | ident->init_seconds;
}




/**
 * prelude_ident_destroy:
 * @ident: Pointer to a #prelude_ident_t object.
 *
 * Destroy a #prelude_ident_t object.
 */
void prelude_ident_destroy(prelude_ident_t *ident) 
{       
        free(ident);
}



