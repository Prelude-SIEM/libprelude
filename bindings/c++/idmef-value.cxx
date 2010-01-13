/*****
*
* Copyright (C) 2008 PreludeIDS Technologies. All Rights Reserved.
* Author: Yoann Vandoorselaere <yoann@prelude-ids.com>
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

#include <sstream>

#include "prelude-error.hxx"
#include "idmef.hxx"
#include "idmef-value.hxx"

using namespace Prelude;


IDMEFValue::~IDMEFValue()
{
        if ( _value )
                idmef_value_destroy(_value);
}


idmef_value_type_id_t IDMEFValue::GetType() const
{
        return idmef_value_get_type(_value);
}


bool IDMEFValue::IsNull() const
{
        return (! _value) ? TRUE : FALSE;
}


IDMEFValue::IDMEFValue()
{
        _value = NULL;
}


IDMEFValue::IDMEFValue(const IDMEFValue &value)
{
        idmef_value_t *v = NULL;

        if ( value._value )
                v = idmef_value_ref(value._value);

        _value = v;
}


void IDMEFValue::_InitFromString(const char *value)
{
        int ret;
        prelude_string_t *str;

        ret = prelude_string_new_dup(&str, value);
        if ( ret < 0 )
                throw PreludeError(ret);

        ret = idmef_value_new_string(&_value, str);
        if ( ret < 0 ) {
                prelude_string_destroy(str);
                throw PreludeError(ret);
        }
}


IDMEFValue::IDMEFValue(const char *value)
{
        _InitFromString(value);
}


IDMEFValue::IDMEFValue(std::string value)
{
        _InitFromString(value.c_str());
}


IDMEFValue::IDMEFValue(int8_t value)
{
        int ret = idmef_value_new_int8(&_value, value);
        if ( ret < 0 )
                throw PreludeError(ret);
}


IDMEFValue::IDMEFValue(uint8_t value)
{
        int ret = idmef_value_new_uint8(&_value, value);
        if ( ret < 0 )
                throw PreludeError(ret);
}


IDMEFValue::IDMEFValue(int16_t value)
{
        int ret = idmef_value_new_int16(&_value, value);
        if ( ret < 0 )
                throw PreludeError(ret);
}


IDMEFValue::IDMEFValue(uint16_t value)
{
        int ret = idmef_value_new_uint16(&_value, value);
        if ( ret < 0 )
                throw PreludeError(ret);
}


IDMEFValue::IDMEFValue(int32_t value)
{
        int ret = idmef_value_new_int32(&_value, value);
        if ( ret < 0 )
                throw PreludeError(ret);
}


IDMEFValue::IDMEFValue(uint32_t value)
{
        int ret = idmef_value_new_uint32(&_value, value);
        if ( ret < 0 )
                throw PreludeError(ret);
}


IDMEFValue::IDMEFValue(int64_t value)
{
        int ret = idmef_value_new_int64(&_value, value);
        if ( ret < 0 )
                throw PreludeError(ret);
}


IDMEFValue::IDMEFValue(uint64_t value)
{
        int ret = idmef_value_new_uint64(&_value, value);
        if ( ret < 0 )
                throw PreludeError(ret);
}


IDMEFValue::IDMEFValue(float value)
{
        int ret = idmef_value_new_float(&_value, value);
        if ( ret < 0 )
                throw PreludeError(ret);
}


IDMEFValue::IDMEFValue(double value)
{
        int ret = idmef_value_new_double(&_value, value);
        if ( ret < 0 )
                throw PreludeError(ret);
}


IDMEFValue::IDMEFValue(IDMEFTime &time)
{
        int ret = idmef_value_new_time(&_value, idmef_time_ref(time));
        if ( ret < 0 )
                throw PreludeError(ret);
}


IDMEFValue::IDMEFValue(std::vector<IDMEFValue> value)
{
        int ret;
        std::vector<Prelude::IDMEFValue>::const_iterator i;

        ret = idmef_value_new_list(&_value);
        if ( ret < 0 )
                throw PreludeError(ret);

        for ( i = value.begin(); i != value.end(); i++ )
                idmef_value_list_add(_value, idmef_value_ref(*i));
}

IDMEFValue::IDMEFValue(idmef_value_t *value)
{
        _value = value;
}


int IDMEFValue::Match(const IDMEFValue &value, int op)
{
        return idmef_value_match(this->_value, value._value, (idmef_criterion_operator_t) op);
}


IDMEFValue IDMEFValue::Clone() const
{
        int ret;
        idmef_value_t *clone;

        ret = idmef_value_clone(_value, &clone);
        if ( ret < 0 )
                throw PreludeError(ret);

        return IDMEFValue(clone);
}


static int iterate_cb(idmef_value_t *value, void *extra)
{
        std::vector<IDMEFValue> *vlist = (std::vector<IDMEFValue> *) extra;

        vlist->push_back(IDMEFValue(idmef_value_ref(value)));

        return 0;
}


IDMEFValue::operator std::vector<IDMEFValue> () const
{
        std::vector<IDMEFValue> vlist;

        if ( ! _value )
                return vlist;

        if ( GetType() != IDMEF_VALUE_TYPE_LIST ) {
                std::stringstream s;
                s << "Left value doesn't fit '" << idmef_value_type_to_string(GetType()) << "' requirement";
                throw PreludeError(s.str());
        }

        idmef_value_iterate(_value, iterate_cb, &vlist);

        return vlist;
}


IDMEFValue::operator IDMEFTime () const
{
        prelude_except_if_fail(_value);

        if ( GetType() != IDMEF_VALUE_TYPE_TIME ) {
                std::stringstream s;
                s << "Left value doesn't fit '" << idmef_value_type_to_string(GetType()) << "' requirement";
                throw PreludeError(s.str());
        }

        return IDMEFTime(idmef_time_ref(idmef_value_get_time(_value)));
}



IDMEFValue::operator int32_t () const
{
        idmef_value_type_id_t vtype;

        prelude_except_if_fail(_value);
        vtype = GetType();

        if ( vtype == IDMEF_VALUE_TYPE_INT8 )
                return idmef_value_get_int8(_value);

        else if ( vtype == IDMEF_VALUE_TYPE_UINT8 )
                return idmef_value_get_uint8(_value);

        else if ( vtype == IDMEF_VALUE_TYPE_INT16 )
                return idmef_value_get_int16(_value);

        else if ( vtype == IDMEF_VALUE_TYPE_UINT16 )
                return idmef_value_get_uint16(_value);

        else if ( vtype == IDMEF_VALUE_TYPE_INT32 )
                return idmef_value_get_int32(_value);

        else if ( vtype == IDMEF_VALUE_TYPE_ENUM )
                return idmef_value_get_enum(_value);

        std::stringstream s;
        s << "Left value doesn't fit '" << idmef_value_type_to_string(vtype) << "' requirement";
        throw PreludeError(s.str());
}


IDMEFValue::operator uint32_t () const
{
        idmef_value_type_id_t vtype;

        prelude_except_if_fail(_value);
        vtype = GetType();

        if ( vtype == IDMEF_VALUE_TYPE_UINT32 )
                return idmef_value_get_uint32(_value);
        else
                return (int32_t) *this;
}


IDMEFValue::operator int64_t () const
{
        idmef_value_type_id_t vtype;

        prelude_except_if_fail(_value);
        vtype = GetType();

        if ( vtype == IDMEF_VALUE_TYPE_INT64 )
                return idmef_value_get_int64(_value);
        else
                return (uint32_t) *this;
}


IDMEFValue::operator uint64_t () const
{
        idmef_value_type_id_t vtype;

        prelude_except_if_fail(_value);
        vtype = GetType();

        if ( vtype == IDMEF_VALUE_TYPE_UINT64 )
                return idmef_value_get_uint64(_value);
        else
                return (int64_t) *this;
}


IDMEFValue::operator float () const
{
        idmef_value_type_id_t vtype;

        prelude_except_if_fail(_value);
        vtype = GetType();

        if ( vtype == IDMEF_VALUE_TYPE_FLOAT )
                return idmef_value_get_float(_value);

        else if ( vtype == IDMEF_VALUE_TYPE_DATA ) {
                idmef_data_t *d = idmef_value_get_data(_value);

                if ( idmef_data_get_type(d) == IDMEF_DATA_TYPE_FLOAT )
                        return idmef_data_get_float(d);
        }

        std::stringstream s;
        s << "Left value doesn't fit '" << idmef_value_type_to_string(vtype) << "' requirement";
        throw PreludeError(s.str());
}


IDMEFValue::operator double () const
{
        idmef_value_type_id_t vtype;

        prelude_except_if_fail(_value);
        vtype = GetType();

        if ( vtype == IDMEF_VALUE_TYPE_DOUBLE )
                return idmef_value_get_double(_value);

        else
                return (float) *this;
}


std::string IDMEFValue::convert_string() const
{
        std::stringstream s;
        prelude_except_if_fail(_value);

        if ( GetType() == IDMEF_VALUE_TYPE_STRING )
                return prelude_string_get_string(idmef_value_get_string(_value));

        else if ( GetType() == IDMEF_VALUE_TYPE_TIME )
                return IDMEFTime(idmef_time_ref(idmef_value_get_time(_value)));

        else if ( GetType() == IDMEF_VALUE_TYPE_ENUM )
                return idmef_class_enum_to_string(idmef_value_get_class(_value), idmef_value_get_enum(_value));

        else if ( GetType() == IDMEF_VALUE_TYPE_DATA ) {
                idmef_data_t *d = idmef_value_get_data(_value);
                idmef_data_type_t t = idmef_data_get_type(d);

                if ( t == IDMEF_DATA_TYPE_CHAR_STRING )
                        return (const char *) idmef_data_get_char_string(d);

                else if ( t == IDMEF_DATA_TYPE_CHAR ) {
                        s << idmef_data_get_char(d);
                        return s.str();
                }

                else if ( t == IDMEF_DATA_TYPE_FLOAT ) {
                        s << idmef_data_get_float(d);
                        return s.str();
                }

                else if ( t == IDMEF_DATA_TYPE_UINT32 ) {
                        s << idmef_data_get_uint32(d);
                        return s.str();
                }

                else if ( t == IDMEF_DATA_TYPE_UINT64 ) {
                        s << idmef_data_get_uint64(d);
                        return s.str();
                }

                else {
                        s << "Left value doesn't fit 'data' type '" << t << "' requirement";
                        throw PreludeError(s.str());
                }
        }

        s << "Left value doesn't fit '" << idmef_value_type_to_string(GetType()) << "' requirement";
        throw PreludeError(s.str());
}


IDMEFValue &IDMEFValue::operator=(const IDMEFValue &p)
{
        if ( this != &p && _value != p._value ) {
                if ( _value )
                        idmef_value_destroy(_value);

                _value = (p._value) ? idmef_value_ref(p._value) : NULL;
        }

        return *this;
}


IDMEFValue::operator const char*() const
{
        static std::string t;
        t = this->convert_string();
        return t.c_str();
}


IDMEFValue::operator idmef_value_t *() const
{
        return _value;
}
