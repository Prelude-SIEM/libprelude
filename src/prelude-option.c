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
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <sys/types.h>
#include <assert.h>

#include "libmissing.h"
#include "prelude-inttypes.h"
#include "prelude-message-id.h"
#include "prelude-msgbuf.h"

#include "prelude-linked-object.h"
#include "prelude-list.h"
#include "prelude-log.h"
#include "variable.h"
#include "config-engine.h"
#include "prelude-option.h"
#include "prelude-client.h"
#include "prelude-error.h"
#include "common.h"


#define DEFAULT_INSTANCE_NAME "default"


struct prelude_option_context {
        prelude_list_t list;
        void *data;
        char *name;
};


struct prelude_option {
        PRELUDE_LINKED_OBJECT;
        
        prelude_list_t optlist;
        prelude_option_t *parent;

        prelude_bool_t already_set;
        prelude_option_type_t type;
        prelude_option_priority_t priority;
        char shortopt;
        
        char *value;
        const char *longopt;  
        const char *description;
        prelude_option_argument_t has_arg;
        
	int (*commit)(prelude_option_t *opt, prelude_string_t *out, void *context);
        int (*set)(prelude_option_t *opt, const char *optarg, prelude_string_t *out, void *context);
        int (*get)(prelude_option_t *opt, prelude_string_t *out, void *context);
	int (*destroy)(prelude_option_t *opt, prelude_string_t *out, void *context);
        
        const char *help;
        const char *input_validation_regex;
        enum { string, integer, boolean } input_type;

        void *private_data;
        
        prelude_list_t value_list;

        void *default_context;
        prelude_list_t context_list;
};



struct cb_list {
        prelude_list_t list;
        char *arg;
        prelude_list_t children;
        prelude_option_t *option;
};


static int get_missing_options(config_t *cfg,
                               const char *filename, prelude_list_t *cblist,
                               prelude_option_t *rootlist, unsigned int *line, int depth, prelude_string_t *err);

/*
 * contain all option
 */
static prelude_option_t *root_optlist = NULL;
extern prelude_option_t *_prelude_generic_optlist;


/*
 * Warning are turned on by default.
 */
static int warnings_flags = PRELUDE_OPTION_WARNING_OPTION|PRELUDE_OPTION_WARNING_ARG;



static void option_err(int flag, const char *fmt, ...) 
{
        if ( warnings_flags & flag ) {
                va_list ap;
                
                va_start(ap, fmt);
                vfprintf(stderr, fmt, ap);
                va_end(ap);
        }
}



static prelude_option_context_t *search_context(prelude_option_t *opt, const char *name)
{
        int ret;
        prelude_list_t *tmp;
        prelude_option_context_t *ptr;

        if ( ! name || ! *name )
                name = DEFAULT_INSTANCE_NAME;
                
        prelude_list_for_each(&opt->context_list, tmp) {

                ptr = prelude_list_entry(tmp, prelude_option_context_t, list);

                ret = strcasecmp(ptr->name, name);
                if ( ret == 0 )
                        return ptr;
        }

        return NULL;
}



static int cmp_option(prelude_option_t *opt, const char *optname, prelude_option_type_t type)
{
        if ( ! (opt->type & type) )
                return -1;
        
        if ( opt->longopt && strcasecmp(opt->longopt, optname) == 0 )
                return 0;
                
        if ( strlen(optname) == 1 && opt->shortopt == *optname )
                return 0;

        return -1;
}




/*
 * Search an option of a given name in the given option list.
 */
static prelude_option_t *search_option(prelude_option_t *root,
                                       const char *optname, prelude_option_type_t type, int walk_children) 
{
        int cmp;
        prelude_list_t *tmp;
        prelude_option_t *item, *ret;
        
        cmp = cmp_option(root, optname, type);
        if ( cmp == 0 )
                return root;
        
        prelude_list_for_each(&root->optlist, tmp) {
                item = prelude_linked_object_get_object(tmp);
                                
                if ( walk_children || (! item->longopt && ! item->shortopt) ) {
                        ret = search_option(item, optname, type, walk_children);
                        if ( ret )
                                return ret;
                }
                                
                cmp = cmp_option(item, optname, type);
                if ( cmp == 0 )
                        return item;
        }
        
        return NULL;
}



