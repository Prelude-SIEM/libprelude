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
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <inttypes.h>
#include <sys/types.h>

#include "prelude-io.h"
#include "prelude-message.h"
#include "prelude-message-id.h"

#include "prelude-list.h"
#include "prelude-log.h"
#include "variable.h"
#include "config-engine.h"
#include "prelude-getopt.h"
#include "prelude-client.h"
#include "common.h"


#define option_run_all 10
#define PRELUDE_OPTION_INIT_FLAG WIDE_HOOK << 1
#define PRELUDE_OPTION_INIT_NAME "__prelude_option_init"


typedef struct prelude_optlist {    
        
        size_t wide_msglen;
        size_t wide_msgcount;
        prelude_msg_t *wide_msg;
        
        prelude_list_t optlist;
} prelude_optlist_t;



struct prelude_option {
        prelude_optlist_t optlist;
        prelude_list_t list;

        prelude_option_t *parent;
        
        int flags;
        int priority;
        char shortopt;
        const char *longopt;  
        const char *description;
        prelude_option_argument_t has_arg;
        
        int (*set)(void **context, prelude_option_t *opt, const char *optarg);
        int (*get)(void **context, char *ibuf, size_t size);
        
        const char *help;
        const char *input_validation_regex;
        enum { string, integer, boolean } input_type;

        void *private_data;
};



struct cb_list {
        prelude_list_t list;
        char *arg;
        void **context;
        prelude_option_t *option;
};



/*
 * Warning are turned on by default.
 */
static int warnings_flags = OPT_INVAL|OPT_INVAL_ARG;


/*
 * contain all option
 */
static prelude_optlist_t *root_optlist = NULL;


static void option_err(int flag, const char *fmt, ...) 
{
        if ( warnings_flags & flag || flag == 1 ) {
                va_list ap;
                
                va_start(ap, fmt);
                vfprintf(stderr, fmt, ap);
                va_end(ap);
        }
}




/*
 * Search an option of a given name in the given option list.
 */
static prelude_option_t *search_option(prelude_optlist_t *optlist,
                                       const char *optname, int flags, int walk_children) 
{
        prelude_option_t *item, *ret;
        prelude_list_t *tmp;
        
        prelude_list_for_each(tmp, &optlist->optlist) {
                item = prelude_list_entry(tmp, prelude_option_t, list);

                if ( walk_children ) {
                        ret = search_option(&item->optlist, optname, flags, walk_children);
                        if ( ret )
                                return ret;
                }

                if ( ! (item->flags & flags) )
                        continue;

                
                if ( item->longopt && strcasecmp(item->longopt, optname) == 0 )
                        return item;
                
                if ( strlen(optname) == 1 && item->shortopt == *optname )
                        return item;
        }
        
        return NULL;
}




static int is_an_argument(const char *stuff) 
{
        int len = strlen(stuff);
        
        if ( stuff[0] == '-' && (len == 2 || (len > 2 && stuff[1] == '-')) ) 
                return -1;
        
        return 0;
}



static int check_option_optarg(prelude_optlist_t *optlist, int argc,
                               char **argv, int *argv_index, char **optarg, size_t size) 
{
        size_t len = 0;
        
        while ( *argv_index < argc && is_an_argument(argv[*argv_index]) == 0 && len < size ) {

                if ( len > 0 )
                        (*optarg)[len++] = ' ';
                
                strncpy((*optarg) + len, argv[*argv_index], size - len);
                len += strlen(argv[*argv_index]);
                (*argv_index)++;
        }

        if ( len == 0 )
                *optarg = NULL;
        
        return 0;
}




static int check_option_reqarg(prelude_optlist_t *optlist, const char *option,
                               int argc, char **argv, int *argv_index, char **optarg, size_t size) 
{
        size_t len = 0;
        
        if ( *argv_index >= argc || is_an_argument(argv[*argv_index]) < 0 ) {
                fprintf(stderr, "Option %s require an argument.\n", option);
                return -1;
        }
        
        while ( *argv_index < argc && is_an_argument(argv[*argv_index]) == 0 && len < size ) {
                         
                if ( len > 0 )
                        (*optarg)[len++] = ' ';
                
                strncpy(*optarg + len, argv[*argv_index], size - len);
                len += strlen(argv[*argv_index]);
                (*argv_index)++;
        }

        if ( len == 0 )
                *optarg = NULL;
        
        return 0;
}




