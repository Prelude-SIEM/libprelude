/*****
*
* Copyright (C) 2003-2005 Nicolas Delon <nicolas@prelude-ids.org>
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

%{
void swig_python_raise_exception(int error)
{
	PyObject *module;
	PyObject *exception_class;
	PyObject *exception;

	module = PyImport_ImportModule("prelude");
	exception_class = PyObject_GetAttrString(module, "PreludeError");
	exception = PyObject_CallFunction(exception_class, "i", error);

	PyErr_SetObject(exception_class, exception);

	Py_DECREF(module);
	Py_DECREF(exception_class);
	Py_DECREF(exception);
}

PyObject *swig_python_string(prelude_string_t *string)
{
	return PyString_FromStringAndSize(prelude_string_get_string(string), prelude_string_get_len(string));
}

PyObject *swig_python_data(idmef_data_t *data)
{
	switch ( idmef_data_get_type(data) ) {
	case IDMEF_DATA_TYPE_CHAR: case IDMEF_DATA_TYPE_BYTE:
		return PyString_FromStringAndSize(idmef_data_get_data(data), 1);

	case IDMEF_DATA_TYPE_CHAR_STRING: 
		return PyString_FromStringAndSize(idmef_data_get_data(data), idmef_data_get_len(data) - 1);

	case IDMEF_DATA_TYPE_BYTE_STRING:
		return PyString_FromStringAndSize(idmef_data_get_data(data), idmef_data_get_len(data));

	case IDMEF_DATA_TYPE_UINT32:
		return PyLong_FromLongLong(idmef_data_get_uint32(data));

	case IDMEF_DATA_TYPE_UINT64:
		return PyLong_FromUnsignedLongLong(idmef_data_get_uint64(data));

	case IDMEF_DATA_TYPE_FLOAT:
		return PyFloat_FromDouble((double) idmef_data_get_float(data));

	default:
		return NULL;
	}
}
%}


/**
 * Some generic type typemaps
 */


%typemap(out) uint32_t {
	$result = PyLong_FromUnsignedLong($1);
};


