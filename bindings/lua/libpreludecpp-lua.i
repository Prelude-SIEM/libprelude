# Exception map
%typemap(throws) Prelude::PreludeError %{
        SWIG_exception(SWIG_RuntimeError, $1.what());
%};

# Lua overloading fixes
%ignore IDMEFCriteria(std::string const &);
%ignore IDMEFValue(int8_t);
%ignore IDMEFValue(uint8_t);
%ignore IDMEFValue(int16_t);
%ignore IDMEFValue(uint16_t);
%ignore IDMEFValue(int32_t);
%ignore IDMEFValue(uint32_t);
%ignore IDMEFValue(int64_t);
%ignore IDMEFValue(uint64_t);
%ignore IDMEFValue(float);
%ignore IDMEFValue(std::string);
%ignore Set(Prelude::IDMEF &, int8_t);
%ignore Set(Prelude::IDMEF &, uint8_t);
%ignore Set(Prelude::IDMEF &, int16_t);
%ignore Set(Prelude::IDMEF &, uint16_t);
%ignore Set(Prelude::IDMEF &, int32_t);
%ignore Set(Prelude::IDMEF &, uint32_t);
%ignore Set(Prelude::IDMEF &, int64_t);
%ignore Set(Prelude::IDMEF &, uint64_t);
%ignore Set(Prelude::IDMEF &, float);
%ignore Set(Prelude::IDMEF &, std::string);
%ignore Set(char const *, int8_t);
%ignore Set(char const *, uint8_t);
%ignore Set(char const *, int16_t);
%ignore Set(char const *, uint16_t);
%ignore Set(char const *, int32_t);
%ignore Set(char const *, uint32_t);
%ignore Set(char const *, int64_t);
%ignore Set(char const *, uint64_t);
%ignore Set(char const *, float);
%ignore Set(char const *, std::string);

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
%ignore operator <<;
%ignore operator >>;

%rename (__str__) *::operator const char *() const;

%header {
        char *my_strdup(const char *in)
        {
                char *out = new char[strlen(in) + 1];
                strcpy(out, in);
                return out;
        }
}

%ignore Prelude::IDMEFCriteria::operator const std::string() const;
%newobject Prelude::IDMEFCriteria::__str__;
%extend Prelude::IDMEFCriteria {
        char *__str__() { return my_strdup(self->ToString().c_str()); }
}

%ignore Prelude::IDMEFTime::operator const std::string() const;
%newobject Prelude::IDMEFTime::__str__;
%extend Prelude::IDMEFTime {
        char *__str__() { return my_strdup(self->ToString().c_str()); }
}

%ignore Prelude::IDMEF::operator const std::string() const;
%newobject Prelude::IDMEF::__str__;
%extend Prelude::IDMEF {
        char *__str__() { return my_strdup(self->ToString().c_str()); }
}



%header %{
#define SWIG_From_int(result) lua_pushnumber(L, (lua_Number) result)
#define SWIG_From_float(result) lua_pushnumber(L, (lua_Number) result)
#define SWIG_From_double(result) lua_pushnumber(L, (lua_Number) result)
#define SWIG_From_unsigned_SS_int(result) lua_pushnumber(L, (lua_Number) result)
#define SWIG_From_long_SS_long(result) lua_pushnumber(L, (lua_Number) result)
#define SWIG_From_unsigned_SS_long_SS_long(result) lua_pushnumber(L, (lua_Number) result)

#define SWIG_FromCharPtr(result) lua_pushstring(L, result)
#define SWIG_FromCharPtrAndSize(result, len) lua_pushlstring(L, result, len)

extern "C" {
int IDMEFValue_to_SWIG(lua_State* L, const IDMEFValue &result);
}

%}

/* tell squid not to cast void * value */
%typemap(in) void *nocast_p {
        FILE **pf;
        pf = (FILE **)lua_touserdata(L, $input);
        if (pf == NULL) {
                lua_pushstring(L,"Argument is not a file");
                SWIG_fail;
        }
        $1 = *pf;
}