static int check_option_noarg(prelude_optlist_t *optlist, const char *option,
                              int argc, char **argv, int *argv_index)
{
        if ( *argv_index < argc && is_an_argument(argv[*argv_index]) == 0 ) {
                fprintf(stderr, "Option %s do not take an argument (%s).\n", option, argv[*argv_index]);
                return -1;
        }

        return 0;
}




static int check_option(prelude_optlist_t *optlist, prelude_option_t *option,
                        char **optarg, size_t size, int argc, char **argv, int *argv_index) 
{
        int ret;

        **optarg = '\0';
        
        switch (option->has_arg) {
                
        case optionnal_argument:
                ret = check_option_optarg(optlist, argc, argv, argv_index, optarg, size);
                break;
                
        case required_argument:
                ret = check_option_reqarg(optlist, option->longopt, argc, argv, argv_index, optarg, size);
                break;

        case no_argument:
                *optarg = NULL;
                ret = check_option_noarg(optlist, option->longopt, argc, argv, argv_index);
                break;

        default:
                log(LOG_ERR, "Unknow value for has_arg parameter.\n");
                return -1;
        }

        return ret;
}




static char *lookup_variable_if_needed(char *optarg) 
{
        const char *val;
        size_t i, j, len = 0;
        char out[1024], c;

        if ( ! optarg )
                return NULL;
                
        for ( i = 0; i <= strlen(optarg) && (len + 1) < sizeof(out); i++ ) {

                if ( optarg[i] != '$' ) {
                        out[len++] = optarg[i];
                        continue;
                }
                
                /*
                 * go to the end of the word.
                 */ 
                j = i;
                while ( optarg[i] != '\0' && optarg[i] != ' ' ) i++;
                
                /*
                 * split into token.
                 */
                c = optarg[i];
                optarg[i] = '\0';
                
                val = variable_get(optarg + j + 1);
                if ( ! val ) 
                        val = optarg + j;
                
                strncpy(&out[len], val, sizeof(out) - len);
                len += strlen(val);
                optarg[i--] = c;
        }
                
        return strdup(out);
}




static int call_option_cb(void **context, prelude_list_t *cblist, prelude_option_t *option, const char *arg) 
{
        struct cb_list *new, *cb;
        prelude_list_t *tmp, *prev = NULL;

        if ( ! option->set )
                return 0;
        
        if ( option->priority == option_run_first ) 
                return option->set(context, option, arg);
        
        new = malloc(sizeof(*new));
        if ( ! new ) {
                log(LOG_ERR, "memory exhausted.\n");
                return -1;
        }

        new->arg = (arg) ? strdup(arg) : NULL;
        new->option = option;
        new->context = context;
        
        if ( option->priority == option_run_last ) {
                prelude_list_add_tail(&new->list, cblist);
                return 0;
        }        
        
        prelude_list_for_each(tmp, cblist) {
                
                cb = prelude_list_entry(tmp, struct cb_list, list);
                
                if ( cb->option->priority < option->priority )
                        break;
                
                prev = tmp;
        }
        
        if ( ! prev )
                prev = cblist;
        
        prelude_list_add(&new->list, prev);

        return 0;
}




static int call_option_from_cb_list(prelude_list_t *cblist, int option_kind) 
{
        char *str;
        struct cb_list *cb;
        int ret = prelude_option_success;
        prelude_list_t *tmp, *bkp;
        
        prelude_list_for_each_safe(tmp, bkp, cblist) {
                
                cb = prelude_list_entry(tmp, struct cb_list, list);

                if ( option_kind != option_run_all && option_kind != cb->option->priority ) 
                        continue;

                ret = cb->option->set(cb->context, cb->option, (str = lookup_variable_if_needed(cb->arg)));
                
                if ( str )
                        free(str);

                if ( ret == prelude_option_error || ret == prelude_option_end )
                        return ret;

                if ( cb->arg )
                        free(cb->arg);
                
                prelude_list_del(&cb->list);
                free(cb);
        }

        return ret;
}



