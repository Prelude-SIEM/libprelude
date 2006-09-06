/*****
*
* Copyright (C) 2000-2005 PreludeIDS Technologies. All Rights Reserved.
* Author: Yoann Vandoorselaere <yoann.v@prelude-ids.com>
*
* This file is part of the Prelude library.
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

#ifndef _LIBPRELUDE_CONFIG_ENGINE_H
#define _LIBPRELUDE_CONFIG_ENGINE_H

typedef struct config config_t;

int _config_get_next(config_t *cfg, char **section,
                     char **entry, char **value, unsigned int *line);

int _config_get_section(config_t *cfg, const char *section, unsigned int *line);

char *_config_get(config_t *cfg, const char *section, const char *entry, unsigned int *line);

int _config_set(config_t *cfg, const char *section, const char *entry, const char *val, unsigned int *line);

int _config_open(config_t **ret, const char *filename);

int _config_close(config_t *cfg);

int _config_del(config_t *cfg, const char *section, const char *entry);

#endif /* _LIBPRELUDE_CONFIG_ENGINE_H */

