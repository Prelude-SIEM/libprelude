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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <sys/types.h>
#include <stdarg.h>

#include "list.h"
#include "prelude-log.h"

#include "idmef-string.h"
#include "idmef-tree.h"
#include "idmef-util.h"
#include "idmef-value.h"
#include "idmef-object.h"
#include "idmef-message.h"

#include "idmef-criteria.h"



idmef_criterion_t *idmef_criterion_new(idmef_object_t *object, 
				       idmef_relation_t relation,
				       idmef_value_t *value)
{
	idmef_criterion_t *criterion;
	idmef_value_type_id_t type;

	criterion = calloc(1, sizeof(*criterion));
	if ( ! criterion ) {
		log(LOG_ERR, "out of memory!\n");
		return NULL;
	}

	criterion->object = object;
	criterion->relation = relation;
	criterion->value = value;

	return criterion;
}



void idmef_criterion_destroy(idmef_criterion_t *criterion)
{
	if ( ! criterion )
		return;

	idmef_object_destroy(criterion->object);

	if ( criterion->value ) /* can be NULL if relation is is_null or is_not_null */
		idmef_value_destroy(criterion->value);

	free(criterion);
}



idmef_criterion_t *idmef_criterion_clone(idmef_criterion_t *criterion)
{
	idmef_object_t *object;
	idmef_value_t *value;

	if ( ! criterion )
		return NULL;

	object = idmef_object_clone(criterion->object);
	if ( ! object )
		return NULL;

	value = idmef_value_clone(criterion->value);
	if ( ! value )
		return NULL;

	return idmef_criterion_new(object, criterion->relation, value);
}



void	idmef_criterion_print(const idmef_criterion_t *criterion)
{
	char buffer[512];

	if ( ! criterion )
		return;

	if ( criterion->relation == relation_is_null ) {
		printf("! %s", idmef_object_get_name(criterion->object));
		return;
	}

	if ( criterion->relation == relation_is_not_null ) {
		printf("%s", idmef_object_get_name(criterion->object));
		return;
	}

	printf("%s ", idmef_object_get_name(criterion->object));

	switch ( criterion->relation ) {
	case relation_substring:
		printf("substr");
		break;

	case relation_regexp:
		printf("=~");
		break;

	case relation_greater:
		printf(">");
		break;

	case relation_greater_or_equal:
		printf(">=");
		break;

	case relation_less:
		printf("<");
		break;

	case relation_less_or_equal:
		printf("<=");
		break;

	case relation_equal:
		printf("==");
		break;

	case relation_not_equal:
		printf("!=");
		break;

	default:
		/* nop */;
	}

	if ( idmef_value_to_string(criterion->value, buffer, sizeof (buffer)) >= 0 )
		printf(" %s", buffer);
}



int	idmef_criterion_to_string(const idmef_criterion_t *criterion, char *buffer, size_t size)
{
	int offset = 0;

	if ( ! criterion )
		return -1;

	if ( criterion->relation == relation_is_null ) {
		MY_SNPRINTF(buffer, size, offset, "! %s", idmef_object_get_name(criterion->object));
		return offset;
	}

	if ( criterion->relation == relation_is_not_null ) {
		MY_SNPRINTF(buffer, size, offset, "%s", idmef_object_get_name(criterion->object));
		return offset;
	}

	MY_SNPRINTF(buffer, size, offset, "%s", idmef_object_get_name(criterion->object));

	switch ( criterion->relation ) {
	case relation_substring:
		MY_SNPRINTF(buffer, size, offset, " substr ");
		break;

	case relation_regexp:
		MY_SNPRINTF(buffer, size, offset, " =~ ");
		break;

	case relation_greater:
		MY_SNPRINTF(buffer, size, offset, " > ");
		break;

	case relation_greater_or_equal:
		MY_SNPRINTF(buffer, size, offset, " >= ");
		break;

	case relation_less:
		MY_SNPRINTF(buffer, size, offset, " < ");
		break;

	case relation_less_or_equal:
		MY_SNPRINTF(buffer, size, offset, " <= ");
		break;

	case relation_equal:
		MY_SNPRINTF(buffer, size, offset, " == ");
		break;

	case relation_not_equal:
		MY_SNPRINTF(buffer, size, offset, " != ");
		break;

	default:
		/* nop */;
	}

	MY_CONCAT(idmef_value_to_string, criterion->value, buffer, size, offset);

	return offset;
}