static int call_init_func(void **context, prelude_list_t *cb_list, prelude_option_t *parent)
{
        prelude_option_t *opt;

        if ( ! parent )
                return -1;

        opt = search_option(&parent->optlist, PRELUDE_OPTION_INIT_NAME, PRELUDE_OPTION_INIT_FLAG, 0);        
        if ( ! opt )
                return -1;

        return call_option_cb(context, cb_list, opt, NULL);
}



/*
 * Try to get all option that were not set from the command line in the config file.
 */
static int get_missing_options(void **context, const char *filename, prelude_optlist_t *rootlist) 
{
        int ret, line = 0;
        config_t *cfg = NULL;
        PRELUDE_LIST_HEAD(cb_list);
        prelude_optlist_t *optlist;
        prelude_option_t *opt, *parentopt = NULL;
        char *entry = NULL, *value = NULL, *section = NULL;
        
        cfg = config_open(filename);
        if ( ! cfg ) {
                log(LOG_INFO, "couldn't open %s.\n", filename);
                return -1;
        }

        optlist = rootlist;

        while ( config_get_next(cfg, &section, &entry, &value, &line) == 0 ) {
                
                if ( ! entry ) {
                        call_init_func(context, &cb_list, parentopt);
                        
                        opt = search_option(rootlist, section, CFG_HOOK, 1);
                        if ( ! opt ) {
                                option_err(OPT_INVAL, "%s:%d: invalid section : \"%s\".\n", filename, line, section);
                                continue;
                        }
                        
                        ret = call_option_cb(context, &cb_list, opt, value);
                        if ( ret == prelude_option_error || ret == prelude_option_end ) 
                                return ret;
                        
                        optlist = &opt->optlist;
                        parentopt = opt;
                }

                else {
                        opt = search_option(optlist, entry, CFG_HOOK, 0);
                        if ( ! opt ) {
                                option_err(OPT_INVAL, "%s:%d: invalid option \"%s\" in \"%s\" section.\n",
                                           filename, line, entry, (section) ? section : "global");
                                continue;
                        }
                        
                        ret = call_option_cb(context, &cb_list, opt, value);  
                        if ( ret == prelude_option_error || ret == prelude_option_end ) 
                                return ret;
                }
        }

        call_init_func(context, &cb_list, parentopt);
        ret = call_option_from_cb_list(&cb_list, option_run_all);
        config_close(cfg);
        
        return ret;
}




static int parse_argument(void **context, prelude_list_t *cb_list,
                          prelude_optlist_t *optlist, const char *filename,
                          int argc, char **argv, int *argv_index, int depth)
{
        int ret;
        prelude_option_t *opt;
        const char *arg, *old;
        char optarg[256], *argptr;
        
        while ( *argv_index < argc ) {

                old = arg = argv[(*argv_index)++];
                                            
                if ( *arg != '-' ) {                        
                        /*
                         * If arg == "", this is an argument we processed, and so nullified.
                         */
                        if ( arg != "" )
                                option_err(OPT_INVAL_ARG, "invalid argument : \"%s\".\n", arg);
                        
                        continue;
                }

                while ( *arg == '-' ) arg++;
                
                opt = search_option(optlist, arg, CLI_HOOK, 0);
                
                if ( ! opt ) {
                        if ( depth ) {
                                (*argv_index)--;
                                return 0;
                        }
                        
                        option_err(OPT_INVAL, "invalid option : \"%s\" (%d).\n", arg, depth);
                        
                        return 0;
                }

                argptr = optarg;
                                
                ret = check_option(optlist, opt, &argptr, sizeof(optarg), argc, argv, argv_index);
                if ( ret < 0 ) 
                        return -1;
                
                if ( opt->set ) {
                        
                        ret = call_option_cb(context, cb_list, opt, argptr);
                        if ( ret == prelude_option_end || ret == prelude_option_error )
                                return ret;        
                }
                
                /*
                 * If the option we just found have sub-option.
                 * Try to match the rest of our argument against them.
                 */
                if ( ! prelude_list_empty(&opt->optlist.optlist) ) {

                        ret = parse_argument(context, cb_list, &opt->optlist, filename, argc, argv, argv_index, depth + 1);
                        if ( ret == prelude_option_end || ret == prelude_option_error )
                                return ret;
                        
                        call_init_func(context, cb_list, opt);
                }
        }
        
        return 0;
}




