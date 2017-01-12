/*****
*
* Copyright (C) 2005-2017 CS-SI. All Rights Reserved.
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
* You should have received a copy of the GNU General Public License along
* with this program; if not, write to the Free Software Foundation, Inc.,
* 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*
*****/

%begin %{
#define SWIG_PYTHON_2_UNICODE
#define TARGET_LANGUAGE_SELF PyObject *
#define TARGET_LANGUAGE_OUTPUT_TYPE PyObject **
%}

%include std_list.i

%ignore *::operator int;
%ignore *::operator long;
%ignore *::operator double;
%ignore *::operator const char *;
%ignore *::operator const std::string;
%ignore *::operator ();

%feature("python:slot", "tp_str", functype="reprfunc") *::what;
%feature("python:slot", "tp_repr", functype="reprfunc") *::toString;
%feature("python:slot", "mp_subscript", functype="binaryfunc") *::get;
%feature("python:slot", "mp_ass_subscript", functype="objobjargproc") *::set;
%feature("python:slot", "tp_hash") Prelude::IDMEFValue::getType;

/*
 * IDMEFClass
 */
%feature("python:slot", "tp_str", functype="reprfunc") Prelude::IDMEFClass::getName;
%feature("python:slot", "sq_item", functype="ssizeargfunc") Prelude::IDMEFClass::_get2;
%feature("python:slot", "mp_subscript", functype="binaryfunc") Prelude::IDMEFClass::get;
%feature("python:slot", "mp_length", functype="lenfunc") Prelude::IDMEFClass::getChildCount;

/*
 * IDMEFTime
 */
%feature("python:slot", "nb_int", functype="unaryfunc") Prelude::IDMEFTime::getSec;
%feature("python:slot", "nb_long", functype="unaryfunc") Prelude::IDMEFTime::_getSec2;
%feature("python:slot", "nb_float", functype="unaryfunc") Prelude::IDMEFTime::getTime;

/*
 *
 */
%feature("python:slot", "nb_lshift") Prelude::IDMEF::readExcept;
%feature("python:slot", "nb_rshift") Prelude::IDMEF::write;
%feature("python:sq_contains") Prelude::IDMEF "_wrap_IDMEF___contains___closure";


%ignore *::operator =;


%fragment("SWIG_FromBytePtrAndSize", "header", fragment="SWIG_FromCharPtrAndSize") %{
#if PY_VERSION_HEX < 0x03000000
# define SWIG_FromBytePtrAndSize(arg, len) PyString_FromStringAndSize(arg, len)
#else
# define SWIG_FromBytePtrAndSize(arg, len) PyBytes_FromStringAndSize(arg, len)
#endif
%}

%{
PyObject *__prelude_log_func = NULL;

static void _cb_python_log(int level, const char *str)
{
        PyObject *arglist, *result;

        SWIG_PYTHON_THREAD_BEGIN_BLOCK;

        arglist = Py_BuildValue("(i,s)", level, str);
        result = PyEval_CallObject(__prelude_log_func, arglist);

        Py_DECREF(arglist);
        Py_XDECREF(result);

        SWIG_PYTHON_THREAD_END_BLOCK;
}


static int _cb_python_write(prelude_msgbuf_t *fd, prelude_msg_t *msg)
{
#if PY_VERSION_HEX < 0x03000000
        size_t ret;
        PyObject *io = (PyObject *) prelude_msgbuf_get_data(fd);
        FILE *f = PyFile_AsFile(io);

        ret = fwrite((const char *)prelude_msg_get_message_data(msg), 1, prelude_msg_get_len(msg), f);
#else
        ssize_t ret;
        int ffd = PyObject_AsFileDescriptor((PyObject *) prelude_msgbuf_get_data(fd));

        do {
                ret = write(ffd, (const char *)prelude_msg_get_message_data(msg), prelude_msg_get_len(msg));
        } while ( ret < 0 && errno == EINTR );

#endif
        if ( ret != prelude_msg_get_len(msg) )
                return prelude_error_from_errno(errno);

        prelude_msg_recycle(msg);
        return 0;
}


static ssize_t _cb_python_read(prelude_io_t *fd, void *buf, size_t size)
{
#if PY_VERSION_HEX < 0x03000000
        ssize_t ret;
        PyObject *io = (PyObject *) prelude_io_get_fdptr(fd);
        FILE *f = PyFile_AsFile(io);

        ret = fread(buf, 1, size, f);
        if ( ret < 0 )
                ret = prelude_error_from_errno(errno);

        else if ( ret == 0 )
                ret = prelude_error(PRELUDE_ERROR_EOF);

#else
        ssize_t ret;
        int ffd = PyObject_AsFileDescriptor((PyObject *) prelude_io_get_fdptr(fd));

        ret = read(ffd, buf, size);
        if ( ret < 0 )
                ret = prelude_error_from_errno(errno);

        else if ( ret == 0 )
                ret = prelude_error(PRELUDE_ERROR_EOF);

#endif
        return ret;
}
%}

