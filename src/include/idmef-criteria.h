/*****
*
* Copyright (C) 2002,2003 Krzysztof Zaraska <kzaraska@student.uci.agh.edu.pl>
* Copyright (C) 2003 Nicolas Delon <delon.nicolas@wanadoo.fr>
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

#ifndef _LIBPRELUDE_IDMEF_CRITERIA_H
#define _LIBPRELUDE_IDMEF_CRITERIA_H

typedef enum {
	operator_error = 0,
	operator_and = 1,
	operator_or = 2
} idmef_operator_t;


/*
 * These structures are public because both idmef-criteria.c and
 * idmef-criteria-string.[yc] need them, otherwise they must not
 * be accessed directly
 */

typedef struct idmef_criterion {
	idmef_object_t *object;
	idmef_criterion_value_t *value;
	idmef_relation_t relation;
} idmef_criterion_t;


typedef struct idmef_criteria {
	struct list_head list;

	idmef_operator_t operator;

	idmef_criterion_t *criterion;
	struct list_head criteria;
}	idmef_criteria_t;


idmef_criterion_t *idmef_criterion_new(idmef_object_t *object, idmef_relation_t relation, idmef_criterion_value_t *value);
void idmef_criterion_destroy(idmef_criterion_t *criterion);
idmef_criterion_t *idmef_criterion_clone(idmef_criterion_t *criterion);
void idmef_criterion_print(const idmef_criterion_t *criterion);
int idmef_criterion_to_string(const idmef_criterion_t *criterion, char *buffer, size_t size);
idmef_object_t *idmef_criterion_get_object(idmef_criterion_t *criterion);
idmef_criterion_value_t *idmef_criterion_get_value(idmef_criterion_t *criterion);
idmef_relation_t idmef_criterion_get_relation(idmef_criterion_t *criterion);
int idmef_criterion_match(idmef_criterion_t *criterion, idmef_message_t *message);

idmef_criteria_t *idmef_criteria_new(void);
void idmef_criteria_destroy(idmef_criteria_t *criteria);
idmef_criteria_t *idmef_criteria_clone(idmef_criteria_t *criteria);
void idmef_criteria_print(idmef_criteria_t *criteria);
int idmef_criteria_to_string(idmef_criteria_t *criteria, char *buffer, size_t size);
int idmef_criteria_is_criterion(idmef_criteria_t *criteria);
idmef_criterion_t *idmef_criteria_get_criterion(idmef_criteria_t *criteria);
int idmef_criteria_add_criteria(idmef_criteria_t *criteria1, idmef_criteria_t *criteria2, idmef_operator_t operator);
int idmef_criteria_add_criterion(idmef_criteria_t *criteria, idmef_criterion_t *criterion, idmef_operator_t operator);
idmef_operator_t idmef_criteria_get_operator(idmef_criteria_t *criteria);
idmef_criteria_t *idmef_criteria_get_next(idmef_criteria_t *criteria, idmef_criteria_t *entry);
int idmef_criteria_match(idmef_criteria_t *criteria, idmef_message_t *message);


#endif /* _LIBPRELUDE_IDMEF_CRITERIA_H */
