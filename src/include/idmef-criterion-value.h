/*****
*
* Copyright (C) 2004 Nicolas Delon <delon.nicolas@wanadoo.fr>
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

#ifndef _LIBPRELUDE_IDMEF_CRITERION_VALUE_H
#define _LIBPRELUDE_IDMEF_CRITERION_VALUE_H


typedef struct idmef_criterion_value_non_linear_time idmef_criterion_value_non_linear_time_t;

typedef enum {
	idmef_criterion_value_type_fixed = 0,
	idmef_criterion_value_type_non_linear_time = 1
}	idmef_criterion_value_type_t;

typedef struct idmef_criterion_value idmef_criterion_value_t;


idmef_criterion_value_non_linear_time_t *idmef_criterion_value_non_linear_time_new(void);
idmef_criterion_value_non_linear_time_t *idmef_criterion_value_non_linear_time_new_string(const char *buf);
void idmef_criterion_value_non_linear_time_destroy(idmef_criterion_value_non_linear_time_t *time);
idmef_criterion_value_non_linear_time_t *idmef_criterion_value_non_linear_time_clone(const idmef_criterion_value_non_linear_time_t *src);
void idmef_criterion_value_non_linear_time_set_year(idmef_criterion_value_non_linear_time_t *time,
						    int year);
void idmef_criterion_value_non_linear_time_set_month(idmef_criterion_value_non_linear_time_t *time,
						     int month);
void idmef_criterion_value_non_linear_time_set_yday(idmef_criterion_value_non_linear_time_t *time,
						    int yday);
void idmef_criterion_value_non_linear_time_set_mday(idmef_criterion_value_non_linear_time_t *time,
						    int mday);
void idmef_criterion_value_non_linear_time_set_wday(idmef_criterion_value_non_linear_time_t *time,
						    int wday);
void idmef_criterion_value_non_linear_time_set_hour(idmef_criterion_value_non_linear_time_t *time,
						    int hour);
void idmef_criterion_value_non_linear_time_set_min(idmef_criterion_value_non_linear_time_t *time,
						   int min);
void idmef_criterion_value_non_linear_time_set_sec(idmef_criterion_value_non_linear_time_t *time,
						   int sec);
int idmef_criterion_value_non_linear_time_get_year(idmef_criterion_value_non_linear_time_t *time);
int idmef_criterion_value_non_linear_time_get_month(idmef_criterion_value_non_linear_time_t *time);
int idmef_criterion_value_non_linear_time_get_yday(idmef_criterion_value_non_linear_time_t *time);
int idmef_criterion_value_non_linear_time_get_mday(idmef_criterion_value_non_linear_time_t *time);
int idmef_criterion_value_non_linear_time_get_wday(idmef_criterion_value_non_linear_time_t *time);
int idmef_criterion_value_non_linear_time_get_hour(idmef_criterion_value_non_linear_time_t *time);
int idmef_criterion_value_non_linear_time_get_min(idmef_criterion_value_non_linear_time_t *time);
int idmef_criterion_value_non_linear_time_get_sec(idmef_criterion_value_non_linear_time_t *time);
void idmef_criterion_value_non_linear_time_print(idmef_criterion_value_non_linear_time_t *time);
int idmef_criterion_value_non_linear_time_to_string(idmef_criterion_value_non_linear_time_t *time,
						    char *buffer, size_t size);


idmef_criterion_value_t *idmef_criterion_value_new_fixed(idmef_value_t *value);
idmef_criterion_value_t *idmef_criterion_value_new_non_linear_time(idmef_criterion_value_non_linear_time_t *time);
idmef_criterion_value_t *idmef_criterion_value_new_generic(idmef_object_t *object, const char *buf);
void idmef_criterion_value_destroy(idmef_criterion_value_t *value);
idmef_criterion_value_t *idmef_criterion_value_clone(const idmef_criterion_value_t *src);
idmef_criterion_value_type_t idmef_criterion_value_get_type(const idmef_criterion_value_t *value);
idmef_value_t *idmef_criterion_value_get_fixed(idmef_criterion_value_t *value);
idmef_criterion_value_non_linear_time_t *idmef_criterion_value_get_non_linear_time(idmef_criterion_value_t *time);
void idmef_criterion_value_print(idmef_criterion_value_t *value);
int idmef_criterion_value_to_string(idmef_criterion_value_t *value, char *buffer, size_t size);



#endif /* _LIBPRELUDE_IDMEF_CRITERION_VALUE_H */