%typemap(in) void (*log_cb)(int level, const char *log) {
        if ( ! PyCallable_Check($input) )
                SWIG_exception_fail(SWIG_ValueError, "Argument is not a callable object");

        if ( __prelude_log_func )
                Py_DECREF(__prelude_log_func);

        __prelude_log_func = $input;
        Py_INCREF($input);

        $1 = _cb_python_log;
};


/* tell swig not to cast void * value */
%typemap(in) void *nocast_file_p %{
#if PY_VERSION_HEX < 0x03000000
        if ( !PyFile_Check((PyObject *) $input) ) {
                SWIG_exception_fail(SWIG_RuntimeError, "Argument is not a file object");

        }
#else
        extern PyTypeObject PyIOBase_Type;
        if ( ! PyObject_IsInstance((PyObject *) $input, (PyObject *) &PyIOBase_Type) ) {
                SWIG_exception_fail(SWIG_RuntimeError, "Argument is not a file object");
        }
#endif

        $1 = $input;
%}


%typemap(typecheck, precedence=SWIG_TYPECHECK_POINTER) void *nocast_file_p %{
#if PY_VERSION_HEX < 0x03000000
        $1 = PyFile_Check((PyObject *) $input);
#else
        extern PyTypeObject PyIOBase_Type;
        $1 = PyObject_IsInstance((PyObject *) $input, (PyObject *) &PyIOBase_Type);
#endif
%}


%exception readExcept(void *nocast_file_p) {
        try {
                $action
        } catch(Prelude::PreludeError &e) {
                if ( e.getCode() == PRELUDE_ERROR_EOF ) {
                        PyErr_SetString(PyExc_EOFError, e.what());
                } else {
                        SWIG_Python_Raise(SWIG_NewPointerObj(new PreludeError(e),
                                                             SWIGTYPE_p_Prelude__PreludeError, SWIG_POINTER_OWN),
                                          "PreludeError", SWIGTYPE_p_Prelude__PreludeError);
                }
                SWIG_fail;
        }
}


%exception read(void *nocast_file_p) {
        try {
                $action
        } catch(Prelude::PreludeError &e) {
                if ( e.getCode() == PRELUDE_ERROR_EOF ) {
                        result = 0;
                } else {
                        SWIG_Python_Raise(SWIG_NewPointerObj(new PreludeError(e),
                                                             SWIGTYPE_p_Prelude__PreludeError, SWIG_POINTER_OWN),
                                          "PreludeError", SWIGTYPE_p_Prelude__PreludeError);
                        SWIG_fail;
                }
        }
}


%exception {
        try {
                $action
        } catch(Prelude::PreludeError &e) {
                SWIG_Python_Raise(SWIG_NewPointerObj(new PreludeError(e),
                                                     SWIGTYPE_p_Prelude__PreludeError, SWIG_POINTER_OWN),
                                  "PreludeError", SWIGTYPE_p_Prelude__PreludeError);
                SWIG_fail;
        }
}


#ifdef SWIG_COMPILE_LIBPRELUDE

%inline %{
void python2_return_unicode(int enabled)
{
    _PYTHON2_RETURN_UNICODE = enabled;
}

%}

%{
typedef PyObject SwigPyObjectState;
%}

/*
 * This is called on Prelude::IDMEF::__getstate__()
 * Store our internal IDMEF data in the PyObjet __dict__
 */
%typemap(out) SwigPyObjectState * {
        int ret;
        PyObject *state;
        SwigPyObject *pyobj = (SwigPyObject *) self;

        state = PyDict_New();
        if ( ! state ) {
                Py_XDECREF(result);
                SWIG_fail;
        }

        ret = PyDict_SetItemString(state, "__idmef_data__", result);
        Py_DECREF(result);

        if ( pyobj->dict ) {
                ret = PyDict_Update(state, pyobj->dict);
                if ( ret < 0 ) {
                        Py_XDECREF(state);
                        SWIG_fail;
                }
        }

        if ( ret < 0 )
                throw PreludeError("error setting internal __idmef_data__ key");

        $result = state;
}

/*
 * This typemap specifically intercept the call to Prelude::IDMEF::__setstate__,
 * since at that time (when unpickling), the object __init__ method has not been
 * called, and the underlying Prelude::IDMEF object is thus NULL.
 *
 * We manually call tp_init() to handle the underlying object creation here.
 */
