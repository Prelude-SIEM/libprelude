/*****
*
* Copyright (C) 2001, 2002 Yoann Vandoorselaere <yoann@mandrakesoft.com>
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
#include "prelude-getopt-wide.h"
#include "prelude-message-id.h"

#include "list.h"
#include "prelude-log.h"
#include "variable.h"
#include "config-engine.h"
#include "prelude-getopt.h"


#define option_run_all 10


typedef struct prelude_optlist {    
        int argv_index;

        int wide_msglen;
        int wide_msgcount;
        prelude_msg_t *wide_msg;
        
        struct list_head optlist;
} prelude_optlist_t;



struct prelude_option {
        prelude_optlist_t optlist;
        struct list_head list;
        
        int flags;
        int priority;
        char shortopt;
        const char *longopt;  
        const char *description;
        prelude_option_argument_t has_arg;
        
        int called_from_cli;
        int (*set)(prelude_option_t *opt, const char *optarg);
        int (*get)(char *ibuf, size_t size);
        
        const char *help;
        const char *input_validation_rexex;
        enum { string, integer, boolean } input_type;

        void *private_data;
};



struct cb_list {
        struct list_head list;
        char *arg;
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



static int get_from_config(struct list_head *cb_list,
                           prelude_optlist_t *optlist, config_t *cfg, const char *section, int line);


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
static prelude_option_t *search_cli_option(prelude_optlist_t *optlist, const char *optname) 
{
        struct list_head *tmp;
        prelude_option_t *item;

        list_for_each(tmp, &optlist->optlist) {
                item = list_entry(tmp, prelude_option_t, list);

                if ( ! (item->flags & CLI_HOOK) )
                        continue;
                               
                if ( strcmp(item->longopt, optname) == 0 )
                        return item;
                
                if ( strlen(optname) == 1 && item->shortopt == *optname )
                        return item;
        }

        return NULL;
}




static int check_option_optarg(prelude_optlist_t *optlist,
                               int argc, char **argv, char *optarg, size_t size) 
{
        int len = 0;
        
        while ( optlist->argv_index < argc && *argv[optlist->argv_index] != '-' && len < size ) {

                if ( len > 0 )
                        optarg[len++] = ' ';
                
                strncpy(optarg + len, argv[optlist->argv_index], size - len);
                len += strlen(argv[optlist->argv_index]);
                argv[optlist->argv_index++] = "";
        }
        
        return 0;
}




static int check_option_reqarg(prelude_optlist_t *optlist, const char *option,
                               int argc, char **argv, char *optarg, size_t size) 
{
        int len = 0;
        
        if ( optlist->argv_index >= argc || *argv[optlist->argv_index] == '-' ) {
                fprintf(stderr, "Option %s require an argument.\n", option);
                return -1;
        }

        while ( optlist->argv_index < argc && *argv[optlist->argv_index] != '-' && len < size ) {
                
                if ( len > 0 )
                        optarg[len++] = ' ';
                
                strncpy(optarg + len, argv[optlist->argv_index], size - len);
                len += strlen(argv[optlist->argv_index]);
                argv[optlist->argv_index++] = "";
        }
                
        return 0;
}




static int check_option_noarg(prelude_optlist_t *optlist, const char *option,
                              int argc, char **argv)
{        
        if ( optlist->argv_index < argc && *argv[optlist->argv_index] != '-' && argv[optlist->argv_index] != "" ) {
                fprintf(stderr, "Option %s do not take an argument (%s).\n", option, argv[optlist->argv_index]);
                return -1;
        }
                
        return 0;
}




static int check_option(prelude_optlist_t *optlist, prelude_option_t *option,
                        char *optarg, size_t size, int argc, char **argv) 
{
        int ret;

        *optarg = '\0';
        
        switch (option->has_arg) {
                
        case optionnal_argument:
                ret = check_option_optarg(optlist, argc, argv, optarg, size);
                break;
                
        case required_argument:
                ret = check_option_reqarg(optlist, option->longopt, argc, argv, optarg, size);
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




static char *lookup_variable_if_needed(char *optarg) 
{
        const char *val;
        int i, j, len = 0;
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



static int call_option_cb(struct list_head *cblist, prelude_option_t *option, const char *arg) 
{
        struct cb_list *new, *cb;
        struct list_head *tmp, *prev = NULL;
        
        if ( option->priority == option_run_first ) 
                return option->set(option, arg);
        
        new = malloc(sizeof(*new));
        if ( ! new ) {
                log(LOG_ERR, "memory exhausted.\n");
                return -1;
        }

        if ( arg )
                new->arg = strdup(arg);
        else
                new->arg = NULL;
        
        new->option = option;

        if ( option->priority == option_run_last ) {
                list_add_tail(&new->list, cblist);
                return 0;
        }        
        
        list_for_each(tmp, cblist) {
                
                cb = list_entry(tmp, struct cb_list, list);
                
                if ( cb->option->priority < option->priority )
                        break;
                
                prev = tmp;
        }
        
        if ( ! prev )
                prev = cblist;
        
        list_add(&new->list, prev);

        return 0;
}




static int call_option_from_cb_list(struct list_head *cblist, int option_kind) 
{
        struct cb_list *cb;
        struct list_head *tmp, *bkp;
        int ret = prelude_option_success;
        
        list_for_each_safe(tmp, bkp, cblist) {
                
                cb = list_entry(tmp, struct cb_list, list);

                if ( option_kind != option_run_all && option_kind != cb->option->priority ) 
                        continue;
                
                ret = cb->option->set(cb->option, lookup_variable_if_needed(cb->arg));
                if ( ret == prelude_option_error || ret == prelude_option_end )
                        return ret;

                /*
                 * cb->arg isn't freed because client may use them later...
                 * FIXME: shouldn't the client use strdup() in this case ?
                 */
                list_del(&cb->list);
                free(cb);
        }

        return ret;
}