idmef_object_t *idmef_criterion_get_object(idmef_criterion_t *criterion)
{
	return criterion ? criterion->object : NULL;
}



idmef_value_t *idmef_criterion_get_value(idmef_criterion_t *criterion)
{
	return criterion ? criterion->value : NULL;
}



idmef_relation_t idmef_criterion_get_relation(idmef_criterion_t *criterion)
{
	return criterion ? criterion->relation : relation_error;
}



/* 
 * Check if message matches criterion. Returns 1 if it does, 0 if it doesn't,
 * -1 on error (e.g. a comparision with NULL)
 */
int idmef_criterion_match(idmef_criterion_t *criterion, idmef_message_t *message)
{
	idmef_value_t *value;
	int retval;

	if ( ! criterion || ! message )
		return -1;

	value = idmef_message_get(message, criterion->object);
	if ( ! value )
		return -1;

	retval = idmef_value_match(value, criterion->value, criterion->relation);

	idmef_value_destroy(value);

	return retval;
}



idmef_criteria_t *idmef_criteria_new(void)
{
	idmef_criteria_t *criteria;

	criteria = calloc(1, sizeof(*criteria));
	if ( ! criteria ) {
		log(LOG_ERR, "out of memory!\n");
		return NULL;
	}

	INIT_LIST_HEAD(&criteria->list);
	INIT_LIST_HEAD(&criteria->criteria);

	return criteria;
}



void idmef_criteria_destroy(idmef_criteria_t *criteria)
{
	if ( ! criteria )
		return;

	if ( criteria->criterion ) {
		idmef_criterion_destroy(criteria->criterion);

	} else {
		idmef_criteria_t *entry;
		struct list_head *tmp, *n;

		list_for_each_safe(tmp, n, &criteria->criteria) {
			entry = list_entry(tmp, idmef_criteria_t, list);
			idmef_criteria_destroy(entry);
		}
	}

	list_del(&criteria->list);
	free(criteria);
}



idmef_criteria_t *idmef_criteria_clone(idmef_criteria_t *src)
{
	idmef_criteria_t *dst;
	idmef_criteria_t *ptr;

	if ( ! src )
		return NULL;

	dst = idmef_criteria_new();
	if ( ! dst ) {
		log(LOG_ERR, "could not create new criteria\n");
		return NULL;
	}

	ptr = NULL;
	while ( (ptr = idmef_criteria_get_next(src, ptr)) ) {
		if ( ptr->criterion ) {
			idmef_criterion_t *criterion;

			criterion = idmef_criterion_clone(ptr->criterion);
			if ( ! criterion )
				goto error;

			if ( idmef_criteria_add_criterion(dst, criterion, ptr->operator) < 0 )
				goto error;

		} else {
			idmef_criteria_t *sub_criteria;

			sub_criteria = idmef_criteria_clone(ptr);
			if ( ! sub_criteria )
				goto error;

			if ( idmef_criteria_add_criteria(dst, sub_criteria, ptr->operator) < 0 )
				goto error;
		}

	}

	return dst;

 error:
	idmef_criteria_destroy(dst);
	return NULL;
}



