/*****
*
* Copyright (C) 1998 - 2000 Yoann Vandoorselaere <yoann@mandrakesoft.com>
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

#ifndef _LIBPRELUDE_PLUGIN_COMMON_H
#define _LIBPRELUDE_PLUGIN_COMMON_H


#define PLUGIN_GENERIC                 \
        char *name; int namelen;       \
        char *author; int authorlen;   \
        char *contact; int contactlen; \
        char *desc; int desclen


typedef struct {
        PLUGIN_GENERIC;
} plugin_generic_t;


#define plugin_name(p) (p)->name
#define plugin_name_len(p) (p)->namelen

#define plugin_author(p) (p)->author
#define plugin_author_len(p) (p)->authorlen

#define plugin_contact(p) (p)->contact
#define plugin_contact_len(p) (p)->contactlen

#define plugin_desc(p) (p)->desc
#define plugin_desc_len(p) (p)->desclen


/*
 *
 */

#define plugin_set_name(p, str) plugin_name(p) = (str); \
                                plugin_name_len(p) = sizeof((str))

#define plugin_set_author(p, str) plugin_author(p) = (str); \
                                  plugin_author_len(p) = sizeof((str))

#define plugin_set_contact(p, str) plugin_contact(p) = (str); \
                                   plugin_contact_len(p) = sizeof((str))

#define plugin_set_desc(p, str) plugin_desc(p) = (str); \
                                plugin_desc_len(p) = sizeof((str))


/*
 * Plugin need to call this function in order to get registered.
 */
int plugin_subscribe(plugin_generic_t *plugin);


int plugin_unsubscribe(plugin_generic_t *plugin);


/*
 * Return a valide not used plugin identity.
 */
int plugin_request_new_id(void);

#endif /* _LIBPRELUDE_PLUGIN_COMMON_H */

