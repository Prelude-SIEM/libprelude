/*****
*
* Copyright (C) 2002, 2003 Krzysztof Zaraska <kzaraska@student.uci.agh.edu.pl>
* Copyright (C) 2003 Nicolas Delon <delon.nicolas@wanadoo.fr>
* Copyright (C) 2004 Yoann Vandoorselaere <yoann@prelude-ids.org>
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
#include <sys/types.h>
#include <stdarg.h>
#include <assert.h>

#include "libmissing.h"
#include "prelude-log.h"
#include "prelude-inttypes.h"

#include "idmef.h"
#include "idmef-util.h"
#include "idmef-criteria.h"



static int idmef_criterion_check_object_value(idmef_object_t *object,
					      idmef_criterion_value_t *value)
{
        int ret = 0;
        idmef_value_t *fixed;
	idmef_value_type_id_t type;

	type = idmef_object_get_value_type(object);

	switch ( idmef_criterion_value_get_type(value) ) {
                
	case idmef_criterion_value_type_fixed: 
		fixed = idmef_criterion_value_get_fixed(value);
		ret = (idmef_value_get_type(fixed) == type);
		break;

	case idmef_criterion_value_type_non_linear_time:
		ret = (type == IDMEF_VALUE_TYPE_TIME);
		break;
                
	}

	return ret ? 0 : -1;
}



static int is_non_linear_time_contiguous(idmef_criterion_value_non_linear_time_t *time)
{
	int time_parts[6];
	int cnt;
	int start;
	
	time_parts[0] = idmef_criterion_value_non_linear_time_get_year(time);
	time_parts[1] = idmef_criterion_value_non_linear_time_get_month(time);
	time_parts[2] = idmef_criterion_value_non_linear_time_get_mday(time);
	time_parts[3] = idmef_criterion_value_non_linear_time_get_hour(time);
	time_parts[4] = idmef_criterion_value_non_linear_time_get_min(time);
	time_parts[5] = idmef_criterion_value_non_linear_time_get_sec(time);

	if ( time_parts[0] != -1 )
		start = 0;

	else if ( time_parts[3] != -1 )
		start = 3;

	else
		return 0;

	for ( cnt = 0; cnt < start; cnt++ )
		if ( time_parts[cnt] != -1 )
			return 0;

	for ( cnt = start + 1;
	      cnt < sizeof (time_parts) / sizeof (time_parts[0]) && time_parts[cnt] != -1;
	      cnt++ );

	for ( cnt += 1; cnt < sizeof (time_parts) / sizeof (time_parts[0]); cnt++ )
		if ( time_parts[cnt] != -1 )
			return 0;

	return 1;
}



static int count_non_linear_time_parts(idmef_criterion_value_non_linear_time_t *time)
{
	return ((idmef_criterion_value_non_linear_time_get_year(time) != -1) +
		(idmef_criterion_value_non_linear_time_get_month(time) != -1) +
		(idmef_criterion_value_non_linear_time_get_yday(time) != -1) +
		(idmef_criterion_value_non_linear_time_get_mday(time) != -1) +
		(idmef_criterion_value_non_linear_time_get_wday(time) != -1) +
		(idmef_criterion_value_non_linear_time_get_hour(time) != -1) +
		(idmef_criterion_value_non_linear_time_get_min(time) != -1) +
		(idmef_criterion_value_non_linear_time_get_sec(time) != -1));
}



static int idmef_criterion_check_relation_value_non_linear_value(idmef_criterion_value_non_linear_time_t *time,
								 idmef_value_relation_t relation)
{
	int time_parts_count;

	if ( relation & IDMEF_VALUE_RELATION_SUBSTR    ||
             relation & IDMEF_VALUE_RELATION_IS_NULL   ||
             relation & IDMEF_VALUE_RELATION_IS_NOT_NULL )
		return -1;

	if ( relation == IDMEF_VALUE_RELATION_EQUAL || relation == IDMEF_VALUE_RELATION_NOT_EQUAL )
		return 0;

	time_parts_count = count_non_linear_time_parts(time);
	if ( time_parts_count == 0 )
		return -1;

	if ( ( idmef_criterion_value_non_linear_time_get_yday(time) != -1 ||
	       idmef_criterion_value_non_linear_time_get_wday(time) != -1 ||
	       idmef_criterion_value_non_linear_time_get_month(time) != -1 ||
	       idmef_criterion_value_non_linear_time_get_mday(time) != -1 ) &&
	     time_parts_count == 1 )
		return 0;

	return is_non_linear_time_contiguous(time) ? 0 : -1;
}



static int idmef_criterion_check_relation(idmef_criterion_value_t *value,
					  idmef_value_relation_t relation)
{
	switch ( idmef_criterion_value_get_type(value) ) {
	case idmef_criterion_value_type_fixed:
		return idmef_value_check_relation(idmef_criterion_value_get_fixed(value), relation);

	case idmef_criterion_value_type_non_linear_time:
		return idmef_criterion_check_relation_value_non_linear_value
			(idmef_criterion_value_get_non_linear_time(value), relation);
	}

	/* never reached */
	return -1;
}



