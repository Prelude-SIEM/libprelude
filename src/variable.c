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
* Written by Yoann Vandoorselaere <yoann@mandrakesoft.com>
*
*****/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "list.h"
#include "prelude-log.h"
#include "variable.h"


typedef struct {
        struct list_head list;
        char *variable;
        char *value;
} variable_t;



static LIST_HEAD(variable_list);



static variable_t *search_entry(const char *variable) 
{
        int ret;
        struct list_head *tmp;
        variable_t *item = NULL;
        
        list_for_each(tmp, &variable_list) {
                item = list_entry(tmp, variable_t, list);

                ret = strcasecmp(item->variable, variable);

                if ( ret == 0 )
                        return item;
        }

        return NULL;
}




static int create_entry(char *variable, char *value)
{
        variable_t *item;

        item = malloc(sizeof(*item));
        if ( ! item ) {
                log(LOG_ERR, "failed to allocate memory.\n");
                return -1;
        }

        item->variable = variable;
        item->value = value;

        list_add(&item->list, &variable_list);

        return 0;
}




/**
 * variable_get:
 * @variable: Variable to get the value from.
 *
 * Get value for the specified variable.
 *
 * Returns: Value of the variable, or NULL if the variable is not set.
 */
char *variable_get(const char *variable) 
{
        variable_t *item;

        item = search_entry(variable);
        
        return ( item ) ? item->value : NULL;
}




/**
 * variable_set:
 * @variable: The variable in question.
 * @value: Value to assign to the variable.
 *
 * Set the specified variable to the given value.
 * The variable is created if it doesn't exit.
 *
 * Returns: 0 on success, -1 on error.
 */
int variable_set(char *variable, char *value) 
{
        int ret = -1;
        variable_t *item;

        item = search_entry(variable);
        if ( ! item )
                ret = create_entry(variable, value);
        else
                item->value = value;

        return ( ret == 0 || item ) ? 0 : -1;
}




/**
 * variable_unset:
 * @variable: The variable in question.
 *
 * Delete the specified variable from the variable lists.
 *
 * Returns: 0 on success, -1 if variable could not be found.
 */
int variable_unset(const char *variable) 
{
        variable_t *item;

        item = search_entry(variable);
        if ( ! item )
                return -1;

        list_del(&item->list);
        
        free(item->variable);
        free(item->value);
        free(item);

        return 0;
}





