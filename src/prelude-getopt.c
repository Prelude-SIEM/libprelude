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

#include "list.h"
#include "common.h"
#include "variable.h"
#include "config-engine.h"
#include "prelude-getopt.h"



typedef struct prelude_option {
        struct list_head list;
        
        char shortopt;
        const char *longopt;
        const char *description;
        prelude_option_argument_t has_arg;

        int called;
        void (*cb)(const char *optarg);
        
} prelude_option_t;



struct prelude_optlist {    
        int argv_index;
        struct list_head optlist;
};




/*
 * Search an option of a given name in the option list.
 */
static prelude_option_t *search_option(prelude_optlist_t *optlist, const char *name) 
{
        struct list_head *tmp;
        prelude_option_t *item;
        
        list_for_each(tmp, &optlist->optlist) {
                item = list_entry(tmp, prelude_option_t, list);
                
                if ( strcmp(item->longopt, name) == 0 || item->shortopt == *name )
                        return item;
        }

        return NULL;
}




static int handle_option_optarg(prelude_optlist_t *optlist,
                                int argc, const char **argv, const char **optarg) 
{
        if ( optlist->argv_index >= argc )
                *optarg = NULL;
        
        if ( *argv[optlist->argv_index] != '-' )
                *optarg = argv[optlist->argv_index++];

        return 0;
}



static int handle_option_reqarg(prelude_optlist_t *optlist, const char *option,
                                int argc, const char **argv, const char **optarg) 
{
        if ( optlist->argv_index >= argc || *argv[optlist->argv_index] == '-' ) {
                fprintf(stderr, "Option %s require an argument.\n", option);
                return -1;
        }

        if ( optlist->argv_index >= argc )
                *optarg = NULL;

        *optarg = argv[optlist->argv_index++];

        return 0;
}



static int handle_option_noarg(prelude_optlist_t *optlist, const char *option,
                               int argc, const char **argv)
{
        if ( optlist->argv_index < argc && *argv[optlist->argv_index] == '-' ) {
                fprintf(stderr, "Option %s do not take an argument.\n", option);
                return -1;
        }

        return 0;
}



static const char *lookup_variable_if_needed(const char *optarg) 
{
        /*
         * If optarg specify a variable, do the lookup.
         */
        if ( optarg && *optarg == '$' ) {
                const char *ptr;
                
                ptr = variable_get(optarg + 1);
                if ( ! ptr ) {
                        log(LOG_INFO, "couldn't lookup variable %s.\n", optarg + 1);
                        return NULL;
                }
                
                optarg = ptr;
        }

        return optarg;
}




static int handle_option(prelude_optlist_t *optlist, const char *option, int argc, const char **argv) 
{
        int ret = 0;
        const char *old = option;
        prelude_option_t *optitem;
        const char *optarg = NULL;
        
        while ( *option == '-' ) option++;

        optitem = search_option(optlist, option);
        if ( ! optitem ) {
                fprintf(stderr, "Invalid option : \"%s\".\n", old);
                return -1;
        }

        switch (optitem->has_arg) {
                
        case optionnal_argument:
                ret = handle_option_optarg(optlist, argc, argv, &optarg);
                break;
                
        case required_argument:
                ret = handle_option_reqarg(optlist, old, argc, argv, &optarg);
                break;

        case no_argument:
                ret = handle_option_noarg(optlist, old, argc, argv);
                break;
        }

        if ( ret < 0 )
                return -1;
        
        optarg = lookup_variable_if_needed(optarg);
        if ( ! optarg && optitem->has_arg == require_argument)
                return -1;

        optitem->called = 1;
        optitem->cb(optarg);
        
        return 0;
}



/*
 * Try to get all option that were not set from the command line in the config file.
 */
