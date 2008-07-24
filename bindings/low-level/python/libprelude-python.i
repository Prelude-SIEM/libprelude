/*****
*
* Copyright (C) 2003, 2004, 2005 PreludeIDS Technologies. All Rights Reserved.
* Author: Nicolas Delon <nicolas.delon@prelude-ids.com>
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


%{
void swig_python_raise_exception(int error)
{
	PyObject *module;
	PyObject *exception_class;
	PyObject *exception;

	module = PyImport_ImportModule("prelude");
	exception_class = PyObject_GetAttrString(module, "PreludeError");
	exception = PyObject_CallFunction(exception_class, "i", error);

	if ( exception ) {
		PyErr_SetObject(exception_class, exception);
		Py_DECREF(exception);
	}

	Py_DECREF(module);
	Py_DECREF(exception_class);
}

PyObject *swig_python_string(prelude_string_t *string)
{
	if ( string )
		return PyString_FromStringAndSize(prelude_string_get_string(string), prelude_string_get_len(string));
	else {
		Py_INCREF(Py_None);
		return Py_None;
	}
}

PyObject *swig_python_data(idmef_data_t *data)
{
	switch ( idmef_data_get_type(data) ) {
	case IDMEF_DATA_TYPE_CHAR: 
	case IDMEF_DATA_TYPE_BYTE:
		return PyString_FromStringAndSize((const char *)idmef_data_get_data(data), 1);

	case IDMEF_DATA_TYPE_CHAR_STRING: 
		return PyString_FromStringAndSize((const char *)idmef_data_get_data(data), idmef_data_get_len(data) - 1);

	case IDMEF_DATA_TYPE_BYTE_STRING:
		return PyString_FromStringAndSize((const char *)idmef_data_get_data(data), idmef_data_get_len(data));

	case IDMEF_DATA_TYPE_UINT32:
		return PyLong_FromLongLong(idmef_data_get_uint32(data));

	case IDMEF_DATA_TYPE_UINT64:
		return PyLong_FromUnsignedLongLong(idmef_data_get_uint64(data));

	case IDMEF_DATA_TYPE_FLOAT:
		return PyFloat_FromDouble((double) idmef_data_get_float(data));

	default:
		Py_INCREF(Py_None);
		return Py_None;
	}
}

%}


/**
 * Some generic type typemaps
 */


%typemap(out) uint32_t {
	$result = PyLong_FromUnsignedLong($1);
};

%typemap(out) INTPOINTER * {
	if ($1 != NULL) {
		$result = PyLong_FromLong(*$1);
	} else {
		$result = Py_None;
	}
};

%typemap(out) UINTPOINTER * {
	if ($1 != NULL) {
		$result = PyLong_FromUnsignedLong(*$1);
	} else {
		$result = Py_None;
	}
};

%typemap(out) INT64POINTER * {
	if ($1 != NULL) {
		$result = PyLong_FromLongLong(*$1);
	} else {
		$result = Py_None;
	}
};

%typemap(out) UINT64POINTER * {
	if ($1 != NULL) {
		$result = PyLong_FromUnsignedLongLong(*$1);
	} else {
		$result = Py_None;
	}
};


/* This typemap is used to allow NULL pointers in _get_next_* functions
 */