/*
 * Get the option represented by 'opt' from the config file,
 * and call the call_option_cb() function as much time as it exist.
 */
static int option_get_all(struct list_head *cb_list,
                          prelude_option_t *opt, config_t *cfg, const char *section, int line) 
{
        int ret;
        const char *str, *entry;

        entry = opt->longopt;
        
        while ( 1 ) {
                str = config_get(cfg, section, entry, &line);
                if ( ! str )
                        return prelude_option_success;
                
                line++;
                        
                ret = call_option_cb(cb_list, opt, str);
                if ( ret == prelude_option_error || ret == prelude_option_end )
                        return ret;
        }

        return 0;
}




/*
 * Get the section represented by 'opt' from the config file,
 * and call the call_option_cb() function as much time as it exist.
 * Initial 'opt' have to be a parent option.
 */
static int section_get_all(struct list_head *cb_list,
                           prelude_option_t *opt, config_t *cfg) 
{
        int ret, line = 0;
        
        /*
         * we are a parent option.
         * For each section in the config file, corresponding to
         * this parent,
         */
        while ( 1 ) {
                ret = config_get_section(cfg, opt->longopt, &line);                
                if ( ret == prelude_option_error || ret == prelude_option_end ) 
                        return ret;
                
                line++;
                
                if ( opt->set ) {
                        /*
                         * call the parent callback.
                         */
                        ret = call_option_cb(cb_list, opt, NULL);
                        if ( ret == prelude_option_error || ret == prelude_option_end ) 
                                return ret;
                }

                /*
                 * there may be suboption, which we want to proceed.
                 */
                ret = get_from_config(cb_list, &opt->optlist, cfg, opt->longopt, line);
                if ( ret == prelude_option_error || ret == prelude_option_end ) 
                        return ret;

                /*
                 * flush queued option when leaving the parent,
                 * because some option may be associated with only *this* parent.
                 */
                ret = call_option_from_cb_list(cb_list, option_run_no_order);
                if ( ret == prelude_option_error || ret == prelude_option_end ) 
                        return ret;
        }

        return 0;
}




static int process_option_cfg_hook(struct list_head *cb_list, prelude_option_t *opt,
                                   config_t *cfg, const char *section, int line) 
{
        if ( opt->called_from_cli && ! list_empty(&opt->optlist.optlist) )
                /*
                 * a parent section specified on CLI completly override CFG.
                 */
                return prelude_option_success;
        
        if ( ! (opt->flags & CFG_HOOK) )                
                return prelude_option_success;

        if ( list_empty(&opt->optlist.optlist) ) 
                /*
                 * Normal option (no suboption)
                 */
                return option_get_all(cb_list, opt, cfg, section, line);
        else 
                return section_get_all(cb_list, opt, cfg);
        
        return 0;
}




static int get_from_config(struct list_head *cb_list,
                           prelude_optlist_t *optlist, config_t *cfg, const char *section, int line) 
{
        struct list_head *tmp;
        prelude_option_t *optitem;

        /*
         * for each option in the option list.
         */
        list_for_each(tmp, &optlist->optlist) {
                optitem = list_entry(tmp, prelude_option_t, list);
                process_option_cfg_hook(cb_list, optitem, cfg, section, line);
        }

        return 0;
}



