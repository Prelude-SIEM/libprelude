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



struct prelude_optlist {    
        int argv_index;
        struct list_head optlist;
};


#define PRELUDE_OPTION_GENERIC             \
        prelude_optlist_t optlist;         \
        struct list_head list;             \
                                           \
        int flags;                         \
        char shortopt;                     \
        const char *longopt;               \
        const char *description;           \
        prelude_option_argument_t has_arg; \
                                           \
        int cb_called;                     \
        int (*set)(const char *optarg)
        

struct prelude_option {
        PRELUDE_OPTION_GENERIC;        
};



struct prelude_option_wide {
        PRELUDE_OPTION_GENERIC;
        const char *help;
        const char *input_validation_rexex;
        enum { string, integer, boolean } input_type;
        int (*get)(char *ibuf, size_t size);
};




/*
 * Search an option of a given name in the given option list.
 */
static prelude_option_t *search_cli_option(prelude_optlist_t *optlist, const char *optname) 
{
        struct list_head *tmp;
        prelude_option_t *item;

        list_for_each(tmp, &optlist->optlist) {
                item = list_entry(tmp, prelude_option_t, list);

                if ( ! (item->flags & CLI_HOOK) )
                        continue;
                
                if ( strcmp(item->longopt, optname) == 0 || item->shortopt == *optname )
                        return item;
        }

        return NULL;
}




static int check_option_optarg(prelude_optlist_t *optlist,
                               int argc, const char **argv, const char **optarg) 
{
        if ( optlist->argv_index >= argc )
                *optarg = NULL;
        
        if ( *argv[optlist->argv_index] != '-' )
                *optarg = argv[optlist->argv_index++];
        
        return 0;
}




