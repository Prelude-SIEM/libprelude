/*****
*
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

#ifdef SWIGPERL5
%module Prelude
#endif /* SWIGPERL5 */

%{
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <inttypes.h>

#include "prelude-io.h"
#include "prelude-message.h"
#include "prelude-message-buffered.h"
#include "prelude-client.h"
#include "idmef.h"
#include "sensor.h"
#include "idmef-value-object.h"
#include "idmef-object-value.h"
#include "idmef-message-send.h"
#include "idmef-tree-print.h"
#include "idmef-tree-to-string.h"
#include "idmef-util.h"

static	int prelude_alert_fill_infos(idmef_message_t *message)
{
	idmef_alert_t *alert;
	idmef_analyzer_t *analyzer;
	idmef_time_t *create_time;
	int retval;

	alert = idmef_message_new_alert(message);
	if ( ! alert )
		return -1;

	create_time = idmef_time_new_gettimeofday();
	if ( ! create_time )
		return -1;

	idmef_alert_set_create_time(alert, create_time);

	analyzer = idmef_alert_new_analyzer(alert);
	if ( ! analyzer )
		return -1;

	return prelude_analyzer_fill_infos(analyzer);
}

static	int prelude_heartbeat_fill_infos(idmef_message_t *message)
{
	idmef_heartbeat_t *heartbeat;
	idmef_analyzer_t *analyzer;
	idmef_time_t *create_time;

	heartbeat = idmef_message_new_heartbeat(message);
	if ( ! heartbeat )
		return -1;

	create_time = idmef_time_new_gettimeofday();
	if ( ! create_time )
		return -1;

	idmef_heartbeat_set_create_time(heartbeat, create_time);

	analyzer = idmef_heartbeat_new_analyzer(heartbeat);
	if ( ! analyzer )
		return -1;

	return prelude_analyzer_fill_infos(analyzer);
}

%}

typedef char int8_t;
typedef unsigned char uint8_t;
typedef short int16_t;
typedef unsigned short uint16_t;
typedef int int32_t;
typedef unsigned int uint32_t;
typedef long long int64_t;
typedef unsigned long long uint64_t;

/*
 * Grab from swig python documentation
 */

%typemap(python, in) char ** {
	/* Check if is a list */
	if ( PyList_Check($input) ) {
		int size = PyList_Size($input);
		int i = 0;

		$1 = (char **) malloc((size+1) * sizeof(char *));
		for ( i = 0; i < size; i++ ) {
			PyObject *o = PyList_GetItem($input,i);
			if ( PyString_Check(o) )
				$1[i] = PyString_AsString(PyList_GetItem($input, i));
			else {
				PyErr_SetString(PyExc_TypeError, "list must contain strings");
				free($1);
				return NULL;
			}
		}
		$1[i] = 0;
	} else {
		PyErr_SetString(PyExc_TypeError, "not a list");
		return NULL;
	}
};

%typemap(python, in) const char * {
	if ( $input == Py_None )
		$1 = NULL;
	else if ( PyString_Check($input) )
		$1 = PyString_AsString($input);
	else {
		PyErr_Format(PyExc_TypeError,
			     "expected None or string, %s found", $input->ob_type->tp_name);
		return NULL;
	}
};

%typemap(python, in) const unsigned char * {
	if ( PyString_Check($input) )
		$1 = PyString_AsString($input);

	else {
		PyErr_Format(PyExc_TypeError,
			     "expected string, %s found", $input->ob_type->tp_name);
		return NULL;
	}
		
};

%typemap(python, out) unsigned char * {
	$result = $1 ? PyString_FromString($1) : Py_BuildValue((char *) "");
};



%typemap(perl5, in) char ** {
	AV *tempav;
	I32 len;
	int i;
	SV  **tv;

	if ( ! SvROK($input) )
	    croak("Argument $argnum is not a reference.");

        if ( SvTYPE(SvRV($input)) != SVt_PVAV )
	    croak("Argument $argnum is not an array.");

        tempav = (AV*) SvRV($input);
	len = av_len(tempav);
	$1 = (char **) malloc((len+2)*sizeof(char *));
	if ( ! $1 )
		croak("out of memory\n");
	for (i = 0; i <= len; i++) {
	    tv = av_fetch(tempav, i, 0);	
	    $1[i] = (char *) SvPV_nolen(*tv);
        }
	$1[i] = NULL;
};


%typemap(perl5, in) uint8_t {
	$1 = (uint8_t) SvIV($input);
};

/*
 * FIXME: this piece of code assume that data is a string
 */

%typemap(perl5, out) unsigned char * {
	$result = $1 ? sv_2mortal(newSVpv($1, 0)) : NULL;
	argvi++;
};

%typemap(perl5, in) uint64_t {
	if ( SvIOK($input) ) {
		$1 = (uint64_t) SvIV($input);

	} else {
		if ( sscanf(SvPV_nolen($input), "%llu", &($1)) < 1 ) {
			croak("Argument %s is not an unsigned 64 bits integer\n", SvPV_nolen($input));
		}
	}
};

%typemap(perl5, out) uint64_t {
	char tmp[32];
	int len;

	len = snprintf(tmp, sizeof (tmp), "%llu", $1);

	if ( len >= 0 && len < sizeof (tmp) ) {
		$result = sv_2mortal(newSVpv(tmp, len));
		argvi++;
	}
};



%include "../../src/include/prelude-client.h"
%include "../../src/include/sensor.h"
%include "../../src/include/idmef-tree-wrap.h"
%include "../../src/include/idmef-message.h"
%include "../../src/include/idmef-value.h"
%include "../../src/include/idmef-object.h"
%include "../../src/include/idmef-time.h"
%include "../../src/include/idmef-data.h"
%include "../../src/include/idmef-string.h"
%include "../../src/include/idmef-criteria.h"
%include "../../src/include/idmef-criteria-string.h"
%include "../../src/include/idmef-value-object.h"
%include "../../src/include/idmef-object-value.h"
%include "../../src/include/prelude-message-buffered.h"
%include "../../src/include/idmef-message-send.h"
%include "../../src/include/idmef-tree-print.h"
%include "../../src/include/idmef-tree-to-string.h"
%include "../../src/include/idmef-value-type.h"
%include "../../src/include/idmef-util.h"
%include "../../src/include/idmef-type.h"

int prelude_alert_fill_infos(idmef_message_t *message);
int prelude_heartbeat_fill_infos(idmef_message_t *message);