static prelude_bool_t is_an_argument(const char *stuff) 
{
        size_t len = strlen(stuff);
        
        if ( stuff[0] == '-' && (len == 2 || (len > 2 && stuff[1] == '-')) ) 
                return FALSE;
        
        return TRUE;
}



static void reorder_argv(int *argc, char **argv, int removed, int *argv_index)
{
        int i;
        
        for ( i = removed; (i + 1) < *argc; i++ ) 
                argv[i] = argv[i + 1];

        (*argc)--;
        (*argv_index)--;
}



static int check_option_optarg(const char **outptr, const char *option, const char *arg)
{
        if ( arg && is_an_argument(arg) )
                *outptr = arg;
        
        return 0;
}




static int check_option_reqarg(const char **outptr, const char *option, const char *arg)
{        
        if ( ! arg || ! is_an_argument(arg) ) {                
                fprintf(stderr, "Option %s require an argument.\n", option);
                return -1;
        }

        *outptr = arg;
                
        return 0;
}



static int check_option(prelude_option_t *option, const char **optarg, const char *arg)
{
        int ret = -1;
        
        *optarg = NULL;
        
        switch (option->has_arg) {
                
        case PRELUDE_OPTION_ARGUMENT_OPTIONAL:
                ret = check_option_optarg(optarg, option->longopt, arg);
                break;
                
        case PRELUDE_OPTION_ARGUMENT_REQUIRED:
                ret = check_option_reqarg(optarg, option->longopt, arg);
                break;

        case PRELUDE_OPTION_ARGUMENT_NONE:
                ret = 0;
                break;
        }

        return ret;
}



static int process_cfg_file(prelude_list_t *cblist, prelude_option_t *optlist,
                            const char *filename, prelude_string_t *err) 
{
        config_t *cfg;
        int line = 0, ret;

        prelude_log_debug(3, "Using configuration file: %s.\n", filename);
        
        ret = config_open(&cfg, filename);
        if ( ret < 0 ) {
                prelude_string_sprintf(err, "%s: could not open %s: %s", prelude_strsource(ret), filename, prelude_strerror(ret));
                return ret;
        }
        
        ret = get_missing_options(cfg, filename, cblist, optlist, &line, 0, err);
        
        config_close(cfg);

        return ret;
}




static int call_option_cb(struct cb_list **cbl, prelude_list_t *cblist, prelude_option_t *option, const char *arg) 
{
        struct cb_list *new, *cb;
        prelude_list_t *tmp, *prev = NULL;
        
        prelude_log_debug(3, "%s(%s)\n", option->longopt, arg);
                
        prelude_list_for_each(cblist, tmp) {
                cb = prelude_list_entry(tmp, struct cb_list, list);
                
                if ( ! prev && option->priority < cb->option->priority )
                        prev = tmp;
        }
        
        *cbl = new = malloc(sizeof(*new));
        if ( ! new )
                return prelude_error_from_errno(errno);

        prelude_list_init(&new->children);
                
        new->arg = (arg) ? strdup(arg) : NULL;
        new->option = option;

        if ( option->priority == PRELUDE_OPTION_PRIORITY_LAST ) {
                prelude_list_add_tail(cblist, &new->list);
                return 0;
        }

        if ( ! prev )
                prev = cblist;
        
        prelude_list_add_tail(prev, &new->list);
        *cbl = new;
        
        return 0;
}



static int do_set(prelude_option_t *opt, const char *value, prelude_string_t *out, void **context)
{
        int ret;
        prelude_option_context_t *oc;
                
        if ( opt->default_context )
                *context = opt->default_context;
        
        if ( opt->type & PRELUDE_OPTION_TYPE_CONTEXT ) {

                if ( ! value )
                        value = DEFAULT_INSTANCE_NAME;
                
                oc = search_context(opt, value);
                if ( oc ) {
                        *context = oc->data;
                        return 0;
                }
        }
                
        if ( ! opt->set )
                return 0;
        
        ret = opt->set(opt, value, out, *context);        
        if ( ret < 0 )
                return ret;

        if ( opt->type & PRELUDE_OPTION_TYPE_CONTEXT ) {
                
                oc = search_context(opt, value);                
                if ( ! oc )
		        return -1;
                
                *context = oc->data;
        }

        return ret;
}