static void get_missing_options(const char *filename, prelude_optlist_t *optlist) 
{
        const char *str;
        config_t *cfg = NULL;
        struct list_head *tmp;
        prelude_option_t *optitem;
        
        list_for_each(tmp, &optlist->optlist) {

                optitem = list_entry(tmp, prelude_option_t, list);
                
                if ( optitem->called )
                        continue;
                
                if ( ! cfg && ! (cfg = config_open(filename)) ) {
                        log(LOG_ERR, "couldn't open %s.\n", filename);
                        return;
                }
                
                str = config_get(cfg, NULL, optitem->longopt);
                if ( str ) 
                        optitem->cb(str);
        }

        if ( cfg )
                config_close(cfg);
}



/**
 * prelude_option_parse_arguments:
 * @optlist: A pointer on an option list.
 * @argc: Number of argument.
 * @argv: Argument list.
 *
 * prelude_option_parse_arguments(), parse the given argument and try to
 * match them against option in @optlist. If an option match, it's associated
 * callback function is called with the eventual option argument if any.
 *
 * Returns: the short option character on success, -1 if an error occured.
 */
int prelude_option_parse_arguments(prelude_optlist_t *optlist,
                                   const char *filename, int argc, const char **argv) 
{
        int ret;
        const char *arg;
        
        while ( optlist->argv_index < argc ) {

                arg = argv[optlist->argv_index++];
                if ( *arg != '-' ) {
                        fprintf(stderr, "Invalid argument : \"%s\".\n", arg);
                        continue;
                }
                
                ret = handle_option(optlist, arg, argc, argv);
                if ( ret == 0 ) 
                        argv[optlist->argv_index - 1] = "";
        }

        get_missing_options(filename, optlist);
        optlist->argv_index = 1;
        
        return 0;
}




/**
 * prelude_option_add:
 * @optlist: Pointer on a #prelude_optlist_t object.
 * @type: Define if the option is local to the program or available to a Manager admin console.
 * @shortopt: Short option name.
 * @longopt: Long option name.
 * @desc: Description of the option.
 * @has_arg: Define if the option has argument.
 * @cb: Callback to be called when the value for this option change.
 *
 * prelude_option_add() create a new option and add it to the option list @optlist.
 *
 * Returns: 0 on success, -1 if an error occured.
 */
int prelude_option_add(prelude_optlist_t *optlist,
                       char shortopt, const char *longopt, const char *desc,
                       prelude_option_argument_t has_arg, void (*cb)(const char *optarg)) 
{
        prelude_option_t *new;

        new = malloc(sizeof(*new));
        if ( ! new ) 
                return -1;
        
        new->has_arg = has_arg;
        new->longopt = longopt;
        new->shortopt = shortopt;
        new->description = desc;
        new->cb = cb;
        new->called = 0;
        
        list_add(&new->list, &optlist->optlist);

        return 0;
}



void prelude_option_print(prelude_optlist_t *optlist) 
{
        int ret;
        prelude_option_t *opt;
        struct list_head *tmp;

        list_for_each(tmp, &optlist->optlist) {

                opt = list_entry(tmp, prelude_option_t, list);

                ret = printf("    -%c --%s ", opt->shortopt, opt->longopt);                
                while ( ret++ < 40 )
                        putchar(' ');

                printf("%s\n", opt->description);
        }
}



/**
 * prelude_option_new:
 *
 * Create a new list of option.
 *
 * Returns: A pointer on a #prelude_optlist_t object,
 * or NULL if an error occured.
 */
prelude_optlist_t *prelude_option_new(void) 
{
        prelude_optlist_t *list;

        list = malloc(sizeof(*list));
        if ( ! list )
                return NULL;

        list->argv_index = 1;
        INIT_LIST_HEAD(&list->optlist);

        return list;
}




void prelude_option_destroy(prelude_optlist_t *optlist) 
{
        prelude_option_t *opt;
        struct list_head *tmp;

        for ( tmp = optlist->optlist.next; tmp != &optlist->optlist; ) {
                
                opt = list_entry(tmp, prelude_option_t, list);
                tmp = tmp->next;

                list_del(&opt->list);
                free(opt);
        }
}