static void reset_option(prelude_optlist_t *optlist) 
{
        prelude_option_t *opt;
        prelude_list_t *tmp;
        
        prelude_list_for_each(tmp, &optlist->optlist) {
                opt = prelude_list_entry(tmp, prelude_option_t, list);
                reset_option(&opt->optlist);
        }
}




/**
 * prelude_option_parse_arguments:
 * @option: A pointer on an option (list).
 * @filename: Pointer to the config filename.
 * @argc: Number of argument.
 * @argv: Argument list.
 *
 * prelude_option_parse_arguments(), parse the given argument and try to
 * match them against option in @option. If an option match, it's associated
 * callback function is called with the eventual option argument if any.
 *
 * Option not matched on the command line are searched in the configuration file
 * specified by @filename.
 *
 * if @option is NULL, all system option will be matched against argc, and argv.
 *
 * Returns: 0 if parsing the option succeed (including the case where one of
 * the callback returned -1 to request interruption of parsing), -1 if an error occured.
 */
int prelude_option_parse_arguments(void **context, prelude_option_t *option,
                                   const char *filename, int argc, char **argv) 
{
        int ret;
        int argv_index = 1;
        PRELUDE_LIST_HEAD(cb_list);
        prelude_optlist_t *optlist;
        
        if ( ! option )
                optlist = root_optlist;
        else
                /*
                 * FIXME: we shouldn't work with root_optlist here,
                 * but with the option itself, and option->optlist
                 */
                optlist = root_optlist; /* &option->optlist; */
        
        if ( filename ) {        
                ret = get_missing_options(context, filename, optlist);
                if ( ret == prelude_option_error || ret == prelude_option_end )
                        goto out;
        }
        
        ret = parse_argument(context, &cb_list, optlist, filename, argc, argv, &argv_index, 0); 
        if ( ret == prelude_option_error || ret == prelude_option_end )
                goto out;

        ret = call_option_from_cb_list(&cb_list, option_run_all);
        if ( ret == prelude_option_error || ret == prelude_option_end )
                goto out;

 out:
        reset_option(optlist);
                
        /*
         * reset.
         */
        
        return ret;
}





static prelude_optlist_t *get_default_optlist(void) 
{
        if ( root_optlist )
                return root_optlist;

        root_optlist = calloc(1, sizeof(*root_optlist));
        if ( ! root_optlist )
                return NULL;
        
        root_optlist->wide_msg = NULL;
        PRELUDE_INIT_LIST_HEAD(&root_optlist->optlist);
        
        return root_optlist;
}





/**
 * prelude_option_add:
 * @parent: Pointer on a parent option.
 * @flags: bitfields.
 * @shortopt: Short option name.
 * @longopt: Long option name.
 * @desc: Description of the option.
 * @has_arg: Define if the option has argument.
 * @set: Callback to be called when the value for this option change.
 * @get: Callback to be called to get the value for this option.
 *
 * prelude_option_add() create a new option. The option is set to be the child
 * of @parent, unless it is NULL. In this case the option is a root option.
 *
 * The @flags parameters can be set to CLI_HOOK (telling the option may be searched
 * on the command line only) or CFG_HOOk (telling the option may be searched in the
 * configuration file only) or both.
 *
 * Returns: Pointer on the option object, or NULL if an error occured.
 */