static int call_option_from_cb_list(prelude_list_t *cblist, prelude_string_t *err, void *default_context, int depth) 
{
        int ret = 0;
        struct cb_list *cb;
        prelude_list_t *tmp, *bkp;
	void *context = default_context;

        prelude_list_for_each_safe(cblist, tmp, bkp) {
                cb = prelude_list_entry(tmp, struct cb_list, list);

                prelude_log_debug(2, "%s(%s)\n", cb->option->longopt, cb->arg);
                
                ret = do_set(cb->option, cb->arg, err, &context);                
                if ( ret < 0 ) 
                        return ret;
                
                if ( ! prelude_list_is_empty(&cb->children) ) {
                        
                        ret = call_option_from_cb_list(&cb->children, err, context, depth + 1);
                        if ( ret < 0 )
                                return ret;
                        
			if ( cb->option->commit ) 
				ret = cb->option->commit(cb->option, err, context);
                }
		
                context = default_context;
		
                if ( cb->arg )
                        free(cb->arg);
                
                prelude_list_del(&cb->list);
                free(cb);
        }
        
        return 0;
}




/*
 * Try to get all option that were not set from the command line in the config file.
 */
static int get_missing_options(config_t *cfg, const char *filename, prelude_list_t *cblist,
                               prelude_option_t *rootlist, unsigned int *line, int depth, prelude_string_t *err) 
{
        int ret;
        const char *argptr;
        prelude_option_t *opt;
        struct cb_list *cbitem;
        char *section = NULL, *entry = NULL, *value = NULL;
        
        while ( (config_get_next(cfg, &section, &entry, &value, line)) == 0 ) {
                
                opt = search_option(rootlist, (section && ! entry) ?
                                    section : entry, PRELUDE_OPTION_TYPE_CFG, 0);
                
                if ( ! opt && entry && value && strcmp(entry, "include") == 0 ) {        
                                                
                        ret = process_cfg_file(cblist, rootlist, value, err);
                        if ( ret < 0 ) 
                                return ret;
                        
                        continue;
                }
                                
                if ( ! opt ) {
                        if ( search_option(_prelude_generic_optlist, (section && ! entry) ? section : entry, ~0, TRUE) )
                                continue;
                        
                        if ( depth != 0 ) {
                                (*line)--;
                                if ( entry ) free(entry);
                                if ( value ) free(value);
                                if ( section ) free(section);
                                
                                return 0;
                        }
                                                
                        if ( section && ! entry )
                                option_err(PRELUDE_OPTION_WARNING_OPTION,
                                           "%s:%d: invalid section : \"%s\".\n", filename, *line, section);
                        else
                                option_err(PRELUDE_OPTION_WARNING_ARG,
                                           "%s:%d: invalid option \"%s\" in \"%s\" section.\n",
                                           filename, *line, entry, (section) ? section : "global");

                        continue;
                }
                
                if ( section && ! entry ) {

                        ret = check_option(opt, &argptr, value);
                        if ( ret < 0 )
                                return ret;

                        ret = call_option_cb(&cbitem, cblist, opt, argptr);
                        if ( ret < 0 ) 
                                return ret;
                        
                        ret = get_missing_options(cfg, filename, &cbitem->children, opt, line, depth + 1, err);
                        if ( ret < 0 )
                                return -1;
                } else {                        
                        if ( opt->already_set )
                                continue;

                        ret = check_option(opt, &argptr, value);                        
                        if ( ret < 0 )
                                return ret;
                        
                        ret = call_option_cb(&cbitem, cblist, opt, argptr);
                        if ( ret < 0 ) 
                                return ret;
                }
        }

        return 0;
}