%fragment("TransitionFunc", "header") {
static int __prelude_log_func;
static lua_State *__lua_state = NULL;
static gl_thread_t __initial_thread;

static void _cb_lua_log(int level, const char *str)
{
        if ( (gl_thread_t) gl_thread_self() != __initial_thread )
                return;

        lua_rawgeti(__lua_state, LUA_REGISTRYINDEX, __prelude_log_func);
        lua_pushnumber(__lua_state, level);
        lua_pushstring(__lua_state, str);
        lua_call(__lua_state, 2, 0);
}


static int _cb_lua_write(prelude_msgbuf_t *fd, prelude_msg_t *msg)
{
        size_t ret;
        FILE *f = (FILE *) prelude_msgbuf_get_data(fd);

        ret = fwrite((const char *)prelude_msg_get_message_data(msg), 1, prelude_msg_get_len(msg), f);
        if ( ret != prelude_msg_get_len(msg) )
                return prelude_error_from_errno(errno);

        prelude_msg_recycle(msg);
        return 0;
}


static ssize_t _cb_lua_read(prelude_io_t *fd, void *buf, size_t size)
{
        ssize_t ret;
        FILE *f = (FILE *) prelude_io_get_fdptr(fd);

        ret = fread(buf, 1, size, f);
        if ( ret < 0 )
                ret = prelude_error_from_errno(errno);

        else if ( ret == 0 )
                ret = prelude_error(PRELUDE_ERROR_EOF);

        return ret;
}

};


%typemap(in, fragment="TransitionFunc") void (*log_cb)(int level, const char *log) {
        if ( ! lua_isfunction(L, -1) )
                SWIG_exception(SWIG_ValueError, "Argument should be a function");

        if ( __lua_state )
                luaL_unref(L, LUA_REGISTRYINDEX, __prelude_log_func);

        __prelude_log_func = luaL_ref(L, LUA_REGISTRYINDEX);
        $1 = _cb_lua_log;
        __lua_state = L;
};


%extend Prelude::IDMEF {
        void Write(void *nocast_p) {
                self->_genericWrite(_cb_lua_write, nocast_p);
        }


        void Read(void *nocast_p) {
                self->_genericRead(_cb_lua_read, nocast_p);
        }
}

%fragment("IDMEFValueList_to_SWIG", "header") {
int IDMEFValueList_to_SWIG(lua_State *L, const Prelude::IDMEFValue &value)
{
        bool is_list;
        int index = 0, ret;
        std::vector<Prelude::IDMEFValue> result = value;
        std::vector<Prelude::IDMEFValue>::const_iterator i;

        lua_newtable(L);

        for ( i = result.begin(); i != result.end(); i++ ) {
                ret = lua_checkstack(L, 2);
                if ( ret < 0 )
                        return ret;

                is_list = (i->GetType() == IDMEF_VALUE_TYPE_LIST);
                if ( is_list )
                        lua_pushnumber(L, ++index);

                ret = IDMEFValue_to_SWIG(L, *i);
                if ( ret < 0 )
                        return -1;

                if ( is_list )
                        lua_settable(L, -3);
                else
                        lua_rawseti(L, -2, ++index);
        }

        return 1;
}
}