prelude_option_t *prelude_option_add(prelude_option_t *parent, int flags,
                                     char shortopt, const char *longopt, const char *desc,
                                     prelude_option_argument_t has_arg,
                                     int (*set)(void **context, prelude_option_t *opt, const char *optarg),
                                     int (*get)(void **context, char *buf, size_t size)) 
{
        prelude_option_t *new;
        prelude_optlist_t *optlist;
        
        new = malloc(sizeof(*new));
        if ( ! new ) 
                return NULL;

        PRELUDE_INIT_LIST_HEAD(&new->optlist.optlist);

        new->priority = option_run_no_order;
        new->flags = flags;
        new->has_arg = has_arg;
        new->longopt = longopt;
        new->shortopt = shortopt;
        new->description = desc;
        new->set = set;
        new->get = get;
        new->parent = parent;

        if ( parent )
                optlist = &parent->optlist;
        else 
                optlist = get_default_optlist();
        
        prelude_list_add_tail(&new->list, &optlist->optlist);
        
        if ( flags & WIDE_HOOK ) {
                root_optlist->wide_msgcount += 4; /* longopt && has_arg && start && end*/
                root_optlist->wide_msglen += strlen(longopt) + 1 + sizeof(uint8_t);
                
                if ( desc ) {
                        root_optlist->wide_msgcount++;
                        root_optlist->wide_msglen += strlen(desc) + 1;
                }
        }
        
        return (prelude_option_t *) new;
}



prelude_option_t *prelude_option_add_init_func(prelude_option_t *parent,
                                               int (*set)(void **context, prelude_option_t *opt, const char *arg)) 
{
        prelude_option_t *new;

        new = calloc(1, sizeof(*new));
        if ( ! new ) {
                log(LOG_ERR, "memory exhausted.\n");
                return NULL;
        }

        new->set = set;
        new->parent = parent;
        new->priority = option_run_no_order;
        new->flags = PRELUDE_OPTION_INIT_FLAG;
        new->longopt = PRELUDE_OPTION_INIT_NAME;
        
        
        PRELUDE_INIT_LIST_HEAD(&new->optlist.optlist);

        prelude_list_add_tail(&new->list, &parent->optlist.optlist);
        
        return new;
}



static void construct_option_msg(prelude_msg_t *msg, prelude_optlist_t *optlist) 
{
        prelude_option_t *opt;
        prelude_list_t *tmp;
        
        prelude_list_for_each(tmp, &optlist->optlist) {
                opt = prelude_list_entry(tmp, prelude_option_t, list);

                if ( !(opt->flags & WIDE_HOOK) )
                        continue;

                prelude_msg_set(msg, PRELUDE_MSG_OPTION_START, 0, NULL);
                prelude_msg_set(msg, PRELUDE_MSG_OPTION_NAME, strlen(opt->longopt) + 1, opt->longopt);
                prelude_msg_set(msg, PRELUDE_MSG_OPTION_DESC, strlen(opt->description) + 1, opt->description);
                prelude_msg_set(msg, PRELUDE_MSG_OPTION_HAS_ARG, sizeof(uint8_t), &opt->has_arg);               

                /*
                 * there is suboption.
                 */
                if ( ! prelude_list_empty(&opt->optlist.optlist) )
                        construct_option_msg(msg, &opt->optlist);
                        
                prelude_msg_set(msg, PRELUDE_MSG_OPTION_END, 0, NULL);                    
        }
}



prelude_msg_t *prelude_option_wide_get_msg(void) 
{
        uint64_t source_id;
        
        if ( ! root_optlist )
                return NULL;
        
        if ( root_optlist->wide_msg )
                return root_optlist->wide_msg;

        root_optlist->wide_msg =
            prelude_msg_new(root_optlist->wide_msgcount + 2,
                            root_optlist->wide_msglen + 100 + sizeof(uint64_t),
                            PRELUDE_MSG_OPTION_LIST, PRELUDE_MSG_PRIORITY_HIGH);

        if ( ! root_optlist->wide_msg ) {
                log(LOG_ERR, "memory exhausted.\n");
                return NULL;
        }

        
        source_id = prelude_hton64(prelude_client_get_analyzerid());
        prelude_msg_set(root_optlist->wide_msg, PRELUDE_MSG_OPTION_SOURCE_ID, sizeof(source_id), &source_id);

        construct_option_msg(root_optlist->wide_msg, root_optlist);
        
        return root_optlist->wide_msg;
}



