/*****
*
* Copyright (C) 2003 Krzysztof Zaraska <kzaraska@student.uci.agh.edu.pl>
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


#ifndef _LIBPRELUDE_IDMEF_OBJECT_VALUE_H
#define _LIBPRELUDE_IDMEF_OBJECT_VALUE_H

typedef struct idmef_object_value idmef_object_value_t;


typedef struct idmef_object_value_list idmef_object_value_list_t;


idmef_object_value_t *idmef_object_value_new(idmef_object_t *object, idmef_value_t *value);


idmef_object_t *idmef_object_value_get_object(idmef_object_value_t *objval);


idmef_value_t *idmef_object_value_get_value(idmef_object_value_t *objval);


void idmef_object_value_destroy(idmef_object_value_t *objval);


idmef_object_value_list_t *idmef_object_value_list_new(void);


int idmef_object_value_list_add(idmef_object_value_list_t *list, idmef_object_value_t *objval);


idmef_object_value_t *idmef_object_value_list_get_next(idmef_object_value_list_t *list);


void idmef_object_value_list_destroy(idmef_object_value_list_t *list);


#endif /* _LIBPRELUDE_IDMEF_OBJECT_VALUE_H */