static int parse_argument(prelude_list_t *cb_list, prelude_option_t *optlist,
                          int *argc, char **argv, int *argv_index, int depth)
{
        int ret;
        prelude_option_t *opt;
        struct cb_list *cbitem;
        const char *arg, *old, *argptr;
        
        while ( *argv_index < *argc ) {
                                
                old = arg = argv[(*argv_index)++];
                
                if ( *arg != '-' )
                        continue;
                                
                while ( *arg == '-' ) arg++;

                if ( ! isalnum((int) *arg) )
                        continue;
                     
                opt = search_option(optlist, arg, PRELUDE_OPTION_TYPE_CLI, 0);                
                if ( ! opt ) {                        
                        if ( depth ) {
                                (*argv_index)--;
                                return 0;
                        }
                        
                        option_err(PRELUDE_OPTION_WARNING_OPTION, "invalid option -- \"%s\" (%d).\n", arg, depth);
                        continue;
                }
                
                reorder_argv(argc, argv, *argv_index - 1, argv_index);
                
                ret = check_option(opt, &argptr, (*argv_index < *argc) ? argv[*argv_index] : NULL);
                if ( ret < 0 ) 
                        return -1;

                if ( argptr )
                        reorder_argv(argc, argv, *argv_index, argv_index);

                ret = call_option_cb(&cbitem, cb_list, opt, argptr);
                if ( ret < 0 )
                        return ret;

                opt->already_set = TRUE;
                
                /*
                 * If the option we just found have sub-option.
                 * Try to match the rest of our argument against them.
                 */
                if ( ! prelude_list_is_empty(&opt->optlist) ) {
                        
                        ret = parse_argument(&cbitem->children, opt,
                                             argc, argv, argv_index, depth + 1);

                        if ( ret < 0 )
                                return ret;
                }
        }
        
        return 0;
}




static int get_option_from_optlist(void *context, prelude_option_t *optlist, prelude_list_t *cb_list,
                                   const char **filename, int *argc, char **argv, prelude_string_t **err)
{
        int argv_index = 1, ret = 0;
        
        if ( argc ) {                
                ret = parse_argument(cb_list, optlist, argc, argv, &argv_index, 0);
                if ( ret < 0 )
                        return ret;

                ret = call_option_from_cb_list(cb_list, *err, context, 0);
                if ( ret < 0 )
                        return ret;
        }
        
        if ( filename && *filename )
                ret = process_cfg_file(cb_list, optlist, *filename, *err);
        
        return ret;
}






/**
 * prelude_option_read:
 * @option: A pointer on an option (list).
 * @filename: Pointer to the config filename.
 * @argc: Number of argument.
 * @argv: Argument list.
 * @err: Pointer to a #prelude_string_t object where to store an error string.
 * @context:
 *
 * prelude_option_read(), parse the given argument and try to
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
int prelude_option_read(prelude_option_t *option, const char **filename,
                        int *argc, char **argv, prelude_string_t **err, void *context) 
{
        int ret;
        PRELUDE_LIST(optlist);
        PRELUDE_LIST(cb_list);
        
        ret = prelude_string_new(err);
        if ( ret < 0 )
                return ret;
        
        if ( option )
                ret = get_option_from_optlist(context, option, &cb_list, filename, argc, argv, err);
        else
                ret = get_option_from_optlist(context, root_optlist, &cb_list, filename, argc, argv, err);

        if ( ret < 0 )
                goto err;
                
        ret = call_option_from_cb_list(&cb_list, *err, context, 0);

 err:
        if ( prelude_string_is_empty(*err) ) {
                prelude_string_destroy(*err);
                *err = NULL;
        }
        
        if ( ret < 0 )
                return ret;

        return 0;
}





/**
 * prelude_option_add:
 * @parent: Pointer on a parent option.
 * @retopt: Pointer where to store the created option.
 * @type: bitfields.
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
 * The @type parameters can be set to PRELUDE_OPTION_TYPE_CLI (telling the
 * option may be searched on the command line), PRELUDE_OPTION_TYPE_CFG (telling
 * the option may be searched in the configuration file) or both.
 *
 * Returns: Pointer on the option object, or NULL if an error occured.
 */