%fragment("IDMEFValue_to_SWIG", "wrapper", fragment="IDMEFValueList_to_SWIG") {
int IDMEFValue_to_SWIG(lua_State* L, const IDMEFValue &result)
{
        int ret = 1;
        std::stringstream s;
        idmef_value_t *value = result;
        idmef_value_type_id_t type = result.GetType();

        if ( type == IDMEF_VALUE_TYPE_STRING ) {
                prelude_string_t *str = idmef_value_get_string(value);
                SWIG_FromCharPtrAndSize(prelude_string_get_string(str), prelude_string_get_len(str));
        }

        else if ( type == IDMEF_VALUE_TYPE_INT8 )
                SWIG_From_int(idmef_value_get_int8(value));

        else if ( type == IDMEF_VALUE_TYPE_UINT8 )
                SWIG_From_unsigned_SS_int(idmef_value_get_uint8(value));

        else if ( type == IDMEF_VALUE_TYPE_INT16 )
                SWIG_From_int(idmef_value_get_int16(value));

        else if ( type == IDMEF_VALUE_TYPE_UINT16 )
                SWIG_From_unsigned_SS_int(idmef_value_get_uint16(value));

        else if ( type == IDMEF_VALUE_TYPE_INT32 )
                SWIG_From_int(idmef_value_get_int32(value));

        else if ( type == IDMEF_VALUE_TYPE_UINT32 )
                SWIG_From_unsigned_SS_int(idmef_value_get_uint32(value));

        else if ( type == IDMEF_VALUE_TYPE_INT64 )
                SWIG_From_long_SS_long(idmef_value_get_int64(value));

        else if ( type == IDMEF_VALUE_TYPE_UINT64 )
                SWIG_From_unsigned_SS_long_SS_long(idmef_value_get_uint64(value));

        else if ( type == IDMEF_VALUE_TYPE_FLOAT )
                SWIG_From_float(idmef_value_get_float(value));

        else if ( type == IDMEF_VALUE_TYPE_DOUBLE )
                SWIG_From_double(idmef_value_get_double(value));

        else if ( type == IDMEF_VALUE_TYPE_ENUM ) {
                const char *s = idmef_class_enum_to_string(idmef_value_get_class(value), idmef_value_get_enum(value));
                SWIG_FromCharPtr(s);
        }

        else if ( type == IDMEF_VALUE_TYPE_TIME ) {
                Prelude::IDMEFTime *time = new Prelude::IDMEFTime(idmef_time_ref(idmef_value_get_time(value)));
                SWIG_NewPointerObj(L, time, SWIGTYPE_p_Prelude__IDMEFTime, 1);
        }

        else if ( type == IDMEF_VALUE_TYPE_LIST )
                ret = IDMEFValueList_to_SWIG(L, result);

        else if ( type == IDMEF_VALUE_TYPE_DATA ) {
                idmef_data_t *d = idmef_value_get_data(value);
                idmef_data_type_t t = idmef_data_get_type(d);

                if ( t == IDMEF_DATA_TYPE_CHAR || t == IDMEF_DATA_TYPE_CHAR_STRING ||
                     t == IDMEF_DATA_TYPE_BYTE || t == IDMEF_DATA_TYPE_BYTE_STRING )
                        SWIG_FromCharPtrAndSize((const char *)idmef_data_get_data(d), idmef_data_get_len(d));

                else if ( t == IDMEF_DATA_TYPE_FLOAT )
                        SWIG_From_float(idmef_data_get_float(d));

                else if ( t == IDMEF_DATA_TYPE_UINT32 )
                        SWIG_From_unsigned_SS_int(idmef_data_get_uint32(d));

                else if ( t == IDMEF_DATA_TYPE_UINT64 )
                        SWIG_From_unsigned_SS_long_SS_long(idmef_data_get_uint64(d));
        }

        else return -1;

        return ret;
}
}


%typemap(out, fragment="IDMEFValue_to_SWIG") Prelude::IDMEFValue {
        int ret;

        if ( $1.IsNull() ) {
                lua_pushnil(L);
                SWIG_arg = 1;
        } else {
                SWIG_arg = IDMEFValue_to_SWIG(L, $1);
                if ( SWIG_arg < 0 ) {
                        std::stringstream s;
                        s << "IDMEFValue typemap does not handle value of type '" << idmef_value_type_to_string($1.GetType()) << "'";
                        SWIG_exception(SWIG_ValueError, s.str().c_str());
                }
        }
};



#define MAX(x, y) ((x) > (y) ? (x) : (y))
%init {
        int argc = 0, ret;
        static char *argv[1024];

        __initial_thread = (gl_thread_t) gl_thread_self();

        lua_getglobal(L, "arg");
        if ( ! lua_istable(L, -1) )
                return;

        lua_pushnil(L);

        while ( lua_next(L, -2) != 0 ) {
                int idx;
                const char *val;

                idx = lua_tonumber(L, -2);
                val = lua_tostring(L, -1);
                lua_pop(L, 1);

                if ( idx < 0 )
                        continue;

                if ( idx >= ((sizeof(argv) / sizeof(char *)) - 1) )
                        throw PreludeError("Argument index too large");

                argv[idx] = strdup(val);
                argc = MAX(idx, argc);
        }

        argc++;
        argv[argc] = NULL;

        ret = prelude_init(&argc, argv);
        if ( ret < 0 )
                throw PreludeError(ret);
}


