/*****
*
* Copyright (C) 2005-2012 CS-SI. All Rights Reserved.
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

%include std_list.i

%warnfilter(511);

%rename (__str__) *::operator const std::string() const;
%rename (__str__) *::operator const char *() const;
%rename (__int__) *::operator int() const;
%rename (__long__) *::operator long() const;
%rename (__float__) *::operator double() const;

%ignore *::operator =;

%begin %{
#define TARGET_LANGUAGE_OUTPUT_TYPE PyObject **
%}

%fragment("SWIG_FromBytePtrAndSize", "header", fragment="SWIG_FromCharPtrAndSize") %{
#if PY_VERSION_HEX < 0x03000000
# define SWIG_FromBytePtrAndSize(arg, len) PyString_FromStringAndSize(arg, len)
#else
# define SWIG_FromBytePtrAndSize(arg, len) PyBytes_FromStringAndSize(arg, len)
#endif
%}

%insert("python") {
import sys

def python2_unicode_patch(cl):
    if cl.__str__ is object.__str__:
        return cl

    if sys.version_info < (3, 0):
         cl.__unicode__ = lambda self: self.__str__().decode('utf-8')

    cl.__repr__ = lambda self: self.__class__.__name__ + "(" + repr(str(self)) + ")"
    return cl
}

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


/* tell squid not to cast void * value */
%typemap(in) void *nocast_file_p %{
#if PY_VERSION_HEX < 0x03000000
        if ( !PyFile_Check((PyObject *) $input) ) {
                const char *errstr = "Argument is not a file object.";
                PyErr_SetString(PyExc_RuntimeError, errstr);
                return NULL;
        }
#else
        extern PyTypeObject PyIOBase_Type;
        if ( ! PyObject_IsInstance((PyObject *) $input, (PyObject *) &PyIOBase_Type) ) {
                const char *errstr = "Argument is not a file object.";
                PyErr_SetString(PyExc_RuntimeError, errstr);
                return NULL;
        }
#endif

        $1 = $input;
%}


%exception {
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


%exception read(void *nocast_p) {
        try {
                $action
        } catch(Prelude::PreludeError &e) {
                if ( e.getCode() == PRELUDE_ERROR_EOF )
                        result = 0;
                else
                        SWIG_exception_fail(SWIG_RuntimeError, e.what());
        }
}


#ifdef SWIG_COMPILE_LIBPRELUDE

%extend Prelude::IDMEFValue {
        long __hash__() {
                return $self->getType();
        }
}


%extend Prelude::IDMEF {
        %insert("python") %{
        def __setitem__(self, key, value):
                return self.set(key, value)

        def __getitem__(self, key):
                try:
                        return self.get(key)
                except:
                        raise IndexError

        %}

        void write(void *nocast_file_p) {
                self->_genericWrite(_cb_python_write, nocast_file_p);
        }

        int read(void *nocast_file_p) {
                self->_genericRead(_cb_python_read, nocast_file_p);
                return 1;
        }

        Prelude::IDMEF &operator >> (void *nocast_file_p) {
                self->_genericWrite(_cb_python_write, nocast_file_p);
                return *self;
        }

        Prelude::IDMEF &operator << (void *nocast_file_p) {
                self->_genericRead(_cb_python_read, nocast_file_p);
                return *self;
        }
}


%extend Prelude::IDMEFClass {
    %insert("python") %{
        def __getitem__(self, key):
                if isinstance(key, slice):
                        return itertools.islice(self, key.start, key.stop, key.step)

                try:
                        return self.get(key)
                except Exception as e:
                        raise IndexError

        def __str__(self):
                return self.getName()

        def __repr__(self):
                return "IDMEFClass(" + self.getName() + ", ".join([repr(i) for i in self]) + "\n)"
    %}
}

#endif

%fragment("IDMEFValueList_to_SWIG", "header", fragment="IDMEFValue_to_SWIG") {
int IDMEFValue_to_SWIG(const Prelude::IDMEFValue &result, void *extra, TARGET_LANGUAGE_OUTPUT_TYPE ret);

PyObject *IDMEFValueList_to_SWIG(const Prelude::IDMEFValue &value, void *extra)
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
                        ret = IDMEFValue_to_SWIG(*i, NULL, &val);
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
                ret = IDMEFValue_to_SWIG($1, NULL, &$result);
                if ( ret < 0 ) {
                        std::stringstream s;
                        s << "IDMEFValue typemap does not handle value of type '" << idmef_value_type_to_string((idmef_value_type_id_t) $1.getType()) << "'";
                        SWIG_exception_fail(SWIG_ValueError, s.str().c_str());
                }
        }
};


%feature("shadow") clone() %{
        def __deepcopy__(self, memo):
                return $action(self.this)
%}


%init {
        int argc, ret, i;
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

        for ( i = 0; i < argc; i++ ) {
                PyObject *o = PyList_GetItem(pyargv, i);
#if PY_VERSION_HEX < 0x03000000
                PyArg_Parse(o, "et", Py_FileSystemDefaultEncoding, &argv[i]);
#else
                argv[i] = PyString_AsString(o);
#endif
                if ( ! argv[i] )
                        break;
        }

        argv[i] = NULL;

        ret = prelude_init(&argc, argv);
        if ( ret < 0 ) {
                free(argv);
                throw PreludeError(ret);
        }

        free(argv);

        Py_DECREF(pyargv);
        Py_DECREF(sys);
}