static int get_max_char(const char *line, int descoff) 
{
        int desclen;
        int max = 0 , i;
        
        desclen = 80 - descoff;
        
        for ( i = 0; i < desclen; i++ ) {

                if ( line[i] == '\0' )
                        return i;
                
                if ( line[i] == ' ' )
                        max = i;
        }
        
        return max;
}



static void print_wrapped(const char *line, int descoff) 
{
        int max, i = 0, j;
        
        while ( 1 ) {
                max = get_max_char(&line[i], descoff);
                
                while ( max-- >= 0 ) {
                        
                        if ( line[i] == '\0' ) {
                                putchar('\n');
                                return;
                        } else
                                putchar(line[i++]);
                }
                        
                putchar('\n');
                for ( j = 0; j < descoff; j++ )
                        putchar(' ');
        }
}



static void print_options(prelude_optlist_t *optlist, int flags, int descoff, int depth) 
{
        int i, separator;
        prelude_option_t *opt;
        prelude_list_t *tmp;
        
        prelude_list_for_each(tmp, &optlist->optlist) {
                
                opt = prelude_list_entry(tmp, prelude_option_t, list);

                /*
                 * If flags is not there, continue.
                 */
                if ( flags && ! (opt->flags & flags) ) 
                        continue;
                
                for ( i = 0; i < depth; i++ )
                        printf("  ");
                
                if ( opt->shortopt != 0 )
                        i += printf("-%c ", opt->shortopt);

                if ( opt->longopt )
                        i += printf("--%s ", opt->longopt);

                while ( i++ < descoff )
                        putchar(' ');

                print_wrapped(opt->description, depth + descoff);
                
                separator = (strlen(opt->description) > (80 - descoff)) ? 1 : 0;
                if ( separator )
                        putchar('\n');
                
                if ( ! prelude_list_empty(&opt->optlist.optlist) ) 
                        print_options(&opt->optlist, flags, descoff, depth + 1);
        }

        putchar('\n');
}




/**
 * prelude_option_print:
 * @opt: Option(s) to print out.
 * @flags: Only option with specified flags will be printed.
 * @descoff: offset from the begining of the line where the description
 * should start.
 *
 * Dump option available in @opt and hooked to the given flags.
 * If @opt is NULL, then the root of the option list is used.
 */
void prelude_option_print(prelude_option_t *opt, int flags, int descoff) 
{
        prelude_optlist_t *optlist = (opt) ? &opt->optlist : root_optlist;
        
        print_options(optlist, flags, descoff, 0);
}



/**
 * prelude_option_set_priotity:
 * @option: Pointer on an option object.
 * @priority: Priority of the option object.
 *
 * prelude_option_set_priority() can be used to associate a priority
 * with an option. This can be used to solve dependancies problem within
 * differents options.
 *
 * A priority of -1 mean an option will always be executed last (with
 * all other option with a -1 priority).
 *
 * The default priority for an option is 0, the caller is responssible
 * for the way it assign priority (knowing that highest priority are always
 * executed first).
 */
void prelude_option_set_priority(prelude_option_t *option, int priority) 
{
        option->priority = priority;
}




/**
 * prelude_option_destroy:
 * @option: Pointer on an option object.
 *
 * Destroy a #prelude_option_t object and all data associated
 * with it (including all suboption).
 */
void prelude_option_destroy(prelude_option_t *option)
{
        prelude_option_t *opt;
        prelude_list_t *tmp, *bkp;

        prelude_list_del(&option->list);

        prelude_list_for_each_safe(tmp, bkp, &option->optlist.optlist) {
                
                opt = prelude_list_entry(tmp, prelude_option_t, list);
                prelude_option_destroy(opt);
        }

        free(option);
}




