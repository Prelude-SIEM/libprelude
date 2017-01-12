/*****
*
* Copyright (C) 2005-2017 CS-SI. All Rights Reserved.
* Author: Yoann Vandoorselaere <yoannv@prelude-ids.com>
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

#if defined(SWIGPYTHON) || defined(SWIGLUA)
%module prelude
#else
%module Prelude
#endif

%warnfilter(509);
%feature("nothread", "1");

%include "std_string.i"
%include "std_vector.i"
%include "exception.i"

%{
#pragma GCC diagnostic ignored "-Wunused-variable"

#include <list>
#include <sstream>

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

%ignore idmef_class_id_t;
typedef int idmef_class_id_t;

typedef long long time_t;


%exception {
        try {
                $action
        } catch(Prelude::PreludeError &e) {
                SWIG_exception(SWIG_RuntimeError, e.what());
                SWIG_fail;
        }
}


#ifdef SWIGPERL
%include perl/libpreludecpp-perl.i
#endif

#ifdef SWIGPYTHON
%include python/libpreludecpp-python.i
#endif

#ifdef SWIGRUBY
%include ruby/libpreludecpp-ruby.i
#endif

#ifdef SWIGLUA
%include lua/libpreludecpp-lua.i
#endif


%ignore operator <<(std::ostream &os, const Prelude::IDMEF &idmef);
%ignore operator >>(std::istream &is, const Prelude::IDMEF &idmef);


%template() std::vector<std::string>;
%template() std::vector<Prelude::IDMEF>;
%template() std::vector<Prelude::IDMEFValue>;
%template() std::vector<Prelude::Connection>;

#ifdef SWIG_COMPILE_LIBPRELUDE
%extend Prelude::IDMEF {
        Prelude::IDMEFValue get(const char *path) {
                Prelude::IDMEFValue value;
                Prelude::IDMEFPath ipath = Prelude::IDMEFPath(*self, path);

                value = ipath.get(*self);
                if ( value.isNull() && ipath.isAmbiguous() ) {
                        std::vector<Prelude::IDMEFValue> v;
                        return Prelude::IDMEFValue(v);
                }

                return value;
        }
}

%extend Prelude::IDMEFPath {
        Prelude::IDMEFValue get(Prelude::IDMEF &message) {
                Prelude::IDMEFValue value;

                value = self->get(message);
                if ( value.isNull() && self->isAmbiguous() ) {
                        std::vector<Prelude::IDMEFValue> v;
                        return Prelude::IDMEFValue(v);
                }

                return value;
        }
}
#endif

%ignore Prelude::IDMEF::get;
%ignore Prelude::IDMEFPath::get;


%fragment("SWIG_FromBytePtrAndSize", "header", fragment="SWIG_FromCharPtrAndSize") %{
#ifndef SWIG_FromBytePtrAndSize
# define SWIG_FromBytePtrAndSize(arg, len) SWIG_FromCharPtrAndSize(arg, len)
#endif
%}


%fragment("IDMEFValue_to_SWIG", "header", fragment="SWIG_From_double",
                                          fragment="SWIG_From_float",
                                          fragment="SWIG_From_int", fragment="SWIG_From_unsigned_SS_int",
                                          fragment="SWIG_From_long_SS_long", fragment="SWIG_From_unsigned_SS_long_SS_long",
                                          fragment="SWIG_FromCharPtr", fragment="SWIG_FromCharPtrAndSize", fragment="SWIG_FromBytePtrAndSize",
                                          fragment="IDMEFValueList_to_SWIG") {

int IDMEFValue_to_SWIG(TARGET_LANGUAGE_SELF self, const Prelude::IDMEFValue &result, void *extra, TARGET_LANGUAGE_OUTPUT_TYPE ret)
{
        idmef_value_t *value = result;
        Prelude::IDMEFValue::IDMEFValueTypeEnum type = result.getType();

        if ( type == Prelude::IDMEFValue::TYPE_STRING ) {
                prelude_string_t *str = idmef_value_get_string(value);
                *ret = SWIG_FromCharPtrAndSize(prelude_string_get_string(str), prelude_string_get_len(str));
        }

        else if ( type == Prelude::IDMEFValue::TYPE_INT8 )
                *ret = SWIG_From_int(idmef_value_get_int8(value));

        else if ( type == Prelude::IDMEFValue::TYPE_UINT8 )
                *ret = SWIG_From_unsigned_SS_int(idmef_value_get_uint8(value));

        else if ( type == Prelude::IDMEFValue::TYPE_INT16 )
                *ret = SWIG_From_int(idmef_value_get_int16(value));

        else if ( type == Prelude::IDMEFValue::TYPE_UINT16 )
                *ret = SWIG_From_unsigned_SS_int(idmef_value_get_uint16(value));

        else if ( type == Prelude::IDMEFValue::TYPE_INT32 )
                *ret = SWIG_From_int(idmef_value_get_int32(value));

        else if ( type == Prelude::IDMEFValue::TYPE_UINT32 )
                *ret = SWIG_From_unsigned_SS_int(idmef_value_get_uint32(value));

        else if ( type == Prelude::IDMEFValue::TYPE_INT64 )
                *ret = SWIG_From_long_SS_long(idmef_value_get_int64(value));

        else if ( type == Prelude::IDMEFValue::TYPE_UINT64 )
                *ret = SWIG_From_unsigned_SS_long_SS_long(idmef_value_get_uint64(value));

        else if ( type == Prelude::IDMEFValue::TYPE_FLOAT )
                *ret = SWIG_From_float(idmef_value_get_float(value));

        else if ( type == Prelude::IDMEFValue::TYPE_DOUBLE )
                *ret = SWIG_From_double(idmef_value_get_double(value));

        else if ( type == Prelude::IDMEFValue::TYPE_ENUM ) {
                const char *s = idmef_class_enum_to_string(idmef_value_get_class(value), idmef_value_get_enum(value));
                *ret = SWIG_FromCharPtr(s);
        }

        else if ( type == Prelude::IDMEFValue::TYPE_TIME ) {
                Prelude::IDMEFTime t = result;
                *ret = SWIG_NewPointerObj(new Prelude::IDMEFTime(t), $descriptor(Prelude::IDMEFTime *), 1);
        }

        else if ( type == Prelude::IDMEFValue::TYPE_LIST )
                *ret = IDMEFValueList_to_SWIG(self, result, extra);

        else if ( type == Prelude::IDMEFValue::TYPE_DATA ) {
                idmef_data_t *d = idmef_value_get_data(value);
                idmef_data_type_t t = idmef_data_get_type(d);

                if ( t == IDMEF_DATA_TYPE_BYTE || t == IDMEF_DATA_TYPE_BYTE_STRING )
                        *ret = SWIG_FromBytePtrAndSize((const char *)idmef_data_get_data(d), idmef_data_get_len(d));

                else if ( t == IDMEF_DATA_TYPE_CHAR )
                        *ret = SWIG_FromCharPtrAndSize((const char *)idmef_data_get_data(d), idmef_data_get_len(d));

                else if ( t == IDMEF_DATA_TYPE_CHAR_STRING )
                        *ret = SWIG_FromCharPtrAndSize((const char *)idmef_data_get_data(d), idmef_data_get_len(d) - 1);

                else if ( t == IDMEF_DATA_TYPE_FLOAT )
                        *ret = SWIG_From_float(idmef_data_get_float(d));

                else if ( t == IDMEF_DATA_TYPE_UINT32 || IDMEF_DATA_TYPE_INT )
                        *ret = SWIG_From_unsigned_SS_long_SS_long(idmef_data_get_int(d));
        }

        else if ( type == Prelude::IDMEFValue::TYPE_CLASS ) {
                idmef_object_t *obj = (idmef_object_t *) idmef_value_get_object(value);
                *ret = SWIG_NewPointerObj(new Prelude::IDMEF(idmef_object_ref(obj)), $descriptor(Prelude::IDMEF *), 1);
        }

        else return -1;

        return 1;
}
}

%ignore Prelude::IDMEF::IDMEF(std::istream &);
%ignore Prelude::IDMEFValue::operator const char*() const;
%ignore Prelude::IDMEFValue::operator std::vector<IDMEFValue>() const;
%ignore Prelude::IDMEFValue::operator Prelude::IDMEFTime() const;
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

/*
 * Force SWIG to use the IDMEFValue * version of the Set() function,
 * so that the user might provide NULL IDMEFValue. Force usage of the
 * std::string value, for binary data.
 */