%typemap(arginit) (PyObject *state) {
        int ret;
        PyObject *obj;
        static PyTypeObject *pytype = NULL;

        if ( ! pytype ) {
                swig_type_info *sti = SWIG_TypeQuery("Prelude::IDMEF *");
                if ( ! sti )
                        throw PreludeError("could not find type SWIG type info for 'Prelude::IDMEF'");

                pytype = ((SwigPyClientData *) sti->clientdata)->pytype;
        }

        obj = PyTuple_New(0);
        ret = pytype->tp_init(self, obj, NULL);
        Py_DECREF(obj);

        if ( ret < 0 )
                throw PreludeError("error calling Prelude::IDMEF tp_init()");

}


/*
 *
 */
%exception Prelude::IDMEF::__setstate__ {
        try {
                SwigPyObject *pyobj = (SwigPyObject *) self;

                $function

                /*
                 * This is called at unpickling time, and set our PyObject internal dict.
                 */
                pyobj->dict = arg2;
                Py_INCREF(arg2);
        } catch(Prelude::PreludeError &e) {
                SWIG_Python_Raise(SWIG_NewPointerObj(new PreludeError(e),
                                                     SWIGTYPE_p_Prelude__PreludeError, SWIG_POINTER_OWN),
                                  "PreludeError", SWIGTYPE_p_Prelude__PreludeError);
                SWIG_fail;
        }
}


%{
static int _getstate_msgbuf_cb(prelude_msgbuf_t *mbuf, prelude_msg_t *msg)
{
        prelude_io_t *io = (prelude_io_t *) prelude_msgbuf_get_data(mbuf);
        return prelude_io_write(io, prelude_msg_get_message_data(msg), prelude_msg_get_len(msg));
}

static ssize_t _setstate_read_cb(prelude_io_t *io, void *buf, size_t size)
{
        FILE *fd = (FILE *) prelude_io_get_fdptr(io);
        return fread(buf, 1, size, fd);
}
%}



%extend Prelude::IDMEF {
        SwigPyObjectState *__getstate__(void)
        {
                int ret;
                prelude_io_t *io;
                PyObject *data;

                ret = prelude_io_new(&io);
                if ( ret < 0 )
                        throw PreludeError(ret);

                prelude_io_set_buffer_io(io);
                self->_genericWrite(_getstate_msgbuf_cb, io);

                data = SWIG_FromCharPtrAndSize((const char *) prelude_io_get_fdptr(io), prelude_io_pending(io));

                prelude_io_close(io);
                prelude_io_destroy(io);

                return data;
        }

        void __setstate__(PyObject *state) {
                FILE *fd;
                char *buf;
                ssize_t len;
                PyObject *data;

                data = PyDict_GetItemString(state, "__idmef_data__");
                if ( ! data )
                        throw PreludeError("no __idmef_data__ key within state dictionary");

                buf = PyString_AsString(data);
                len = PyString_Size(data);

                fd = fmemopen(buf, len, "r");
                if ( ! fd )
                        throw PreludeError(prelude_error_from_errno(errno));

                self->_genericRead(_setstate_read_cb, fd);
                fclose(fd);

                PyDict_DelItemString(state, "__idmef_data__");
        }

        int __contains__(const char *key) {
                return self->get(key).isNull() ? FALSE : TRUE;
        }

        IDMEF(void *nocast_file_p) {
                IDMEF *x = new IDMEF;

                try {
                        x->_genericRead(_cb_python_read, nocast_file_p);
                } catch(...) {
                        delete(x);
                        throw;
                }

                return x;
        }

        void write(void *nocast_file_p) {
                self->_genericWrite(_cb_python_write, nocast_file_p);
        }

        int read(void *nocast_file_p) {
                self->_genericRead(_cb_python_read, nocast_file_p);
                return 1;
        }

        int readExcept(void *nocast_file_p) {
                self->_genericRead(_cb_python_read, nocast_file_p);
                return 1;
        }
}


/*
 * When a comparison operator is called, this prevent an exception
 * if the compared operand does not have the correct datatype.
 *
 * By returning Py_NotImplemented, the python code might provide its
 * own comparison method within the compared operand class
 */
%typemap(in) (const Prelude::IDMEFTime &time) {
        int ret;
        void *obj;

        ret = SWIG_ConvertPtr($input, &obj, $descriptor(Prelude::IDMEFTime *),  0  | 0);
        if ( ! SWIG_IsOK(ret) || ! obj ) {
                Py_INCREF(Py_NotImplemented);
                return Py_NotImplemented;
        }

        $1 = reinterpret_cast< Prelude::IDMEFTime * >(obj);
}