/*
 * Try to get all option that were not set from the command line in the config file.
 */
static int get_missing_options(const char *filename, prelude_optlist_t *optlist) 
{
        int ret;
        LIST_HEAD(cb_list);
        config_t *cfg = NULL;

        cfg = config_open(filename);
        if ( ! cfg ) {
                log(LOG_INFO, "couldn't open %s.\n", filename);
                return -1;
        }

        ret = get_from_config(&cb_list, optlist, cfg, NULL, 0);
        if ( ret < 0 )
                goto err;


        ret = call_option_from_cb_list(&cb_list, option_run_all);
        
 err:
        config_close(cfg);
        
        return ret;
}




static int parse_argument(struct list_head *cb_list,
                          prelude_optlist_t *optlist, const char *filename,
                          int argc, char **argv, int depth)
{
        int ret;
        char optarg[256];
        prelude_option_t *opt;
        const char *arg, *old;
        int saved_index = 0, tmp;
        
        while ( optlist->argv_index < argc ) {
                
                old = arg = argv[optlist->argv_index++];                
                            
                if ( *arg != '-' ) {                        
                        /*
                         * If arg == "", this is an argument we processed, and so nullified.
                         */
                        if ( arg != "" )
                                option_err(OPT_INVAL_ARG, "Invalid argument : \"%s\".\n", arg);
                        
                        continue;
                }

                while ( *arg == '-' ) arg++;
                
                opt = search_cli_option(optlist, arg);
                if ( ! opt ) {
                        if ( depth == 0 ) {
                                option_err(OPT_INVAL, "Invalid option : \"%s\".\n", arg);
                                continue;
                        }
                        
                        optlist->argv_index--;
                        return 0;
                }

                
                argv[optlist->argv_index - 1] = "";
                
                /*
                 * If the option we just found have sub-option.
                 * Try to match the rest of our argument against them.
                 */
                if ( ! list_empty(&opt->optlist.optlist) ) {
                        saved_index = optlist->argv_index;

                        if ( opt->has_arg == required_argument ||
                             (opt->has_arg == optionnal_argument && *argv[optlist->argv_index + 1] == '-') ) 
                                opt->optlist.argv_index = optlist->argv_index + 1;
                        else
                                opt->optlist.argv_index = optlist->argv_index;

                        opt->called_from_cli = 1;
                        
                        ret = parse_argument(cb_list, &opt->optlist, filename, argc, argv, depth + 1);
                        if ( ret == prelude_option_end || ret == prelude_option_error )
                                return ret;
    
                        optlist->argv_index = opt->optlist.argv_index;
                }

                                
                /*
                 * check option *after* sub-option, so the caller can do some kind
                 * of dependancy between option. We use the saved argument index, because
                 * suboption parsing could have incremented the argument index.
                 */
                tmp = 0;
                if ( saved_index ) {
                        tmp = optlist->argv_index;
                        optlist->argv_index = saved_index;
                        saved_index = 0;
                }

                ret = check_option(optlist, opt, optarg, sizeof(optarg), argc, argv);
                if ( ret < 0 ) 
                        return -1;
                
                if ( tmp ) 
                        optlist->argv_index = tmp;
                                
                if ( opt->set ) {
                        opt->called_from_cli = 1;
                        
                        ret = call_option_cb(cb_list, opt, optarg);
                        if ( ret == prelude_option_end || ret == prelude_option_error )
                                return ret;        
                }
        }
        
        return 0;
}