%ignore Prelude::IDMEF::set(char const *, int8_t);
%ignore Prelude::IDMEF::set(char const *, uint8_t);
%ignore Prelude::IDMEF::set(char const *, int16_t);
%ignore Prelude::IDMEF::set(char const *, uint16_t);
%ignore Prelude::IDMEF::set(char const *, char const *);
%ignore Prelude::IDMEF::set(char const *, Prelude::IDMEFValue &value);
%ignore Prelude::IDMEFPath::set(Prelude::IDMEF &, int8_t) const;
%ignore Prelude::IDMEFPath::set(Prelude::IDMEF &, uint8_t) const;
%ignore Prelude::IDMEFPath::set(Prelude::IDMEF &, int16_t) const;
%ignore Prelude::IDMEFPath::set(Prelude::IDMEF &, uint16_t) const;
%ignore Prelude::IDMEFPath::set(Prelude::IDMEF &, char const *) const;
%ignore Prelude::IDMEFPath::set(Prelude::IDMEF &, Prelude::IDMEFValue &) const;
%ignore Prelude::IDMEFValue::IDMEFValue(char const *);

%ignore idmef_path_t;
%ignore idmef_object_t;
%ignore idmef_criteria_t;
%ignore prelude_client_t;
%ignore prelude_client_profile_t;
%ignore prelude_connection_t;
%ignore prelude_connection_pool_t;
%ignore operator idmef_path_t *() const;
%ignore operator idmef_criteria_t *() const;
%ignore operator idmef_object_t *() const;
%ignore operator prelude_connection_t *();
%ignore operator prelude_connection_pool_t *();
%ignore operator idmef_message_t *() const;
%ignore operator idmef_time_t *() const;
%ignore operator idmef_value_t *() const;
%ignore operator prelude_client_profile_t *() const;