void	idmef_criteria_print(const idmef_criteria_t *criteria)
{
	idmef_criteria_t *ptr;

	ptr = NULL;
	while ( (ptr = idmef_criteria_get_next(criteria, ptr)) ) {
		if ( ptr->criterion ) {
			idmef_criterion_print(ptr->criterion);

		} else {
			printf("(");
			idmef_criteria_print(ptr);
			printf(")");
		}

		switch ( ptr->operator ) {
		case operator_and:
			printf(" && ");
			break;

		case operator_or:
			printf(" || ");
			break;

		default:
			/* nop */;
		}
	}

	fflush(stdout);
}



int	idmef_criteria_to_string(const idmef_criteria_t *criteria, char *buffer, size_t size)
{
	idmef_criteria_t *ptr;
	int offset = 0;

	ptr = NULL;
	while ( (ptr = idmef_criteria_get_next(criteria, ptr)) ) {
		if ( ptr->criterion ) {
			MY_CONCAT(idmef_criterion_to_string, ptr->criterion, buffer, size, offset);

		} else {
			MY_SNPRINTF(buffer, size, offset, "(");
			MY_CONCAT(idmef_criteria_to_string, ptr, buffer, size, offset);
			MY_SNPRINTF(buffer, size, offset, ")");
		}

		switch ( ptr->operator ) {
		case operator_and:
			MY_SNPRINTF(buffer, size, offset, " && ");
			break;

		case operator_or:
			MY_SNPRINTF(buffer, size, offset, " || ");
			break;

		default:
			/* nop */;
		}
	}

	return offset;
}



int idmef_criteria_is_criterion(idmef_criteria_t *criteria)
{
	return criteria ? (criteria->criterion != NULL) : -1;
}



idmef_criterion_t *idmef_criteria_get_criterion(idmef_criteria_t *criteria)
{
	return criteria ? criteria->criterion : NULL;
}



int idmef_criteria_add_criteria(idmef_criteria_t *criteria1, idmef_criteria_t *criteria2, idmef_operator_t operator)
{
	if ( ! criteria1 || ! criteria2 ) 
		return -1;

	if ( ! list_empty(&criteria1->criteria) ) {
		idmef_criteria_t *last_criteria;

		last_criteria = list_entry(criteria1->criteria.prev, idmef_criteria_t, list);
		last_criteria->operator = operator;
	}

	list_add_tail(&criteria2->list, &criteria1->criteria);

	return 0;
}



int idmef_criteria_add_criterion(idmef_criteria_t *criteria, idmef_criterion_t *criterion, idmef_operator_t operator)
{
	idmef_criteria_t *new_criteria;

	new_criteria = idmef_criteria_new();
	if ( ! new_criteria )
		return -1;

	new_criteria->criterion = criterion;

	return idmef_criteria_add_criteria(criteria, new_criteria, operator);
}



idmef_operator_t idmef_criteria_get_operator(idmef_criteria_t *criteria)
{
	return criteria ? criteria->operator : operator_error;
}



idmef_criteria_t *idmef_criteria_get_next(idmef_criteria_t *criteria, idmef_criteria_t *entry)
{
	if ( list_empty(&criteria->criteria) )
		return NULL;

	/* First element */
	if ( ! entry )
		return list_entry(criteria->criteria.next, idmef_criteria_t, list);

	/* Last element */
	if ( entry->list.next == &criteria->criteria )
		return NULL;

	return list_entry(entry->list.next, idmef_criteria_t, list);
}



int idmef_criteria_match(idmef_criteria_t *criteria, idmef_message_t *message)
{
	idmef_criteria_t *criteria_ptr;
	int match;

	criteria_ptr = NULL;
	while ( (criteria_ptr = idmef_criteria_get_next(criteria, criteria_ptr)) ) {

		match = (criteria_ptr->criterion ?
			 idmef_criterion_match(criteria_ptr->criterion, message) :
			 idmef_criteria_match(criteria_ptr, message));

		if ( match < 0 )
			return -1;

		if ( match ) {
			if ( criteria_ptr->operator == operator_or )
				return 1;

		} else {
			if ( criteria_ptr->operator == operator_and )
				return 0;
		}
	}

	return 1;
}