static void reset_option(prelude_optlist_t *optlist) 
{
        struct list_head *tmp;
        prelude_option_t *opt;

        list_for_each(tmp, &optlist->optlist) {
                opt = list_entry(tmp, prelude_option_t, list);
                reset_option(&opt->optlist);
                opt->called_from_cli = 0;
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
int prelude_option_parse_arguments(prelude_option_t *option, const char *filename, int argc, char **argv) 
{
        int ret;
        LIST_HEAD(cb_list);        
        prelude_optlist_t *optlist;
        
        if ( ! option )
                optlist = root_optlist;
        else
                /*
                 * FIXME: we shouldn't work with root_optlist here,
                 * but with the option itself, and option->optlist
                 */
                optlist = root_optlist; /* &option->optlist; */
        
        ret = parse_argument(&cb_list, optlist, filename, argc, argv, 0); 
        if ( ret == prelude_option_error || ret == prelude_option_end )
                goto out;

        ret = call_option_from_cb_list(&cb_list, option_run_all);
        if ( ret == prelude_option_error || ret == prelude_option_end )
                goto out;

        /*
         * Only try to get missing options from config file
         * if parsing arguments succeed and caller didn't requested us to stop.
         */
        if ( filename ) 
                ret = get_missing_options(filename, optlist);

 out:
        reset_option(optlist);
                
        /*
         * reset.
         */
        root_optlist->argv_index = 1;
        
        return ret;
}





static prelude_optlist_t *get_default_optlist(void) 
{
        if ( root_optlist )
                return root_optlist;

        root_optlist = malloc(sizeof(*root_optlist));
        if ( ! root_optlist )
                return NULL;
        
        root_optlist->wide_msg = NULL;
        root_optlist->wide_msgcount = 0;
        root_optlist->wide_msglen = 0;
        root_optlist->argv_index = 1;
        INIT_LIST_HEAD(&root_optlist->optlist);
        
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
                                     prelude_option_argument_t has_arg, int (*set)(prelude_option_t *opt, const char *optarg),
                                     int (*get)(char *buf, size_t size)) 
{
        prelude_option_t *new;
        prelude_optlist_t *optlist;
        
        new = malloc(sizeof(*new));
        if ( ! new ) 
                return NULL;

        INIT_LIST_HEAD(&new->optlist.optlist);

        new->priority = option_run_no_order;
        new->flags = flags;
        new->has_arg = has_arg;
        new->longopt = longopt;
        new->shortopt = shortopt;
        new->description = desc;
        new->set = set;
        new->get = get;
        new->called_from_cli = 0;

        if ( parent ) {
                optlist = &parent->optlist;
                
                /*
                 * parent option are always last to be ran, except if the caller
                 * specified a priority, because it often depend on the configuration
                 * of the child option.
                 */
                if ( parent->priority == option_run_no_order )
                        parent->priority = option_run_last;
        } else 
                optlist = get_default_optlist();
        
        list_add_tail(&new->list, &optlist->optlist);
        
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




static void construct_option_msg(prelude_msg_t *msg, prelude_optlist_t *optlist) 
{
        struct list_head *tmp;
        prelude_option_t *opt;
        
        list_for_each(tmp, &optlist->optlist) {
                opt = list_entry(tmp, prelude_option_t, list);

                if ( !(opt->flags & WIDE_HOOK) )
                        continue;

                prelude_msg_set(msg, PRELUDE_OPTION_START, 0, NULL);
                prelude_msg_set(msg, PRELUDE_OPTION_NAME, strlen(opt->longopt) + 1, opt->longopt);
                prelude_msg_set(msg, PRELUDE_OPTION_DESC, strlen(opt->description) + 1, opt->description);
                prelude_msg_set(msg, PRELUDE_OPTION_HAS_ARG, sizeof(uint8_t), &opt->has_arg);               

                /*
                 * there is suboption.
                 */
                if ( ! list_empty(&opt->optlist.optlist) )
                        construct_option_msg(msg, &opt->optlist);
                        
                prelude_msg_set(msg, PRELUDE_OPTION_END, 0, NULL);                    
        }
}



prelude_msg_t *prelude_option_wide_get_msg(void) 
{
        if ( root_optlist->wide_msg )
                return root_optlist->wide_msg;

        root_optlist->wide_msg = prelude_msg_new(root_optlist->wide_msgcount, root_optlist->wide_msglen,
                                                 PRELUDE_MSG_OPTION_LIST, PRELUDE_MSG_PRIORITY_HIGH);
        if ( ! root_optlist->wide_msg ) {
                log(LOG_ERR, "memory exhausted.\n");
                return NULL;
        }
        

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
                
                if ( ! list_empty(&opt->optlist.optlist) ) 
                        print_options(&opt->optlist, flags, descoff, depth + 1);
        }

        putchar('\n');
}




/**
 * prelude_option_print:
 * @flags: Only option with specified flags will be printed.
 * @descoff: offset from the begining of the line where the description
 * should start.
 *
 * Dump option available in optlist and hooked to the command line.
 */
void prelude_option_print(int flags, int descoff) 
{        
        print_options(root_optlist, flags, descoff, 0);
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
        struct list_head *tmp;

        list_del(&option->list);
        
        for ( tmp = option->optlist.optlist.next; tmp != &option->optlist.optlist; ) {
                
                opt = list_entry(tmp, prelude_option_t, list);
                tmp = tmp->next;
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