/*
 * We need to unlock the interpreter lock before calling certain methods
 * because they might acquire internal libprelude mutex that may also be
 * acquired undirectly through the libprelude asynchronous stack.
 *
 * [Thread 2]: Libprelude async stack
 * -> Lock internal mutexX
 *    -> prelude_log()
 *       -> SWIG/C log callback
 *          -> Wait on Interpreter lock [DEADLOCK]
 *             -> Python logging callback (never called)
 *
 * [Thread 1] ConnectionPool::Recv()
 *  -> Acquire Interpreter lock
 *      *** At this time, thread 2 lock internal mutexX
 *      -> Wait on internal mutexX [DEADLOCK]
 *
 * In this situation, [Thread 1] hold the Interpreter lock and is
 * waiting on mutexX, which itself cannot be released by [Thread 2]
 * until [Thread 1] unlock the Interpreter lock.
 *
 * One rule to prevent deadlock is to always acquire mutex in the same
 * order. We thus need to make sure the interpreter lock is released
 * before calling C++ method that are susceptible to lock a mutex that
 * is also triggered from the asynchronous interface.
 *
 * Note that we are not releasing the Interpreter lock in all C++ call,
 * because it come at a performance cost, so we only try to do it when
 * needed.
 */

#ifdef SWIG_COMPILE_LIBPRELUDE
%feature("exceptionclass") Prelude::PreludeError;
%feature("kwargs") Prelude::ClientEasy::ClientEasy;
%feature("kwargs") Prelude::Client::recvIDMEF;
%feature("kwargs") Prelude::IDMEFClass::getPath;
%feature("kwargs") Prelude::IDMEFPath::getClass;
%feature("kwargs") Prelude::IDMEFPath::getValueType;
%feature("kwargs") Prelude::IDMEFPath::setIndex;
%feature("kwargs") Prelude::IDMEFPath::getIndex;
%feature("kwargs") Prelude::IDMEFPath::undefineIndex;
%feature("kwargs") Prelude::IDMEFPath::compare;
%feature("kwargs") Prelude::IDMEFPath::getName;
%feature("kwargs") Prelude::IDMEFPath::isList;

%include prelude.hxx
%include prelude-client-profile.hxx

%feature("nothread", "0");
%include prelude-connection.hxx
%include prelude-connection-pool.hxx
%include prelude-client.hxx
%feature("nothread", "1");

%include prelude-log.hxx
%include prelude-error.hxx
%include prelude-client-easy.hxx
%include idmef-criteria.hxx
%include idmef-value.hxx
%include idmef-path.hxx
%include idmef-time.hxx
%include idmef-class.hxx
%include idmef.hxx
#endif