/**
 * prelude_option_set_warnings;
 * @flags: bitwise OR of flags.
 * @old_flags: Pointer to an integer where to store old flags to.
 *
 * Set current warnings flags to @flags (unless @flags is 0).
 *
 * Uppon return, the variable pointed to by @old_flags is updated
 * to contain the old flags unless it point to NULL.
 */
void prelude_option_set_warnings(int flags, int *old_flags) 
{
        if ( old_flags ) 
                *old_flags = warnings_flags;
        
        if ( flags != 0 ) 
                warnings_flags = flags;
}




void prelude_option_set_set_callback(prelude_option_t *opt,
                                     int (*set)(void **context, prelude_option_t *opt, const char *optarg))
{
        opt->set = set;
}



void *prelude_option_get_set_callback(prelude_option_t *opt)
{
        return opt->set;
}



char prelude_option_get_shortname(prelude_option_t *opt) 
{
        return opt->shortopt;
}



const char *prelude_option_get_longname(prelude_option_t *opt) 
{
        return opt->longopt;
}



void prelude_option_set_private_data(prelude_option_t *opt, void *data) 
{
        opt->private_data = data;
}



void *prelude_option_get_private_data(prelude_option_t *opt) 
{
        return opt->private_data;
}



int prelude_option_invoke_set(const char *option, const char *value)
{
        prelude_option_t *opt;

        opt = search_option(root_optlist, option, WIDE_HOOK, 1);
        if ( ! opt )
                return -1;
        
        if ( opt->has_arg == no_argument && value != NULL )
                return -1;
        
        if ( opt->has_arg == required_argument && value == NULL )
                return -1;

        return opt->set(NULL, opt, value);
}



int prelude_option_invoke_get(const char *option, char *buf, size_t len)
{
        prelude_option_t *opt;

        opt = search_option(root_optlist, option, WIDE_HOOK, 1);
        if ( ! opt || ! opt->get)
                return -1;
        
        return opt->get(NULL, buf, len);
}




prelude_option_t *prelude_option_new(prelude_option_t *parent) 
{
        prelude_option_t *new;
        prelude_optlist_t *optlist;
        
        if ( ! parent )
                optlist = get_default_optlist();
        else
                optlist = &parent->optlist;
        
        new = calloc(1, sizeof(*new));
        if ( ! new ) 
                return NULL;

        new->parent = parent;
        PRELUDE_INIT_LIST_HEAD(&new->optlist.optlist);
        
        prelude_list_add_tail(&new->list, &optlist->optlist);

        return new;
}



void prelude_option_set_longopt(prelude_option_t *opt, const char *longopt) 
{
        opt->longopt = longopt;
}



const char *prelude_option_get_longopt(prelude_option_t *opt) 
{
        return opt->longopt;
}



void prelude_option_set_description(prelude_option_t *opt, const char *description) 
{
        opt->description = description;
}



const char *prelude_option_get_description(prelude_option_t *opt)
{
        return opt->description;
}



void prelude_option_set_has_arg(prelude_option_t *opt, prelude_option_argument_t has_arg) 
{
        opt->has_arg = has_arg;
}



prelude_option_argument_t prelude_option_get_has_arg(prelude_option_t *opt) 
{
        return opt->has_arg;
}



void prelude_option_set_help(prelude_option_t *opt, const char *help) 
{
        opt->help = help;
}



const char *prelude_option_get_help(prelude_option_t *opt) 
{
        return opt->help;
}



void prelude_option_set_input_validation_regex(prelude_option_t *opt, const char *regex)
{
        opt->input_validation_regex = regex;
}



const char *prelude_option_get_input_validation_regex(prelude_option_t *opt)
{
        return opt->input_validation_regex;
}



void prelude_option_set_input_type(prelude_option_t *opt, uint8_t input_type) 
{
        opt->input_type = input_type;
}



uint8_t prelude_option_get_input_type(prelude_option_t *opt)
{
        return opt->input_type;
}



prelude_option_t *prelude_option_get_parent(prelude_option_t *opt)
{
        return opt->parent;
}

