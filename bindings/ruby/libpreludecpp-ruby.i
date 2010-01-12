%rename (__str__) *::operator const std::string() const;
%rename (__str__) *::operator const char *() const;
%rename (__int__) *::operator int() const;
%rename (__long__) *::operator long() const;
%rename (__float__) *::operator double() const;

%ignore *::operator =;
%ignore *::operator !=;


%header %{
#define TARGET_LANGUAGE_OUTPUT_TYPE VALUE *
int IDMEFValue_to_SWIG(const IDMEFValue &result, TARGET_LANGUAGE_OUTPUT_TYPE ret);
%}

/* tell squid not to cast void * value */
%typemap(in) void *nocast_p {
        Check_Type($input, T_FILE);
        $1 = &$input;
}

%{
extern "C" {
#include "rubyio.h"
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
        FILE *f;
        ssize_t ret;
        OpenFile *fptr;
        VALUE *io = (VALUE *) prelude_msgbuf_get_data(fd);

        GetOpenFile(*io, fptr);
        f = fptr->f;

        ret = fwrite((const char *) prelude_msg_get_message_data(msg), 1, prelude_msg_get_len(msg), f);
        if ( ret != prelude_msg_get_len(msg) )
                return prelude_error_from_errno(errno);

        prelude_msg_recycle(msg);

        return 0;
}


static ssize_t _cb_ruby_read(prelude_io_t *fd, void *buf, size_t size)
{
        FILE *f;
        ssize_t ret;
        OpenFile *fptr;
        VALUE *io = (VALUE *) prelude_io_get_fdptr(fd);

        GetOpenFile(*io, fptr);
        f = fptr->f;

        ret = fread(buf, 1, size, f);
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


%extend Prelude::IDMEF {
        void Write(void *nocast_p) {
                self->_genericWrite(_cb_ruby_write, nocast_p);
        }

        void Read(void *nocast_p) {
                self->_genericRead(_cb_ruby_read, nocast_p);
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
VALUE IDMEFValueList_to_SWIG(const Prelude::IDMEFValue &value)
{
        VALUE ary;
        int ret, j = 0;
        std::vector<Prelude::IDMEFValue> result = value;
        std::vector<Prelude::IDMEFValue>::const_iterator i;

        ary = rb_ary_new2(result.size());

        for ( i = result.begin(); i != result.end(); i++ ) {
                VALUE val;

                ret = IDMEFValue_to_SWIG(*i, &val);
                if ( ret < 0 )
                        return Qnil;

                RARRAY(ary)->ptr[j++] = val;
        }

        RARRAY(ary)->len = result.size();

        return ary;
}
}


%typemap(out, fragment="IDMEFValue_to_SWIG") Prelude::IDMEFValue {
        int ret;

        if ( $1.IsNull() )
                $result = Qnil;
        else {
                ret = IDMEFValue_to_SWIG($1, &$result);
                if ( ret < 0 ) {
                        std::stringstream s;
                        s << "IDMEFValue typemap does not handle value of type '" << idmef_value_type_to_string($1.GetType()) << "'";
                        SWIG_exception_fail(SWIG_ValueError, s.str().c_str());
                }
        }
};

%init {
        int ret;
        char **argv;
        int _i, argc;
        VALUE rbargv, *ptr;

        __initial_thread = (gl_thread_t) gl_thread_self();

        rbargv = rb_const_get(rb_cObject, rb_intern("ARGV"));
        argc = RARRAY(rbargv)->len + 1;

        if ( argc + 1 < 0 )
                throw PreludeError("Invalid argc length");

        argv = (char **) malloc((argc + 1) * sizeof(char *));
        if ( ! argv )
                throw PreludeError("Allocation failure");

        argv[0] = STR2CSTR(rb_gv_get("$0"));

        ptr = RARRAY(rbargv)->ptr;
        for ( ptr = RARRAY(rbargv)->ptr, _i = 1; _i < argc; _i++, ptr++ )
                argv[_i] =  STR2CSTR(*ptr);

        argv[_i] = NULL;

        ret = prelude_init(&argc, argv);
        if ( ret < 0 ) {
                free(argv);
                throw PreludeError(ret);
        }

        free(argv);
}