%typemap(in) const char * {
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


%typemap(in) const unsigned char * {
	if ( PyString_Check($input) )
		$1 = PyString_AsString($input);

	else {
		PyErr_Format(PyExc_TypeError,
			     "expected string, %s found", $input->ob_type->tp_name);
		return NULL;
	}
		
};


%typemap(out) unsigned char * {
	$result = $1 ? PyString_FromString($1) : Py_BuildValue((char *) "");
};


%typemap(in) char **argv {
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


%typemap(freearg) char **argv {
	free($1);
};








/**
 * Prelude specific typemaps
 */


%typemap(in) (const void *data, size_t len) {
	$1 = PyString_AsString($input);
	$2 = PyString_Size($input);
};


%typemap(in) (uint64_t *target_id, size_t size) {
	int i;
	$2 = PyList_Size($input);
	$1 = malloc($2 * sizeof (uint64_t));
	for ( i = 0; i < $2; i++ ) {
		PyObject *o = PyList_GetItem($input, i);
		$1[i] = PyLong_AsUnsignedLongLong(o);
	}
};


%typemap(freearg) uint64_t *target_id {
	free($1);
};


%typemap(out) int {
	if ( $1 < 0 ) {
		swig_python_raise_exception($1);
		return NULL;
	}

	$result = PyInt_FromLong($1);
};


%typemap(in) const time_t * (time_t tmp) {
	if ( PyInt_Check($input) )
		tmp = (time_t) PyInt_AsLong($input);
	else if ( PyLong_Check($input) )
		tmp = (time_t) PyLong_AsUnsignedLong($input);
	else {
		PyErr_Format(PyExc_TypeError,
			     "expected int or long, %s found", $input->ob_type->tp_name);
		return NULL;
	}

	$1 = &tmp;
};


%typemap(argout) (uint64_t *source_id, uint32_t *request_id, void **value) {
	int ret = PyInt_AsLong($result);
	PyObject *tuple = PyTuple_New(4);
	PyObject *value_obj = Py_None;

	PyTuple_SetItem(tuple, 0, PyLong_FromUnsignedLongLong(*$1));
	PyTuple_SetItem(tuple, 1, PyLong_FromUnsignedLong(*$2));
	PyTuple_SetItem(tuple, 2, PyInt_FromLong(ret));

	if ( *$3 ) {
		switch ( ret ) {
		case PRELUDE_OPTION_REPLY_TYPE_LIST:
			value_obj = SWIG_NewPointerObj((void *) * $3, SWIGTYPE_p_prelude_option_t, 0);
			break;
		default:
			value_obj = PyString_FromString(* $3);
			break;
		}
	}

	PyTuple_SetItem(tuple, 3, value_obj);

	$result = tuple;
};


%typemap(in) int *argc (int tmp) {
	tmp = PyInt_AsLong($input);
	$1 = &tmp;
};


%typemap(in) prelude_string_t * {
	int ret;

	ret = prelude_string_new_dup_fast(&($1), PyString_AsString($input), PyString_Size($input));
	if ( ret < 0 ) {
		swig_python_raise_exception(ret);
		return NULL;
	}
};


%typemap(python, out) prelude_string_t * {
	$result = swig_python_string($1);
};


%typemap(python, out) idmef_data_t * {
	$result = swig_python_data($1);
};


%clear idmef_value_t **ret;
%typemap(python, argout) idmef_value_t **ret {
	if ( PyInt_AsLong($result) == 0 ) {
		$result = Py_None;

	} else {
		$result = SWIG_NewPointerObj((void *) * $1, SWIGTYPE_p_idmef_value_t, 0);
	}
};


%typemap(python, in, numinputs=0) prelude_string_t *out {
	int ret;
	
	ret = prelude_string_new(&($1));
	if ( ret < 0 ) {
		swig_python_raise_exception(ret);
		return NULL;
	}
};
%typemap(python, argout) prelude_string_t *out {
	$result = PyString_FromStringAndSize(prelude_string_get_string($1), prelude_string_get_len($1));
	prelude_string_destroy($1);
};



%typemap(in, numinputs=0) SWIGTYPE **OUTPARAM ($*1_type tmp) {
	$1 = ($1_ltype) &tmp;
};



%typemap(argout) SWIGTYPE **OUTPARAM {
	$result = SWIG_NewPointerObj((void *) * $1, $*1_descriptor, 0);
};


%apply SWIGTYPE **OUTPARAM {
	prelude_client_t **,
	prelude_client_profile_t **, 
	prelude_msgbuf_t **,
	prelude_msg_t **,
	prelude_connection_t **,
	idmef_path_t **,
	idmef_value_t **,
	idmef_criteria_t **,
	idmef_time_t **,
        idmef_data_t **
};


%apply SWIGTYPE **OUTPARAM {
	idmef_additional_data_t **,
	idmef_reference_t **,
	idmef_classification_t **,
	idmef_user_id_t **,
	idmef_user_t **,
	idmef_address_t **,
	idmef_process_t **,
	idmef_web_service_t **,
	idmef_snmp_service_t **,
	idmef_service_t **,
	idmef_node_t **,
	idmef_source_t **,
	idmef_file_access_t **,
	idmef_inode_t **,
	idmef_linkage_t **,
	idmef_checksum_t **,
	idmef_file_t **,
	idmef_target_t **,
	idmef_analyzer_t **,
	idmef_alertident_t **,
	idmef_impact_t **,
	idmef_action_t **,
	idmef_confidence_t **,
	idmef_assessment_t **,
	idmef_tool_alert_t **,
	idmef_correlation_alert_t **,
	idmef_overflow_alert_t **,
	idmef_alert_t **,
	idmef_heartbeat_t **,
	idmef_message_t **
};



%pythoncode %{
class PreludeError(Exception):
    def __init__(self, errno, strerror=None):
	self.errno = errno
	self._strerror = strerror
    
    def __str__(self):
	if self._strerror:
	    return self._strerror
        return prelude_strerror(self.errno)
%}