int prelude_option_add(prelude_option_t *parent, prelude_option_t **retopt, prelude_option_type_t type,
                       char shortopt, const char *longopt, const char *desc, prelude_option_argument_t has_arg,
                       int (*set)(prelude_option_t *opt, const char *optarg, prelude_string_t *err, void *context),
                       int (*get)(prelude_option_t *opt, prelude_string_t *out, void *context)) 
{
        int ret;
        prelude_option_t *new;
        
        ret = prelude_option_new(parent, &new);
        if ( ret < 0 )
                return ret;

        prelude_list_init(&new->optlist);
        prelude_list_init(&new->context_list);
                
        new->priority = PRELUDE_OPTION_PRIORITY_NONE;
        new->type = type;
        new->has_arg = has_arg;
        new->longopt = longopt;
        new->shortopt = shortopt;
        new->description = desc;
        new->set = set;
        new->get = get;

        if ( retopt )
                *retopt = new;
        
        return 0;
}



static void send_option_msg(prelude_bool_t parent_need_context,
                            void *context, prelude_option_t *opt, const char *iname, prelude_msgbuf_t *msg)
{
        int ret;
        prelude_string_t *value;
        const char *name = (iname) ? iname : opt->longopt;

        prelude_msgbuf_set(msg, PRELUDE_MSG_OPTION_START, 0, NULL);
        prelude_msgbuf_set(msg, PRELUDE_MSG_OPTION_NAME, strlen(name) + 1, name);
        prelude_msgbuf_set(msg, PRELUDE_MSG_OPTION_HAS_ARG, sizeof(uint8_t), &opt->has_arg);
        prelude_msgbuf_set(msg, PRELUDE_MSG_OPTION_TYPE, sizeof(uint8_t), &opt->type);
        
        if ( opt->description )
                prelude_msgbuf_set(msg, PRELUDE_MSG_OPTION_DESC, strlen(opt->description) + 1, opt->description);

        if ( opt->type & PRELUDE_OPTION_TYPE_CONTEXT && ! context )
                return;

        if ( parent_need_context && ! context )
                return;

        ret = prelude_string_new(&value);
        if ( ret < 0 )
                return;

        if ( ! opt->get )
                return;

        ret = opt->get(opt, value, context);
        if ( ret < 0 )
                return;

        if ( ! prelude_string_is_empty(value) ) 
                prelude_msgbuf_set(msg, PRELUDE_MSG_OPTION_VALUE,
                                   prelude_string_get_len(value) + 1,
                                   prelude_string_get_string(value));
}


static void construct_option_msg(prelude_bool_t parent_need_context,
                                 void *default_context, prelude_msgbuf_t *msg, prelude_option_t *root) 
{
        char value[1024];
        prelude_option_t *opt;
        prelude_list_t *tmp, *tmp2;
	prelude_option_context_t *oc;
                        
        prelude_list_for_each(&root->optlist, tmp) {
                opt = prelude_linked_object_get_object(tmp);
                                
                prelude_list_for_each(&opt->context_list, tmp2) {
                        oc = prelude_list_entry(tmp2, prelude_option_context_t, list);
                        
                        snprintf(value, sizeof(value), "%s[%s]", opt->longopt, oc->name);
                        
                        if ( opt->type & PRELUDE_OPTION_TYPE_WIDE )
                                send_option_msg(TRUE, oc->data, opt, value, msg);
                        
                        construct_option_msg(TRUE, oc->data, msg, opt);

                        if ( opt->type & PRELUDE_OPTION_TYPE_WIDE )
                                prelude_msgbuf_set(msg, PRELUDE_MSG_OPTION_END, 0, NULL);
                }

                if ( prelude_list_is_empty(&opt->context_list) ) {
                        void *ctx = NULL;
                        prelude_bool_t need_ctx = opt->type & PRELUDE_OPTION_TYPE_CONTEXT ? TRUE : parent_need_context;

                        if ( ! (opt->type & PRELUDE_OPTION_TYPE_CONTEXT) )                        
                                ctx = opt->default_context ? opt->default_context : default_context;
                                                
                        if ( opt->type & PRELUDE_OPTION_TYPE_WIDE )
                                send_option_msg(need_ctx, ctx, opt, NULL, msg);
                        
                        construct_option_msg(need_ctx, ctx, msg, opt);
                        
                        if ( opt->type & PRELUDE_OPTION_TYPE_WIDE )
                                prelude_msgbuf_set(msg, PRELUDE_MSG_OPTION_END, 0, NULL);
                }
        }        
}




