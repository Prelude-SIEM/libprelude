/*****
*
* Copyright (C) 1998 - 2000, 2002 Yoann Vandoorselaere <yoann@mandrakesoft.com>
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

#ifndef _PLUGIN_COMMON_PRV_H
#define _PLUGIN_COMMON_PRV_H

#include "list.h"
#include <inttypes.h>

typedef struct 
{
        /*
         * List members for internal list (inside library).
         */
        struct list_head int_list;

        /*
         * List members for external list (outside library).
         */
        struct list_head ext_list;

        int already_used;

        /*
         * pointer to the plugin
         */ 
        plugin_generic_t *plugin;

        const char *infos;
        
        /*
         * Plugin running time and count.
         */
        double p_time;
        uint32_t p_count;

        /*
         * Pointer on the parent of this container (on the entry in the lib).
         */
        void *parent;
} plugin_container_t;


/*
 * Load all plugins in directory 'dirname'.
 * The CB arguments will be called for each plugin that register
 * (using the plugin_register function), then the application will
 * have the ability to use plugin_register_for_use to tell it want
 * to use this plugin.
 */
int plugin_load_from_dir(const char *dirname, int argc, char **argv,
                         int (*subscribe)(plugin_container_t *p), void (*unsubscribe)(plugin_container_t *pc));


/*
 * Call this if you want to use this plugin.
 */ 
int plugin_add(plugin_container_t *pc, struct list_head *h, const char *infos);

void plugin_del(plugin_container_t *pc);


/*
 * Print stats for the plugin contained in this container.
 */
void plugin_print_stats(plugin_container_t *pc);


/*
 * Print stats for all plugins.
 */
void plugins_print_stats(void);


/*
 * Print possible options for each plugins.
 */
void plugins_print_opts(const char *dirname);



/*
 *
 */
int plugin_get_highest_id(void);

/*
 * Macro used to start a plugin.
 */
#define plugin_run(pc, type, member, arg...) do {                              \
        struct timeval start, end;                                             \
                                                                               \
        gettimeofday(&start, NULL);                                            \
        ((type *)pc->plugin)->member(arg);                                     \
        gettimeofday(&end, NULL);                                              \
        pc->p_time += (double) end.tv_sec + (double) (end.tv_usec * 1e-6);     \
        pc->p_time -= (double) start.tv_sec + (double) (start.tv_usec * 1e-6); \
        pc->p_count++;                                                         \
} while (0)


/*
 * Macro used to start a plugin.
 */
#define plugin_run_with_return_value(pc, type, member, ret, arg...) do {       \
        struct timeval start, end;                                             \
                                                                               \
        gettimeofday(&start, NULL);                                            \
        ret = ((type *)pc->plugin)->member(arg);                               \
        gettimeofday(&end, NULL);                                              \
        pc->p_time += (double) end.tv_sec + (double) (end.tv_usec * 1e-6);     \
        pc->p_time -= (double) start.tv_sec + (double) (start.tv_usec * 1e-6); \
        pc->p_count++;                                                         \
} while (0)


#endif
