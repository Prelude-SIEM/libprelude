/*****
*
* Copyright (C) 2005-2016 CS-SI. All Rights Reserved.
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

%rename (__str__) *::operator const std::string() const;
%rename (__str__) *::operator const char *() const;
%rename (__int__) *::operator int() const;
%rename (__long__) *::operator long() const;
%rename (__float__) *::operator double() const;

%ignore *::operator =;
%ignore *::operator !=;


%header %{
#define TARGET_LANGUAGE_SELF VALUE
#define TARGET_LANGUAGE_OUTPUT_TYPE VALUE *
%}

/* tell squid not to cast void * value */
%typemap(in) void *nocast_p {
        Check_Type($input, T_FILE);
        $1 = &$input;
}

%{
extern "C" {

#include <ruby.h>
/*
 * cannot put libmissing into the include path, as it will trigger
 * a compilation failure here.
 */
#include "config.h"
#include "../../libmissing/glthread/thread.h"


#ifdef HAVE_RUBY_IO_H /* Ruby 1.9 */
# include "ruby/io.h"
# define GetReadFile(x) rb_io_stdio_file(x)
# define GetWriteFile(x) rb_io_stdio_file(x)
#else
# include "rubyio.h" /* Ruby 1.8 */
#endif

#ifndef HAVE_RB_IO_T
# define rb_io_t OpenFile
#endif

#ifndef StringValuePtr
# define StringValuePtr(s) STR2CSTR(s)
#endif

#ifndef RARRAY_LEN
# define RARRAY_LEN(s) RARRAY(s)->len
#endif

#ifndef RARRAY_PTR
# define RARRAY_PTR(s) RARRAY(s)->ptr
#endif
}
%};


%fragment("TransitionFunc", "header") {
static gl_thread_t __initial_thread;
static VALUE __prelude_log_func = Qnil;

static void _cb_ruby_log(int level, const char *str)
{
        static int cid = rb_intern("call");

        if ( (gl_thread_t) gl_thread_self() != __initial_thread )
                return;

        rb_funcall(__prelude_log_func, cid, 2, SWIG_From_int(level), SWIG_FromCharPtr(str));
}


static int _cb_ruby_write(prelude_msgbuf_t *fd, prelude_msg_t *msg)
{
        ssize_t ret;
        rb_io_t *fptr;
        VALUE *io = (VALUE *) prelude_msgbuf_get_data(fd);

        GetOpenFile(*io, fptr);

        ret = fwrite((const char *) prelude_msg_get_message_data(msg), 1, prelude_msg_get_len(msg), GetWriteFile(fptr));
        if ( ret != prelude_msg_get_len(msg) )
                return prelude_error_from_errno(errno);

        prelude_msg_recycle(msg);

        return 0;
}


static ssize_t _cb_ruby_read(prelude_io_t *fd, void *buf, size_t size)
{
        ssize_t ret;
        rb_io_t *fptr;
        VALUE *io = (VALUE *) prelude_io_get_fdptr(fd);

        GetOpenFile(*io, fptr);

        ret = fread(buf, 1, size, GetReadFile(fptr));
        if ( ret < 0 )
                ret = prelude_error_from_errno(errno);

        else if ( ret == 0 )
                ret = prelude_error(PRELUDE_ERROR_EOF);

        return ret;
}
};


%typemap(in, fragment="TransitionFunc") void (*log_cb)(int level, const char *log) {
        if ( ! SWIG_Ruby_isCallable($input) )
                SWIG_exception_fail(SWIG_ValueError, "Argument is not a callable object");

        __prelude_log_func = $input;
        rb_global_variable(&$input);

        $1 = _cb_ruby_log;
};


%exception {
        try {
                $action
        } catch(Prelude::PreludeError &e) {
                if ( e.getCode() == PRELUDE_ERROR_EOF )
                        rb_raise(rb_eEOFError, "%s", e.what());
                else
                        SWIG_exception(SWIG_RuntimeError, e.what());

                SWIG_fail;
        }
}

%exception read(void *nocast_p) {
        try {
                $action
        } catch(Prelude::PreludeError &e) {
                if ( e.getCode() == PRELUDE_ERROR_EOF ) {
                        result = 0;
                } else
                        SWIG_exception_fail(SWIG_RuntimeError, e.what());
        }
}


%extend Prelude::IDMEF {
        void write(void *nocast_p) {
                self->_genericWrite(_cb_ruby_write, nocast_p);
        }

        int read(void *nocast_p) {
                self->_genericRead(_cb_ruby_read, nocast_p);
                return 1;
        }

        Prelude::IDMEF &operator >> (void *nocast_p) {
                self->_genericWrite(_cb_ruby_write, nocast_p);
                return *self;
        }

        Prelude::IDMEF &operator << (void *nocast_p) {
                self->_genericRead(_cb_ruby_read, nocast_p);
                return *self;
        }
}


%fragment("IDMEFValueList_to_SWIG", "header") {
int IDMEFValue_to_SWIG(TARGET_LANGUAGE_SELF self, const Prelude::IDMEFValue &result, void *extra, TARGET_LANGUAGE_OUTPUT_TYPE ret);

VALUE IDMEFValueList_to_SWIG(TARGET_LANGUAGE_SELF self, const Prelude::IDMEFValue &value, void *extra)
{
        int ret;
        VALUE ary;
        std::vector<Prelude::IDMEFValue> result = value;
        std::vector<Prelude::IDMEFValue>::const_iterator i;

        ary = rb_ary_new2(result.size());

        for ( i = result.begin(); i != result.end(); i++ ) {
                VALUE val;

                if ( (*i).isNull() )
                        val = Qnil;
                else {
                        ret = IDMEFValue_to_SWIG(self, *i, extra, &val);
                        if ( ret < 0 )
                                return Qnil;
                }

                if ( ! rb_ary_push(ary, val) )
                        return Qnil;
        }

        return ary;
}
}


%typemap(out, fragment="IDMEFValue_to_SWIG") Prelude::IDMEFValue {
        int ret;

        if ( $1.isNull() )
                $result = Qnil;
        else {
                ret = IDMEFValue_to_SWIG(self, $1, NULL, &$result);
                if ( ret < 0 ) {
                        std::stringstream s;
                        s << "IDMEFValue typemap does not handle value of type '" << idmef_value_type_to_string((idmef_value_type_id_t) $1.getType()) << "'";
                        SWIG_exception_fail(SWIG_ValueError, s.str().c_str());
                }
        }
};

%init {
        int ret;
        char **argv;
        int _i, argc;
        VALUE rbargv, *ptr, tmp;

        __initial_thread = (gl_thread_t) gl_thread_self();

        rbargv = rb_const_get(rb_cObject, rb_intern("ARGV"));
        argc = RARRAY_LEN(rbargv) + 1;

        if ( argc + 1 < 0 )
                throw PreludeError("Invalid argc length");

        argv = (char **) malloc((argc + 1) * sizeof(char *));
        if ( ! argv )
                throw PreludeError("Allocation failure");

        tmp = rb_gv_get("$0");
        argv[0] = StringValuePtr(tmp);

        for ( ptr = RARRAY_PTR(rbargv), _i = 1; _i < argc; _i++, ptr++ )
                argv[_i] = StringValuePtr(*ptr);

        argv[_i] = NULL;

        ret = prelude_init(&argc, argv);
        if ( ret < 0 ) {
                free(argv);
                throw PreludeError(ret);
        }

        free(argv);
}