int prelude_option_wide_send_msg(prelude_msgbuf_t *msgbuf, void *context)
{
        prelude_msgbuf_set(msgbuf, PRELUDE_MSG_OPTION_LIST, 0, NULL);
        prelude_msgbuf_set(msgbuf, PRELUDE_MSG_OPTION_START, 0, NULL);
        
        construct_option_msg(FALSE, context, msgbuf, root_optlist);
        
        return 0;
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



static void print_options(prelude_option_t *root, prelude_option_type_t type, int descoff, int depth) 
{
        int i;
        prelude_option_t *opt;
        prelude_list_t *tmp;
        
        prelude_list_for_each(&root->optlist, tmp) {
                
                opt = prelude_linked_object_get_object(tmp);

                /*
                 * If type is not there, continue.
                 */
                if ( opt->type == PRELUDE_OPTION_TYPE_ROOT )
                        print_options(opt, type, descoff, depth);
                else {                          
                        if ( type && ! (opt->type & type) ) 
                                continue;

                        for ( i = 0; i < depth; i++ )
                                printf("  ");
                        
                        if ( ! prelude_list_is_empty(&opt->optlist) )
                                putchar('\n');
                        
                        if ( opt->shortopt != 0 )
                                i += printf("-%c ", opt->shortopt);
                        
                        if ( opt->longopt )
                                i += printf("--%s ", opt->longopt);
                        
                        while ( i++ < descoff )
                                putchar(' ');
                        
                        if ( opt->description )
                                print_wrapped(opt->description, depth + descoff);
                        else
                                putchar('\n');

                        if ( ! prelude_list_is_empty(&opt->optlist) )
                                print_options(opt, type, descoff, depth + 1);
                }
        }
        putchar('\n');
}




/**
 * prelude_option_print:
 * @opt: Option(s) to print out.
 * @type: Only option with specified types will be printed.
 * @descoff: offset from the begining of the line where the description should start.
 *
 * Dump option available in @opt and hooked to the given types.
 * If @opt is NULL, then the root of the option list is used.
 */
void prelude_option_print(prelude_option_t *opt, prelude_option_type_t type, int descoff) 
{        
        print_options(opt ? opt : root_optlist, type, descoff, 0);
}



/**
 * prelude_option_set_priority:
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
void prelude_option_set_priority(prelude_option_t *option, prelude_option_priority_t priority) 
{
        assert(prelude_list_is_empty(&option->optlist));
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
        prelude_list_t *tmp, *bkp;
        prelude_option_t *opt;
        
        if ( ! option )
                option = root_optlist;
        
        prelude_list_for_each_safe(&option->optlist, tmp, bkp) {
                opt = prelude_linked_object_get_object(tmp);
                prelude_option_destroy(opt);
        }

        if ( option ) {                
                if ( option->value )
                        free(option->value);
                
                if ( ! prelude_list_is_empty(&option->_list) )
                        prelude_linked_object_del((prelude_linked_object_t *) option);
                
                free(option);
        }
}




/**
 * prelude_option_set_warnings;
 * @new: bitwise OR of #prelude_option_warning_t.
 * @old_warnings: Pointer where to store the old #prelude_option_warning_t to.
 *
 * Set current warnings flags to @new.
 *
 * Uppon return, if not NULL, the pointer to @old_warnings is updated
 * to contain the old warnings.
 */
void prelude_option_set_warnings(prelude_option_warning_t new, prelude_option_warning_t *old_warnings) 
{
        if ( old_warnings ) 
                *old_warnings = warnings_flags;
        
        warnings_flags = new;
}


void prelude_option_set_commit_callback(prelude_option_t *opt,
     	                                int (*commit)(prelude_option_t *opt, prelude_string_t *out, void *context))
{
	opt->commit = commit;
}


void *prelude_option_get_commit_callback(prelude_option_t *opt)
{
	return opt->commit;
}


void prelude_option_set_get_callback(prelude_option_t *opt,
                                     int (*get)(prelude_option_t *opt, prelude_string_t *out, void *context))
{
        opt->get = get;
}



void *prelude_option_get_get_callback(prelude_option_t *opt)
{
        return opt->get;
}




void prelude_option_set_set_callback(prelude_option_t *opt,
                                     int (*set)(prelude_option_t *opt, const char *optarg, prelude_string_t *out, void *context))
{
        opt->set = set;
}



void *prelude_option_get_set_callback(prelude_option_t *opt)
{
        return opt->set;
}



void prelude_option_set_destroy_callback(prelude_option_t *opt,
                                         int (*destroy)(prelude_option_t *opt, prelude_string_t *out, void *context))
{
        opt->destroy = destroy;
        opt->type |= PRELUDE_OPTION_TYPE_DESTROY;
}



void *prelude_option_get_destroy_callback(prelude_option_t *opt)
{
        return opt->destroy;
}



char prelude_option_get_shortname(prelude_option_t *opt) 
{
        return opt->shortopt;
}



const char *prelude_option_get_longname(prelude_option_t *opt) 
{
        return opt->longopt;
}



void prelude_option_set_value(prelude_option_t *opt, const char *value)
{
        if ( opt->value )
                free(opt->value);
        
        opt->value = strdup(value);
}



prelude_list_t *prelude_option_get_optlist(prelude_option_t *opt)
{
        return &opt->optlist;
}



prelude_option_t *prelude_option_get_next(prelude_option_t *start, prelude_option_t *cur)
{
        prelude_list_t *tmp = (cur) ? &cur->_list : NULL;

        prelude_list_for_each_continue(&start->optlist, tmp)
                return prelude_linked_object_get_object(tmp);

        return NULL;
}



prelude_bool_t prelude_option_has_optlist(prelude_option_t *opt)
{
	return ! prelude_list_is_empty(&opt->optlist);
}



const char *prelude_option_get_value(prelude_option_t *opt)
{
        return opt->value;
}



void prelude_option_set_private_data(prelude_option_t *opt, void *data) 
{
        opt->private_data = data;
}



void *prelude_option_get_private_data(prelude_option_t *opt) 
{
        return opt->private_data;
}



int prelude_option_invoke_set(prelude_option_t *opt, const char *value, prelude_string_t *err, void **context)
{        
        if ( opt->has_arg == PRELUDE_OPTION_ARGUMENT_NONE && value ) {
                prelude_string_sprintf(err, "%s does not take an argument", opt->longopt);
                return -1;
        }
        
        if ( opt->has_arg == PRELUDE_OPTION_ARGUMENT_REQUIRED && ! value ) {
                prelude_string_sprintf(err, "%s require an argument", opt->longopt);
                return -1;
        }

        prelude_log_debug(3, "opt=%s value=%s\n", opt->longopt, value);
        
        return do_set(opt, value, err, context);
}



int prelude_option_invoke_commit(prelude_option_t *opt, const char *ctname, prelude_string_t *value, void *context)
{
	int ret;
	prelude_option_context_t *oc = NULL;
	
        if ( ! opt->commit ) 
		return 0;

        if ( opt->default_context )
                context = opt->default_context;
        
	if ( opt->type & PRELUDE_OPTION_TYPE_CONTEXT ) {
		oc = search_context(opt, ctname);
		if ( ! oc ) {
			prelude_string_sprintf(value, "could not find option with context %s[%s]",
                                               opt->longopt, ctname);
			return -1;
		}
		context = oc->data;
	}
	
        ret = opt->commit(opt, value, context);
	if ( ret < 0 )
		prelude_string_sprintf(value, "commit failed for %s", opt->longopt);
	
	return ret;
}



int prelude_option_invoke_destroy(prelude_option_t *opt, const char *ctname, prelude_string_t *value, void *context)
{
	int ret;
	prelude_option_context_t *oc = NULL;
	
        if ( ! opt->destroy ) {
                prelude_string_sprintf(value, "%s does not support destruction", opt->longopt);
		return -1;
	}

        if ( opt->default_context )
                context = opt->default_context;
        
	if ( opt->type & PRELUDE_OPTION_TYPE_CONTEXT ) {
		oc = search_context(opt, ctname);
		if ( ! oc ) {
			prelude_string_sprintf(value, "could not find option with context %s[%s]",
                                               opt->longopt, ctname);
			return -1;
		}
                
		context = oc->data;
	}
        
        ret = opt->destroy(opt, value, context);
	if ( ret < 0 ) {
		prelude_string_sprintf(value, "destruction for %s[%s] failed", opt->longopt, ctname);
		return -1;
	}
	
	if ( oc ) 
		prelude_option_context_destroy(oc);
        
	return 0;
}



int prelude_option_invoke_get(prelude_option_t *opt, const char *ctname, prelude_string_t *value, void *context)
{
	prelude_option_context_t *oc;
	
        if ( ! opt->get ) {
                prelude_string_sprintf(value, "%s doesn't support value retrieval", opt->longopt);
                return -1;
        }

        if ( opt->default_context )
                context = opt->default_context;
        
	if ( opt->type & PRELUDE_OPTION_TYPE_CONTEXT ) {
		oc = search_context(opt, ctname);
		if ( ! oc ) {
			prelude_string_sprintf(value, "could not find option with context %s[%s]",
                                               opt->longopt, ctname);
			return -1;
		}
		
		context = oc->data;
	}
        
        return opt->get(opt, value, context);
}




int prelude_option_new(prelude_option_t *parent, prelude_option_t **retopt)
{
        prelude_option_t *new;
        
        if ( ! parent ) {
                if ( ! root_optlist ) {
                        
                        root_optlist = calloc(1, sizeof(*root_optlist));
                        if ( ! root_optlist ) 
                                return prelude_error_from_errno(errno);
                        
                        root_optlist->parent = parent;
                        prelude_list_init(&root_optlist->optlist);
                        prelude_list_init(&root_optlist->context_list);
                        prelude_list_init(&root_optlist->_list);
                }
                
                parent = root_optlist;
        }

        new = *retopt = calloc(1, sizeof(**retopt));
        if ( ! new ) 
                return prelude_error_from_errno(errno);

        new->parent = parent;
        prelude_list_init(&new->optlist);
        prelude_list_init(&new->context_list);
        prelude_linked_object_add_tail(&parent->optlist, (prelude_linked_object_t *) new);

        return 0;
}



int prelude_option_new_root(prelude_option_t **retopt)
{
        int ret;
        prelude_option_t *new;
        
        ret = prelude_option_new(NULL, &new);
        if ( ret < 0 )
                return ret;

        new->longopt = NULL;
        new->type = PRELUDE_OPTION_TYPE_ROOT;

        if ( retopt )
                *retopt = new;
        
        return 0;
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



void prelude_option_set_type(prelude_option_t *opt, prelude_option_type_t type)
{
        opt->type = type;
}


prelude_option_type_t prelude_option_get_type(prelude_option_t *opt)
{
        return opt->type;
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



void prelude_option_set_default_context(prelude_option_t *opt, void *data)
{
        opt->default_context = data;
}



int prelude_option_new_context(prelude_option_t *opt, prelude_option_context_t **ctx, const char *name, void *data)
{
        prelude_option_context_t *new;
	
        new = malloc(sizeof(*new));
        if ( ! new )
                return prelude_error_from_errno(errno);

        if ( name && ! *name )
                name = DEFAULT_INSTANCE_NAME;
                
        new->data = data;
        new->name = (name) ? strdup(name) : NULL;

        if ( ! opt )
                prelude_list_init(&new->list);
        else {
        	opt->type |= PRELUDE_OPTION_TYPE_CONTEXT;
		prelude_list_add_tail(&opt->context_list, &new->list);
        }
        
        *ctx = new;
        
        return 0;
}



void prelude_option_context_destroy(prelude_option_context_t *oc)
{
        if ( ! prelude_list_is_empty(&oc->list) )
                prelude_list_del(&oc->list);

        if ( oc->name )
                free(oc->name);

        free(oc);
}



prelude_option_t *prelude_option_search(prelude_option_t *parent, const char *name,
                                        prelude_option_type_t type, int walk_children)
{
        return search_option(parent ? parent : root_optlist, name, type, walk_children);
}