/*
 * Workaround SWIG %features bug, which prevent us from applying multiple
 * features to the same method.
 */
%extend Prelude::IDMEFTime {
        long _getSec2(void) {
                return self->getSec();
        }
}

%exception Prelude::IDMEFClass::_get2 {
        try {
                $action;
        } catch(Prelude::PreludeError &e) {
                if ( e.getCode() == PRELUDE_ERROR_IDMEF_CLASS_UNKNOWN_CHILD ||
                     e.getCode() == PRELUDE_ERROR_IDMEF_PATH_DEPTH )
                        SWIG_exception_fail(SWIG_IndexError, e.what());
        }
}

%extend Prelude::IDMEFClass {
        Prelude::IDMEFClass _get2(int i) {
                return self->get(i);
        }
}
#endif

%fragment("IDMEFValueList_to_SWIG", "header", fragment="IDMEFValue_to_SWIG") {
int IDMEFValue_to_SWIG(TARGET_LANGUAGE_SELF self, const Prelude::IDMEFValue &result, void *extra, TARGET_LANGUAGE_OUTPUT_TYPE ret);

PyObject *IDMEFValueList_to_SWIG(TARGET_LANGUAGE_SELF self, const Prelude::IDMEFValue &value, void *extra)
{
        int j = 0, ret;
        PyObject *pytuple;
        std::vector<Prelude::IDMEFValue> result = value;
        std::vector<Prelude::IDMEFValue>::const_iterator i;

        pytuple = PyTuple_New(result.size());

        for ( i = result.begin(); i != result.end(); i++ ) {
                PyObject *val;

                if ( (*i).isNull() ) {
                        Py_INCREF(Py_None);
                        val = Py_None;
                } else {
                        ret = IDMEFValue_to_SWIG(self, *i, NULL, &val);
                        if ( ret < 0 )
                                return NULL;
                }

                PyTuple_SetItem(pytuple, j++, val);
        }

        return pytuple;
}
}


%typemap(out, fragment="IDMEFValue_to_SWIG") Prelude::IDMEFValue {
        int ret;

        if ( $1.isNull() ) {
                Py_INCREF(Py_None);
                $result = Py_None;
        } else {
#ifdef SWIGPYTHON_BUILTIN
                ret = IDMEFValue_to_SWIG(self, $1, NULL, &$result);
#else
                ret = IDMEFValue_to_SWIG(NULL, $1, NULL, &$result);
#endif
                if ( ret < 0 ) {
                        std::string s = "IDMEFValue typemap does not handle value of type '";
                        s += idmef_value_type_to_string((idmef_value_type_id_t) $1.getType());
                        s += "'";
                        SWIG_exception_fail(SWIG_ValueError, s.c_str());
                }
        }
};


%init {
        int argc, ret, idx;
        char **argv = NULL;
        PyObject *sys = PyImport_ImportModule("sys");
        PyObject *pyargv = PyObject_GetAttrString(sys, "argv");

        argc = PyObject_Length(pyargv);
        assert(argc >= 1);
        assert(PyList_Check(pyargv));

        if ( argc + 1 < 0 )
                throw PreludeError("Invalid argc length");

        argv = (char **) malloc((argc + 1) * sizeof(char *));
        if ( ! argv )
                throw PreludeError("Allocation failure");

        for ( idx = 0; idx < argc; idx++ ) {
                PyObject *o = PyList_GetItem(pyargv, idx);
#if PY_VERSION_HEX < 0x03000000
                PyArg_Parse(o, "et", Py_FileSystemDefaultEncoding, &argv[idx]);
#else
                argv[idx] = PyString_AsString(o);
#endif
                if ( ! argv[idx] )
                        break;
        }

        argv[idx] = NULL;

        ret = prelude_init(&argc, argv);
        if ( ret < 0 ) {
                free(argv);
                throw PreludeError(ret);
        }

        free(argv);

        Py_DECREF(pyargv);
        Py_DECREF(sys);
}


%{
extern "C" {
SWIGINTERN PyObject *_wrap_IDMEF___contains__(PyObject *self, PyObject *args);
}

#define MYPY_OBJOBJPROC_CLOSURE(wrapper)                        \
SWIGINTERN int                                                  \
wrapper##_closure(PyObject *a, PyObject *b) {                   \
    PyObject *pyresult;                                         \
    int result;                                                 \
    pyresult = wrapper(a, b);                                   \
    result = pyresult && PyObject_IsTrue(pyresult) ? 1 : 0;     \
    Py_XDECREF(pyresult);                                       \
    return result;                                              \
}

MYPY_OBJOBJPROC_CLOSURE(_wrap_IDMEF___contains__)
%}