static int check_option_reqarg(prelude_optlist_t *optlist, const char *option,
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




static int check_option_noarg(prelude_optlist_t *optlist, const char *option,
                              int argc, const char **argv)
{        
        if ( optlist->argv_index < argc && *argv[optlist->argv_index] != '-' ) {
                fprintf(stderr, "Option %s do not take an argument.\n", option);
                return -1;
        }
                
        return 0;
}




static int check_option(prelude_optlist_t *optlist, prelude_option_t *option,
                        const char **optarg, int argc, const char **argv) 
{
        int ret;
        
        switch (option->has_arg) {
                
        case optionnal_argument:
                ret = check_option_optarg(optlist, argc, argv, optarg);
                break;
                
        case required_argument:
                ret = check_option_reqarg(optlist, option->longopt, argc, argv, optarg);
                break;

        case no_argument:
                ret = check_option_noarg(optlist, option->longopt, argc, argv);
                break;

        default:
                log(LOG_ERR, "Unknow value for has_arg parameter.\n");
                return -1;
        }

        return ret;
}





static int lookup_variable_if_needed(const char **optarg) 
{
        /*
         * This is not a variable.
         */
        if ( ! *optarg || **optarg != '$' )
                return 0;
                
        *optarg = variable_get(*optarg + 1);
        if ( ! *optarg ) {
                log(LOG_INFO, "couldn't lookup variable %s.\n", *optarg + 1);
                return -1;
        }
        
        return 0;
}




/*
 * Try to get all option that were not set from the command line in the config file.
 */
static int get_missing_options(const char *filename, prelude_optlist_t *optlist) 
{
        int ret;
        const char *str;
        config_t *cfg = NULL;
        struct list_head *tmp;
        prelude_option_t *optitem;
        
        list_for_each(tmp, &optlist->optlist) {

                optitem = list_entry(tmp, prelude_option_t, list);
                
                if ( optitem->cb_called || !(optitem->flags & CFG_HOOK) )
                        continue;
                
                if ( ! cfg && ! (cfg = config_open(filename)) ) {
                        log(LOG_ERR, "couldn't open %s.\n", filename);
                        return -1;
                }
                
                str = config_get(cfg, NULL, optitem->longopt);
                if ( str ) {
                        ret = optitem->set(str);
                        if ( ret < 0 )
                                return -1;
                }
        }

        if ( cfg )
                config_close(cfg);

        return 0;
}




static int parse_argument(prelude_optlist_t *optlist,
                          const char *filename, int argc, const char **argv)
{
        int ret;
        const char *arg, *old;
        prelude_option_t *opt;
        
        while ( optlist->argv_index < argc ) {
                
                old = arg = argv[optlist->argv_index++];                
                if ( *arg != '-' ) {
                        fprintf(stderr, "Invalid argument : \"%s\".\n", arg);
                        continue;
                }

                while ( *arg == '-' ) arg++;
                
                opt = search_cli_option(optlist, arg);                
                if ( ! opt ) {
                        /*
                         * Do not stop parsing, this can not be an error (for exemple
                         * if the option is handled by another part of the application.
                         */
                        log(LOG_INFO, "Invalid option : \"%s\".\n", old);
                } else {
                        const char *optarg = NULL;
                        
                        ret = check_option(optlist, opt, &optarg, argc, argv);
                        if ( ret < 0 ) 
                                return -1;
                        
                        ret = lookup_variable_if_needed(&optarg);
                        if ( ret < 0 )
                                return -1;
                        
                        opt->cb_called = 1;
                        
                        ret = opt->set(optarg);
                        if ( ret == prelude_option_end || ret == prelude_option_error )
                                return ret;
                        
                        /*
                         * If the option we just found have sub-option.
                         * Try to match the rest of our argument against them.
                         */
                        if ( ! list_empty(&opt->optlist.optlist) ) {
                                opt->optlist.argv_index = optlist->argv_index;
                                ret = parse_argument(&opt->optlist, filename, argc, argv);
                                optlist->argv_index = opt->optlist.argv_index;
                        }

                        if ( ret < 0 )
                                return -1;
                }
        }

        return 0;
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
 * Returns: 0 if parsing the option succeed (including the case where one of
 * the callback returned -1 to request interruption of parsing), -1 if an error occured.
 */
int prelude_option_parse_arguments(prelude_optlist_t *optlist,
                                   const char *filename, int argc, const char **argv) 
{
        int ret;
        
        ret = parse_argument(optlist, filename, argc, argv);        
        if ( ret < 0 )
                return -1;

        if ( ret == prelude_option_end )
                return 0;
        
        /*
         * Only try to get missing options from config file
         * if parsing arguments succeed and caller didn't requested us to stop.
         */
        get_missing_options(filename, optlist);

        /*
         * reset.
         */
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
 * Returns: Pointer on the option object, or NULL if an error occured.
 */
prelude_option_t *prelude_option_add(prelude_optlist_t *optlist, int flags, 
                                     char shortopt, const char *longopt, const char *desc,
                                     prelude_option_argument_t has_arg, int (*set)(const char *optarg)) 
{
        prelude_option_t *new;

        new = malloc(sizeof(*new));
        if ( ! new ) 
                return NULL;

        INIT_LIST_HEAD(&new->optlist.optlist);

        new->flags = flags;
        new->has_arg = has_arg;
        new->longopt = longopt;
        new->shortopt = shortopt;
        new->description = desc;
        new->set = set;
        new->cb_called = 0;
        
        list_add_tail(&new->list, &optlist->optlist);

        return new;
}




prelude_option_t *prelude_option_wide_add(prelude_optlist_t *optlist, int flags,
                                          char shortopt, const char *longopt, const char *desc,
                                          prelude_option_argument_t has_arg, int (*set)(const char *optarg),
                                          int (*get)(char *buf, size_t size)) 
{
        prelude_option_wide_t *new;

        new = malloc(sizeof(*new));
        if ( ! new ) 
                return NULL;

        INIT_LIST_HEAD(&new->optlist.optlist);

        new->flags = flags;
        new->has_arg = has_arg;
        new->longopt = longopt;
        new->shortopt = shortopt;
        new->description = desc;
        new->set = set;
        new->get = get;
        new->cb_called = 0;
        
        list_add_tail(&new->list, &optlist->optlist);

        return (prelude_option_t *) new;
}






static void print_options(prelude_optlist_t *optlist, int flags, int depth) 
{
        int ret, i;
        prelude_option_t *opt;
        struct list_head *tmp;

        list_for_each(tmp, &optlist->optlist) {

                opt = list_entry(tmp, prelude_option_t, list);

                /*
                 * If flags is not there, continue.
                 */
                if ( !(opt->flags & flags) )
                        continue;
                                
                for ( i = 0; i < depth; i++ )
                        printf("  ");
                
                ret = printf("    -%c --%s ", opt->shortopt, opt->longopt);                
                while ( ret++ < 40 )
                        putchar(' ');

                printf("%s\n", opt->description);

                if ( ! list_empty(&opt->optlist.optlist) ) 
                        print_options(&opt->optlist, flags, ++depth);
        }
}




/**
 * prelude_option_print_cli:
 * @optlist: Pointer on a #prelude_optlist_t object.
 * @flags: Only option with specified flags will be printed.
 *
 * Dump option available in optlist and hooked to the command line.
 */
void prelude_option_print(prelude_optlist_t *optlist, int flags) 
{
        print_options(optlist, flags, 0);
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




/**
 * prelude_option_destroy:
 * @optlist: Pointer on a list of option.
 *
 * Destroy a #prelude_optlist_t object and all data associated
 * with it.
 */
void prelude_option_destroy(prelude_optlist_t *optlist) 
{
        prelude_option_t *opt;
        struct list_head *tmp;

        for ( tmp = optlist->optlist.next; tmp != &optlist->optlist; ) {
                
                opt = list_entry(tmp, prelude_option_t, list);
                tmp = tmp->next;

                if ( ! list_empty(&opt->optlist.optlist) )
                        prelude_option_destroy(&opt->optlist);
                
                list_del(&opt->list);
                free(opt);
        }
}