static int idmef_criterion_check(idmef_object_t *object,
				 idmef_criterion_value_t *value,
                                 idmef_value_relation_t relation)
{
	if ( idmef_criterion_check_object_value(object, value) < 0 )
		return -1;
        
	return idmef_criterion_check_relation(value, relation);
}



idmef_criterion_t *idmef_criterion_new(idmef_object_t *object,
				       idmef_criterion_value_t *value,
                                       idmef_value_relation_t relation)
{
	idmef_criterion_t *criterion;
        
	if ( value && idmef_criterion_check(object, value, relation) < 0 )
		return NULL;

	criterion = calloc(1, sizeof(*criterion));
	if ( ! criterion ) {
		log(LOG_ERR, "memory exhausted.\n");
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
		idmef_criterion_value_destroy(criterion->value);

	free(criterion);
}



idmef_criterion_t *idmef_criterion_clone(idmef_criterion_t *criterion)
{
	idmef_object_t *object;
	idmef_criterion_value_t *value;

	if ( ! criterion )
		return NULL;

	object = idmef_object_clone(criterion->object);
	if ( ! object )
		return NULL;

	value = idmef_criterion_value_clone(criterion->value);
	if ( ! value )
		return NULL;

	return idmef_criterion_new(object, value, criterion->relation);
}



void idmef_criterion_print(const idmef_criterion_t *criterion)
{
        const char *operator;
        
        operator = idmef_value_relation_to_string(criterion->relation);
        assert(operator);

        if ( ! criterion->value ) {
	        printf("%s %s", operator, idmef_object_get_name(criterion->object));
                return;
        }

        printf("%s %s ", idmef_object_get_name(criterion->object), operator);
	idmef_criterion_value_print(criterion->value);
}



int idmef_criterion_to_string(const idmef_criterion_t *criterion, char *buffer, size_t size)
{
	int offset = 0;
        const char *operator;

        operator = idmef_value_relation_to_string(criterion->relation);
        assert(operator);

        if ( ! criterion->value ) {
                MY_SNPRINTF(buffer, size, offset, "%s %s", operator, idmef_object_get_name(criterion->object));
                return offset;
        }
        
        MY_SNPRINTF(buffer, size, offset, "%s %s ", idmef_object_get_name(criterion->object), operator);        
        MY_CONCAT(idmef_criterion_value_to_string, criterion->value, buffer, size, offset);
        
        return offset;
}



idmef_object_t *idmef_criterion_get_object(idmef_criterion_t *criterion)
{
	return criterion ? criterion->object : NULL;
}



idmef_criterion_value_t *idmef_criterion_get_value(idmef_criterion_t *criterion)
{
	return criterion ? criterion->value : NULL;
}



idmef_value_relation_t idmef_criterion_get_relation(idmef_criterion_t *criterion)
{
        return criterion->relation;
}



/* 
 * Check if message matches criterion. Returns 1 if it does, 0 if it doesn't,
 * -1 on error (e.g. a comparision with NULL)
 */
int idmef_criterion_match(idmef_criterion_t *criterion, idmef_message_t *message)
{
        int ret = 0;
	idmef_value_t *value;
	
	value = idmef_object_get(message, criterion->object);
	if ( ! value ) 
		return (criterion->relation == IDMEF_VALUE_RELATION_IS_NULL) ? 0 : -1;
        
        if ( ! criterion->value )
                return (criterion->relation == IDMEF_VALUE_RELATION_IS_NOT_NULL) ? 0 : -1; 
        
	switch ( idmef_criterion_value_get_type(criterion->value) ) {
                
        case idmef_criterion_value_type_fixed:
        	ret = idmef_value_match(value,
                                        idmef_criterion_value_get_fixed(criterion->value),
                                        criterion->relation);
		break;
		
	case idmef_criterion_value_type_non_linear_time:
		/* not supported for the moment */
                ret = -1;
		break;
	}

	idmef_value_destroy(value);

	return ret;
}



idmef_criteria_t *idmef_criteria_new(void)
{
	idmef_criteria_t *criteria;

	criteria = calloc(1, sizeof(*criteria));
	if ( ! criteria ) {
		log(LOG_ERR, "out of memory!\n");
		return NULL;
	}

        criteria->or = NULL;
        criteria->and = NULL;

	return criteria;
}



void idmef_criteria_destroy(idmef_criteria_t *criteria)
{
        if ( ! criteria )
                return;
        
        idmef_criterion_destroy(criteria->criterion);

        idmef_criteria_destroy(criteria->or);
        idmef_criteria_destroy(criteria->and);
        
	free(criteria);
}



idmef_criteria_t *idmef_criteria_clone(idmef_criteria_t *src)
{
        idmef_criteria_t *new;

        if ( ! src )
                return NULL;
        
        new = malloc(sizeof(*new));
        if ( ! new ) {
                log(LOG_ERR, "memory exhausted.\n");
                return NULL;
        }

        memcpy(new, src, sizeof(*new));
        
        new->or = idmef_criteria_clone(src->or);
        new->and = idmef_criteria_clone(src->and);
        new->criterion = idmef_criterion_clone(src->criterion);
        
        return new;
}




idmef_criteria_t *idmef_criteria_get_or(idmef_criteria_t *criteria)
{
        return criteria->or;
}



idmef_criteria_t *idmef_criteria_get_and(idmef_criteria_t *criteria)
{
        return criteria->and;
}





void idmef_criteria_print(idmef_criteria_t *criteria)
{
        if ( ! criteria )
                return;

        if ( criteria->or )
                printf("((");
        
        idmef_criterion_print(criteria->criterion);

        if ( criteria->and ) {
                printf(" && ");
                idmef_criteria_print(criteria->and);
        }
        
        if ( criteria->or ) {
                printf(") || (");
                idmef_criteria_print(criteria->or);
                printf("))");
        }
}



int idmef_criteria_to_string(idmef_criteria_t *criteria, char *buffer, size_t size)
{
	int offset = 0;

	if ( ! criteria )
		return -1;

	if ( criteria->or )
		MY_SNPRINTF(buffer, size, offset, "((");
        
        MY_CONCAT(idmef_criterion_to_string, criteria->criterion, buffer, size, offset);

        if ( criteria->and ) {
                MY_SNPRINTF(buffer, size, offset, " && ");
		MY_CONCAT(idmef_criteria_to_string, criteria->and, buffer, size, offset);
        }
        
        if ( criteria->or ) {
		MY_SNPRINTF(buffer, size, offset, ") || (");
		MY_CONCAT(idmef_criteria_to_string, criteria->or, buffer, size, offset);
		MY_SNPRINTF(buffer, size, offset, "))");
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



void idmef_criteria_or_criteria(idmef_criteria_t *criteria, idmef_criteria_t *criteria2)
{
        idmef_criteria_t *last = NULL;
        
        while ( criteria ) {
                last = criteria;
                criteria = criteria->or;
        }
        
        last->or = criteria2;
}



void idmef_criteria_and_criteria(idmef_criteria_t *criteria, idmef_criteria_t *criteria2)
{
        idmef_criteria_t *last = NULL;
        
        while ( criteria ) {
                last = criteria;

                if ( criteria->or )
                        idmef_criteria_and_criteria(criteria->or, idmef_criteria_clone(criteria2));
                
                criteria = criteria->and;
        }
        
        last->and = criteria2;
}




void idmef_criteria_set_criterion(idmef_criteria_t *criteria, idmef_criterion_t *criterion)
{
        criteria->criterion = criterion;
}



int idmef_criteria_match(idmef_criteria_t *criteria, idmef_message_t *message)
{
        int ret;
        idmef_criteria_t *next;
        
        ret = idmef_criterion_match(criteria->criterion, message);
        if ( ret < 0 )
                next = criteria->or;
        else
                next = criteria->and;
        
        if ( ! next )
                return ret;
                
        return idmef_criteria_match(next, message);
}
