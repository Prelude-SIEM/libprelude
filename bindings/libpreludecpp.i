%module PreludeEasy

%include "std_string.i"
%include "std_vector.i"
%include "exception.i"

%{
#include <list>
#include <sstream>

#ifndef SWIGPYTHON
# include "config.h"
# include "glthread/thread.h"
#endif

#include "prelude.hxx"
#include "prelude-log.hxx"
#include "prelude-error.hxx"
#include "prelude-connection.hxx"
#include "prelude-connection-pool.hxx"
#include "prelude-client-profile.hxx"
#include "prelude-client.hxx"
#include "prelude-client-easy.hxx"
#include "idmef-criteria.hxx"
#include "idmef-value.hxx"
#include "idmef-path.hxx"
#include "idmef-time.hxx"
#include "idmef.hxx"

using namespace Prelude;
%}


typedef char int8_t;
typedef unsigned char uint8_t;

typedef short int16_t;
typedef unsigned short uint16_t;

typedef int int32_t;
typedef unsigned int uint32_t;

typedef long long int64_t;
typedef unsigned long long uint64_t;

%ignore prelude_error_t;
typedef signed int prelude_error_t;


#ifdef SWIGPERL
%include perl/libpreludecpp-perl.i
#endif

#ifdef SWIGPYTHON
%include libpreludecpp-python.i
#endif

#ifdef SWIGRUBY
%include libpreludecpp-ruby.i
#endif

#ifdef SWIGLUA
%include libpreludecpp-lua.i
#endif

%catches(Prelude::PreludeError);

%ignore operator <<(std::ostream &os, const Prelude::IDMEF &idmef);
%ignore operator >>(std::istream &is, const Prelude::IDMEF &idmef);


%template() std::vector<std::string>;
%template() std::vector<Prelude::IDMEFValue>;
%template() std::vector<Prelude::Connection>;


%fragment("IDMEFValue_to_SWIG", "header", fragment="IDMEFValueList_to_SWIG", fragment="SWIG_From_float") {


int IDMEFValue_to_SWIG(const IDMEFValue &result, TARGET_LANGUAGE_OUTPUT_TYPE ret)
{
        std::stringstream s;
        idmef_value_t *value = result;
        idmef_value_type_id_t type = result.GetType();

        if ( type == IDMEF_VALUE_TYPE_STRING ) {
                prelude_string_t *str = idmef_value_get_string(value);
                *ret = SWIG_FromCharPtrAndSize(prelude_string_get_string(str), prelude_string_get_len(str));
        }

        else if ( type == IDMEF_VALUE_TYPE_INT8 )
                *ret = SWIG_From_int(idmef_value_get_int8(value));

        else if ( type == IDMEF_VALUE_TYPE_UINT8 )
                *ret = SWIG_From_unsigned_SS_int(idmef_value_get_uint8(value));

        else if ( type == IDMEF_VALUE_TYPE_INT16 )
                *ret = SWIG_From_int(idmef_value_get_int16(value));

        else if ( type == IDMEF_VALUE_TYPE_UINT16 )
                *ret = SWIG_From_unsigned_SS_int(idmef_value_get_uint16(value));

        else if ( type == IDMEF_VALUE_TYPE_INT32 )
                *ret = SWIG_From_int(idmef_value_get_int32(value));

        else if ( type == IDMEF_VALUE_TYPE_UINT32 )
                *ret = SWIG_From_unsigned_SS_int(idmef_value_get_uint32(value));

        else if ( type == IDMEF_VALUE_TYPE_INT64 )
                *ret = SWIG_From_long_SS_long(idmef_value_get_int64(value));

        else if ( type == IDMEF_VALUE_TYPE_UINT64 )
                *ret = SWIG_From_unsigned_SS_long_SS_long(idmef_value_get_uint64(value));

        else if ( type == IDMEF_VALUE_TYPE_FLOAT )
                *ret = SWIG_From_float(idmef_value_get_float(value));

        else if ( type == IDMEF_VALUE_TYPE_DOUBLE )
                *ret = SWIG_From_double(idmef_value_get_double(value));

        else if ( type == IDMEF_VALUE_TYPE_ENUM ) {
                const char *s = idmef_class_enum_to_string(idmef_value_get_class(value), idmef_value_get_enum(value));
                *ret = SWIG_FromCharPtr(s);
        }

        else if ( type == IDMEF_VALUE_TYPE_TIME ) {
                IDMEFTime time = result;
                *ret = SWIG_NewPointerObj(new IDMEFTime(time), SWIGTYPE_p_Prelude__IDMEFTime, 1);
        }

        else if ( type == IDMEF_VALUE_TYPE_LIST )
                *ret = IDMEFValueList_to_SWIG(result);

        else if ( type == IDMEF_VALUE_TYPE_DATA ) {
                idmef_data_t *d = idmef_value_get_data(value);
                idmef_data_type_t t = idmef_data_get_type(d);

                if ( t == IDMEF_DATA_TYPE_CHAR ||
                     t == IDMEF_DATA_TYPE_BYTE || t == IDMEF_DATA_TYPE_BYTE_STRING )
                        *ret = SWIG_FromCharPtrAndSize((const char *)idmef_data_get_data(d), idmef_data_get_len(d));

                else if ( t == IDMEF_DATA_TYPE_CHAR_STRING )
                        *ret = SWIG_FromCharPtrAndSize((const char *)idmef_data_get_data(d), idmef_data_get_len(d) - 1);

                else if ( t == IDMEF_DATA_TYPE_FLOAT )
                        *ret = SWIG_From_float(idmef_data_get_float(d));

                else if ( t == IDMEF_DATA_TYPE_UINT32 )
                        *ret = SWIG_From_unsigned_SS_int(idmef_data_get_uint32(d));

                else if ( t == IDMEF_DATA_TYPE_UINT64 )
                        *ret = SWIG_From_unsigned_SS_long_SS_long(idmef_data_get_uint64(d));
        }

        else if ( type == IDMEF_VALUE_TYPE_CLASS )
                *ret = SWIG_NewPointerObj(new IDMEFValue(idmef_value_ref(value)), SWIGTYPE_p_Prelude__IDMEFValue, 1);

        else return -1;

        return 0;
}
}

