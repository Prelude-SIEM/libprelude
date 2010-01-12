# Exception map
%typemap(throws) Prelude::PreludeError %{
        SWIG_exception(SWIG_RuntimeError, $1.what());
%};


# Conversion not allowed
%ignore *::operator =;
%ignore *::operator int() const;
%ignore *::operator long() const;
%ignore *::operator int32_t() const;
%ignore *::operator uint32_t() const;
%ignore *::operator int64_t() const;
%ignore *::operator uint64_t() const;
%ignore *::operator float() const;
%ignore *::operator double() const;
%ignore *::operator Prelude::IDMEFTime() const;
%ignore *::operator const std::string() const;
%ignore *::operator const char *() const;


%header %{
#define TARGET_LANGUAGE_OUTPUT_TYPE SV **
int IDMEFValue_to_SWIG(const IDMEFValue &result, TARGET_LANGUAGE_OUTPUT_TYPE ret);
%}

%fragment("IDMEFValueList_to_SWIG", "header") {
SV *IDMEFValueList_to_SWIG(const Prelude::IDMEFValue &value)
{
        int j = 0, ret;
        std::vector<Prelude::IDMEFValue> result = value;
        std::vector<Prelude::IDMEFValue>::const_iterator i;

        AV *myav;
        SV *svret, **svs = new SV*[result.size()];

        for ( i = result.begin(); i != result.end(); i++ ) {
                ret = IDMEFValue_to_SWIG(*i, &svs[j++]);
                if ( ret < 0 )
                        return NULL;
        }

        myav = av_make(result.size(), svs);
        delete[] svs;

        svret = newRV_noinc((SV*) myav);
        sv_2mortal(svret);

        return svret;
}
}

/* tell squid not to cast void * value */
%typemap(in) void *nocast_p {
        $1 = $input;
}

%fragment("TransitionFunc", "header") {
static SV *__prelude_log_func;
static gl_thread_t __initial_thread;


static void _cb_perl_log(int level, const char *str)
{
        if ( (gl_thread_t) gl_thread_self() != __initial_thread )
                return;

        dSP;
        ENTER;
        SAVETMPS;

        PUSHMARK(SP);
        XPUSHs(SWIG_From_int(level));
        XPUSHs(SWIG_FromCharPtr(str));
        PUTBACK;

        perl_call_sv(__prelude_log_func, G_VOID);

        FREETMPS;
        LEAVE;
}


static int _cb_perl_write(prelude_msgbuf_t *fd, prelude_msg_t *msg)
{
        int ret;
        PerlIO *io = (PerlIO *) prelude_msgbuf_get_data(fd);

        ret = PerlIO_write(io, (const char *) prelude_msg_get_message_data(msg), prelude_msg_get_len(msg));
        if ( ret != prelude_msg_get_len(msg) )
                return prelude_error_from_errno(errno);

        prelude_msg_recycle(msg);

        return 0;
}


static ssize_t _cb_perl_read(prelude_io_t *fd, void *buf, size_t size)
{
        int ret;
        PerlIO *io = (PerlIO *) prelude_io_get_fdptr(fd);

        ret = PerlIO_read(io, buf, size);
        if ( ret < 0 )
                ret = prelude_error_from_errno(errno);

        else if ( ret == 0 )
                ret = prelude_error(PRELUDE_ERROR_EOF);

        return ret;
}
};

%typemap(in, fragment="TransitionFunc") void (*log_cb)(int level, const char *log) {
        if ( __prelude_log_func )
                SvREFCNT_dec(__prelude_log_func);

        __prelude_log_func = $input;
        SvREFCNT_inc($input);

        $1 = _cb_perl_log;
};

%extend Prelude::IDMEF {
        void Write(void *nocast_p) {
                PerlIO *io = IoIFP(sv_2io((SV *) nocast_p));
                self->_genericWrite(_cb_perl_write, io);
        }

        void Read(void *nocast_p) {
                PerlIO *io = IoIFP(sv_2io((SV *) nocast_p));
                self->_genericRead(_cb_perl_read, io);
        }
}


%typemap(out, fragment="IDMEFValue_to_SWIG") Prelude::IDMEFValue {
        int ret;

        if ( $1.IsNull() ) {
                SvREFCNT_inc (& PL_sv_undef);
                $result = &PL_sv_undef;
        } else {
                SV *mysv;

                ret = IDMEFValue_to_SWIG($1, &mysv);
                if ( ret < 0 ) {
                        std::stringstream s;
                        s << "IDMEFValue typemap does not handle value of type '" << idmef_value_type_to_string($1.GetType()) << "'";
                        SWIG_exception_fail(SWIG_ValueError, s.str().c_str());
                }

                $result = mysv;
        }

        argvi++;
};


%init {
        STRLEN len;
        char **argv;
        int j, argc = 1, ret;
        AV *pargv = get_av("ARGV", FALSE);

        __initial_thread = (gl_thread_t) gl_thread_self();

        ret = av_len(pargv);
        if ( ret >= 0 )
                argc += ret + 1;

        if ( argc + 1 < 0 )
                throw PreludeError("Invalide argc length");

        argv = (char **) malloc((argc + 1) * sizeof(char *));
        if ( ! argv )
                throw PreludeError("Allocation failure");

        argv[0] = SvPV(get_sv("0", FALSE), len);

        for ( j = 0; j < ret + 1; j++ )
                argv[j + 1] = SvPV(*av_fetch(pargv, j, FALSE), len);

        argv[j + 1] = NULL;

        ret = prelude_init(&argc, argv);
        if ( ret < 0 ) {
                free(argv);
                throw PreludeError(ret);
        }

        free(argv);
}
