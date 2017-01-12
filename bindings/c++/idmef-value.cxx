/*****
*
* Copyright (C) 2008-2017 CS-SI. All Rights Reserved.
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
* You should have received a copy of the GNU General Public License along
* with this program; if not, write to the Free Software Foundation, Inc.,
* 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*
*****/

#include <string.h>
#include <sstream>

#include "prelude-error.hxx"
#include "idmef.hxx"
#include "idmef-value.hxx"
#include "idmef-criteria.hxx"

using namespace Prelude;


IDMEFValue::~IDMEFValue()
{
        if ( _value )
                idmef_value_destroy(_value);
}


IDMEFValue::IDMEFValueTypeEnum IDMEFValue::getType() const
{
        return (IDMEFValue::IDMEFValueTypeEnum) idmef_value_get_type(_value);
}


bool IDMEFValue::isNull() const
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


void IDMEFValue::_InitFromString(const char *value, size_t len)
{
        int ret;
        prelude_string_t *str;

        ret = prelude_string_new_dup_fast(&str, value, len);
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
        _InitFromString(value, strlen(value));
}


IDMEFValue::IDMEFValue(const std::string &value)
{
        _InitFromString(value.c_str(), value.size());
}


IDMEFValue::IDMEFValue(int32_t value)
{
        int ret = idmef_value_new_int32(&_value, value);
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


IDMEFValue::IDMEFValue(IDMEF *idmef)
{
        int ret;
        idmef_value_t *v = NULL;

        if (  idmef ) {
                ret = idmef_value_new_class(&v, idmef->getId(), idmef_object_ref((idmef_object_t *) *idmef));
                if ( ret < 0 )
                       throw PreludeError(ret);
        }

        _value = v;
}


IDMEFValue::IDMEFValue(const std::vector<IDMEF> &value)
{
        int ret;
        idmef_value_t *vitem;
        std::vector<Prelude::IDMEF>::const_iterator i;

        ret = idmef_value_new_list(&_value);
        if ( ret < 0 )
                throw PreludeError(ret);

        for ( i = value.begin(); i != value.end(); i++ ) {
                ret = idmef_value_new_class(&vitem, i->getId(), idmef_object_ref((idmef_object_t *) *i));
                if ( ret < 0 )
                        throw PreludeError(ret);

                idmef_value_list_add(_value, vitem);
        }
}


IDMEFValue::IDMEFValue(const std::vector<IDMEFValue> &value)
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


int IDMEFValue::match(const IDMEFValue &value, int op) const
{
        int ret;

        ret = idmef_value_match(this->_value, value._value, (idmef_criterion_operator_t) op);
        if ( ret < 0 )
                throw PreludeError(ret);

        return ret;
}


IDMEFValue IDMEFValue::clone() const
{
        int ret;
        idmef_value_t *clone;

        ret = idmef_value_clone(_value, &clone);
        if ( ret < 0 )
                throw PreludeError(ret);

        return IDMEFValue(clone);
}


const std::string IDMEFValue::toString() const
{
        int ret;
        std::string s;
        prelude_string_t *str;

        ret = prelude_string_new(&str);
        if ( ret < 0 )
                throw PreludeError(ret);

        ret = idmef_value_to_string(_value, str);
        if ( ret < 0 ) {
                prelude_string_destroy(str);
                throw PreludeError(ret);
        }

        s = prelude_string_get_string(str);
        prelude_string_destroy(str);

        return s;
}


static int iterate_cb(idmef_value_t *value, void *extra)
{
        std::vector<IDMEFValue> *vlist = (std::vector<IDMEFValue> *) extra;

        if ( value )
                value = idmef_value_ref(value);

        vlist->push_back(IDMEFValue(value));

        return 0;
}


IDMEFValue::operator std::vector<IDMEFValue> () const
{
        std::vector<IDMEFValue> vlist;

        if ( ! _value )
                return vlist;

        if ( getType() != TYPE_LIST ) {
                std::stringstream s;
                s << "Left value doesn't fit '" << idmef_value_type_to_string((idmef_value_type_id_t) getType()) << "' requirement";
                throw PreludeError(s.str());
        }

        idmef_value_iterate(_value, iterate_cb, &vlist);

        return vlist;
}


IDMEFValue::operator IDMEFTime () const
{
        prelude_except_if_fail(_value);

        if ( getType() != TYPE_TIME ) {
                std::stringstream s;
                s << "Left value doesn't fit '" << idmef_value_type_to_string((idmef_value_type_id_t) getType()) << "' requirement";
                throw PreludeError(s.str());
        }

        return IDMEFTime(idmef_time_ref(idmef_value_get_time(_value)));
}



IDMEFValue::operator int32_t () const
{
        IDMEFValueTypeEnum vtype;

        prelude_except_if_fail(_value);
        vtype = getType();

        if ( vtype == TYPE_INT8 )
                return idmef_value_get_int8(_value);

        else if ( vtype == TYPE_UINT8 )
                return idmef_value_get_uint8(_value);

        else if ( vtype == TYPE_INT16 )
                return idmef_value_get_int16(_value);

        else if ( vtype == TYPE_UINT16 )
                return idmef_value_get_uint16(_value);

        else if ( vtype == TYPE_INT32 )
                return idmef_value_get_int32(_value);

        else if ( vtype == TYPE_ENUM )
                return idmef_value_get_enum(_value);

        std::stringstream s;
        s << "Left value doesn't fit '" << idmef_value_type_to_string((idmef_value_type_id_t) vtype) << "' requirement";
        throw PreludeError(s.str());
}


IDMEFValue::operator uint32_t () const
{
        prelude_except_if_fail(_value);

        if ( getType() == TYPE_UINT32 )
                return idmef_value_get_uint32(_value);
        else
                return (int32_t) *this;
}


IDMEFValue::operator int64_t () const
{
        prelude_except_if_fail(_value);

        if ( getType() == TYPE_INT64 )
                return idmef_value_get_int64(_value);
        else
                return (uint32_t) *this;
}


IDMEFValue::operator uint64_t () const
{
        prelude_except_if_fail(_value);

        if ( getType() == TYPE_UINT64 )
                return idmef_value_get_uint64(_value);
        else
                return (int64_t) *this;
}


IDMEFValue::operator float () const
{
        IDMEFValueTypeEnum vtype;

        prelude_except_if_fail(_value);
        vtype = getType();

        if ( vtype == TYPE_FLOAT )
                return idmef_value_get_float(_value);

        else if ( vtype == TYPE_DATA ) {
                idmef_data_t *d = idmef_value_get_data(_value);

                if ( idmef_data_get_type(d) == IDMEF_DATA_TYPE_FLOAT )
                        return idmef_data_get_float(d);
        }

        std::stringstream s;
        s << "Left value doesn't fit '" << idmef_value_type_to_string((idmef_value_type_id_t) vtype) << "' requirement";
        throw PreludeError(s.str());
}


IDMEFValue::operator double () const
{
        prelude_except_if_fail(_value);

        if ( getType() == TYPE_DOUBLE )
                return idmef_value_get_double(_value);
        else
                return (float) *this;
}


std::string IDMEFValue::convert_string() const
{
        std::stringstream s;
        prelude_except_if_fail(_value);

        if ( getType() == TYPE_STRING )
                return prelude_string_get_string(idmef_value_get_string(_value));

        else if ( getType() == TYPE_TIME )
                return IDMEFTime(idmef_time_ref(idmef_value_get_time(_value)));

        else if ( getType() == TYPE_ENUM )
                return idmef_class_enum_to_string(idmef_value_get_class(_value), idmef_value_get_enum(_value));

        else if ( getType() == TYPE_DATA ) {
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

                else if ( t == IDMEF_DATA_TYPE_INT ) {
                        s << idmef_data_get_int(d);
                        return s.str();
                }

                else {
                        s << "Left value doesn't fit 'data' type '" << t << "' requirement";
                        throw PreludeError(s.str());
                }
        }

        s << "Left value doesn't fit '" << idmef_value_type_to_string((idmef_value_type_id_t) getType()) << "' requirement";
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


bool IDMEFValue::operator <= (const IDMEFValue &value) const
{
        return this->match(value, IDMEFCriterion::OPERATOR_LESSER|IDMEFCriterion::OPERATOR_EQUAL);
}


bool IDMEFValue::operator >= (const IDMEFValue &value) const
{
        return this->match(value, IDMEFCriterion::OPERATOR_GREATER|IDMEFCriterion::OPERATOR_EQUAL);
}


bool IDMEFValue::operator < (const IDMEFValue &value) const
{
        return this->match(value, IDMEFCriterion::OPERATOR_LESSER);
}


bool IDMEFValue::operator > (const IDMEFValue &value) const
{
        return this->match(value, IDMEFCriterion::OPERATOR_GREATER);
}


bool IDMEFValue::operator == (const IDMEFValue &value) const
{
        return this->match(value, IDMEFCriterion::OPERATOR_EQUAL);
}


bool IDMEFValue::operator == (const std::vector<IDMEFValue> &vlist) const
{
        return this->match(vlist, IDMEFCriterion::OPERATOR_EQUAL);
}


bool IDMEFValue::operator != (const IDMEFValue &value) const
{
        return this->match(value, IDMEFCriterion::OPERATOR_NOT|IDMEFCriterion::OPERATOR_EQUAL);
}