%ignore Prelude::IDMEFValue::operator int8_t() const;
%ignore Prelude::IDMEFValue::operator uint8_t() const;
%ignore Prelude::IDMEFValue::operator int16_t() const;
%ignore Prelude::IDMEFValue::operator uint16_t() const;
%ignore Prelude::IDMEFValue::operator int32_t() const;
%ignore Prelude::IDMEFValue::operator uint32_t() const;
%ignore Prelude::IDMEFValue::operator int64_t() const;
%ignore Prelude::IDMEFValue::operator uint64_t() const;
%ignore Prelude::IDMEFValue::operator float() const;
%ignore Prelude::IDMEFValue::operator double() const;
%ignore Prelude::IDMEFValue::operator const char*() const;
%ignore Prelude::IDMEFValue::operator std::vector<IDMEFValue>() const;
%ignore Prelude::IDMEFValue::operator Prelude::IDMEFTime() const;

/*
 * Force SWIG to use the IDMEFValue * version of the Set() function,
 * so that the user might provide NULL IDMEFValue.
 */
%ignore Prelude::IDMEF::Set(char const *, Prelude::IDMEFValue &value);
%ignore Prelude::IDMEFPath::Set(Prelude::IDMEF &, Prelude::IDMEFValue &);

%ignore idmef_path_t;
%ignore idmef_criteria_t;
%ignore prelude_client_t;
%ignore prelude_client_profile_t;
%ignore prelude_connection_t;
%ignore prelude_connection_pool_t;
%ignore operator prelude_connection_t *();
%ignore operator prelude_connection_pool_t *();
%ignore operator idmef_message_t *() const;
%ignore operator idmef_time_t *() const;
%ignore operator idmef_value_t *() const;
%ignore operator prelude_client_profile_t *() const;

%include prelude.hxx
%include prelude-log.hxx
%include prelude-error.hxx
%include prelude-connection.hxx
%include prelude-connection-pool.hxx
%include prelude-client-profile.hxx
%include prelude-client.hxx
%include prelude-client-easy.hxx
%include idmef-criteria.hxx
%include idmef-value.hxx
%include idmef-path.hxx
%include idmef-time.hxx
%include idmef.hxx