%typemap(in) SWIGTYPE *LISTEDPARAM {
	if ( $input == Py_None ) {
		$1 = NULL;
	} else {
		if ( SWIG_ConvertPtr($input, (void **)&$1, $1_descriptor, SWIG_POINTER_EXCEPTION|0) )
			return NULL;
	}
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

%typemap(freearg) const char * "";

%typemap(in) const unsigned char * {
	if ( PyString_Check($input) )
		$1 = (unsigned char *) PyString_AsString($input);

	else {
		PyErr_Format(PyExc_TypeError,
			     "expected string, %s found", $input->ob_type->tp_name);
		return NULL;
	}
		
};


%typemap(out) unsigned char *idmef_data_get_byte_string {
	if ( $1 ) 
		$result = PyString_FromStringAndSize((char *) $1, idmef_data_get_len(arg1));
	else
		$result = Py_BuildValue((char *) "");
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
%exception {
   Py_BEGIN_ALLOW_THREADS
   $function
   Py_END_ALLOW_THREADS
}


%typemap(in) (char *data, size_t len) {
	if ( ! PyString_Check($input) ) {
		PyErr_SetString(PyExc_ValueError, "Expected a string");
    		return NULL;
  	}

	$1 = PyString_AsString($input);
	$2 = PyString_Size($input);
};

%typemap(in) (const char *data, size_t len) {
	if ( ! PyString_Check($input) ) {
		PyErr_SetString(PyExc_ValueError, "Expected a string");
    		return NULL;
  	}

	$1 = PyString_AsString($input);
	$2 = PyString_Size($input);
};

%typemap(in) (unsigned char *data, size_t len) {
	if ( ! PyString_Check($input) ) {
		PyErr_SetString(PyExc_ValueError, "Expected a string");
    		return NULL;
  	}

	$1 = (unsigned char *) PyString_AsString($input);
	$2 = PyString_Size($input);
};

%typemap(in) (const unsigned char *data, size_t len) {
	if ( ! PyString_Check($input) ) {
		PyErr_SetString(PyExc_ValueError, "Expected a string");
    		return NULL;
  	}

	$1 = (unsigned char *) PyString_AsString($input);
	$2 = PyString_Size($input);	
};

%typemap(in) (const void *data, size_t len) {
	if ( ! PyString_Check($input) ) {
		PyErr_SetString(PyExc_ValueError, "Expected a string");
    		return NULL;
  	}

	$1 = PyString_AsString($input);
	$2 = PyString_Size($input);
};


%typemap(in) (uint64_t *target_id, size_t size) {
	int i;
	$2 = PyList_Size($input);
	$1 = malloc($2 * sizeof(uint64_t));
	for ( i = 0; i < $2; i++ ) {
		PyObject *o = PyList_GetItem($input, i);
		if ( PyInt_Check(o) ) 
			$1[i] = (unsigned long) PyInt_AsLong(o);
		else
			$1[i] = PyLong_AsUnsignedLongLong(o);
	}
};


%typemap(freearg) (uint64_t *target_id, size_t size) {
	free($1);
};


%typemap(out) FUNC_NO_ERROR {
	$result = PyInt_FromLong($1);
};


%typemap(out) int {
	if ( $1 < 0 ) {
		swig_python_raise_exception($1);
		$result = NULL;
	} else {
		$result = PyInt_FromLong($1);
	}
};



%typemap(out) int32_t {
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
			value_obj = SWIG_NewPointerObj((void *) * $3, SWIG_TypeQuery("prelude_option_t *"), 0);
			break;
		default:
			value_obj = PyString_FromString(* $3);
			break;
		}
	} else {
		value_obj = Py_None;
		Py_INCREF(Py_None);
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


%typemap(out) prelude_string_t * {
	$result = swig_python_string($1);
};


%typemap(out) idmef_data_t * {
	$result = swig_python_data($1);
};


%typemap(out) void * idmef_value_get_object {
	void *swig_type;

	swig_type = swig_idmef_value_get_descriptor(arg1);
	if ( ! swig_type ) {
		$result = Py_None;
		Py_INCREF(Py_None);
	} else {
		$result = SWIG_NewPointerObj($1, swig_type, 0);
	}
};


%clear idmef_value_t **ret;
%typemap(argout) idmef_value_t **ret {
	if ( PyInt_AsLong($result) == 0 ) {
		$result = Py_None;
		Py_INCREF(Py_None);

	} else if ( PyInt_AsLong($result) > 0 ) {
		$result = SWIG_NewPointerObj((void *) * $1, $*1_descriptor, 0);
	}
};


%typemap(in, numinputs=0) prelude_string_t *out {
	int ret;
	
	ret = prelude_string_new(&($1));
	if ( ret < 0 ) {
		swig_python_raise_exception(ret);
		return NULL;
	}
};

%typemap(argout) prelude_string_t *out {
	if ( result >= 0 )
		$result = PyString_FromStringAndSize(prelude_string_get_string($1), prelude_string_get_len($1));

	prelude_string_destroy($1);
};


%typemap(in, numinputs=0) prelude_msg_t **outmsg ($*1_type tmp) {
	tmp = NULL;
	$1 = ($1_ltype) &tmp;
};

%typemap(in, numinputs=0) prelude_connection_t **outconn ($*1_type tmp) {
	tmp = NULL;
	$1 = ($1_ltype) &tmp;
};


%typemap(in, numinputs=0) SWIGTYPE **OUTPARAM ($*1_type tmp) {
	$1 = ($1_ltype) &tmp;
};



%typemap(argout) SWIGTYPE **OUTPARAM {
	if ( result >= 0 )
		$result = SWIG_NewPointerObj((void *) * $1, $*1_descriptor, 0);
};


%typemap(in) SWIGTYPE *INPARAM {
	if ( $input == Py_None )
		return NULL;

	if ( SWIG_ConvertPtr($input, (void **)&arg$argnum, $1_descriptor, SWIG_POINTER_EXCEPTION|0) )
		return NULL;
}


/*
 *
 */
%apply FUNC_NO_ERROR {
	int idmef_additional_data_compare,
	int idmef_reference_compare,
	int idmef_classification_compare,
	int idmef_user_id_compare,
	int idmef_user_compare,
	int idmef_address_compare,
	int idmef_process_compare,
	int idmef_web_service_compare,
	int idmef_snmp_service_compare,
	int idmef_service_compare,
	int idmef_node_compare,
	int idmef_source_compare,
	int idmef_file_access_compare,
	int idmef_inode_compare,
	int idmef_linkage_compare,
	int idmef_checksum_compare,
	int idmef_file_compare,
	int idmef_target_compare,
	int idmef_analyzer_compare,
	int idmef_alertident_compare,
	int idmef_impact_compare,
	int idmef_action_compare,
	int idmef_confidence_compare,
	int idmef_assessment_compare,
	int idmef_tool_alert_compare,
	int idmef_correlation_alert_compare,
	int idmef_overflow_alert_compare,
	int idmef_alert_compare,
	int idmef_heartbeat_compare,
	int idmef_message_compare,
	int prelude_string_compare,
	int idmef_data_compare,
	int idmef_time_compare,
	int idmef_path_compare,
	int idmef_path_ncompare
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
