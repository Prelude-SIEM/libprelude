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

#ifndef _LIBPRELUDE_IDMEF_VALUE_HXX
#define _LIBPRELUDE_IDMEF_VALUE_HXX

#include <vector>
#include "idmef-time.hxx"

namespace Prelude
{
        class IDMEF;
}

namespace Prelude {
        class IDMEFValue {
            private:
                void _InitFromString(const char *value, size_t size);
                std::string convert_string() const;

            protected:
                idmef_value_t *_value;
                std::string _myconv;

            public:
                enum IDMEFValueTypeEnum {
                        TYPE_UNKNOWN    = IDMEF_VALUE_TYPE_UNKNOWN,
                        TYPE_INT8       = IDMEF_VALUE_TYPE_INT8,
                        TYPE_UINT8      = IDMEF_VALUE_TYPE_UINT8,
                        TYPE_INT16      = IDMEF_VALUE_TYPE_INT16,
                        TYPE_UINT16     = IDMEF_VALUE_TYPE_UINT16,
                        TYPE_INT32      = IDMEF_VALUE_TYPE_INT32,
                        TYPE_UINT32     = IDMEF_VALUE_TYPE_UINT32,
                        TYPE_INT64      = IDMEF_VALUE_TYPE_INT64,
                        TYPE_UINT64     = IDMEF_VALUE_TYPE_UINT64,
                        TYPE_FLOAT      = IDMEF_VALUE_TYPE_FLOAT,
                        TYPE_DOUBLE     = IDMEF_VALUE_TYPE_DOUBLE,
                        TYPE_STRING     = IDMEF_VALUE_TYPE_STRING,
                        TYPE_TIME       = IDMEF_VALUE_TYPE_TIME,
                        TYPE_DATA       = IDMEF_VALUE_TYPE_DATA,
                        TYPE_ENUM       = IDMEF_VALUE_TYPE_ENUM,
                        TYPE_LIST       = IDMEF_VALUE_TYPE_LIST,
                        TYPE_CLASS      = IDMEF_VALUE_TYPE_CLASS
                };

                IDMEFValueTypeEnum getType() const;
                bool isNull() const;

                IDMEFValue();
                ~IDMEFValue();
                IDMEFValue(IDMEF *idmef);
                IDMEFValue(const std::vector<IDMEF> &value);
                IDMEFValue(const IDMEFValue &value);
                IDMEFValue(const std::vector<IDMEFValue> &value);
                IDMEFValue(idmef_value_t *value);
                IDMEFValue(const std::string &value);
                IDMEFValue(const char *value);
                IDMEFValue(int32_t value);
                IDMEFValue(int64_t value);
                IDMEFValue(uint64_t value);
                IDMEFValue(float value);
                IDMEFValue(double value);
                IDMEFValue(Prelude::IDMEFTime &time);

                int match(const IDMEFValue &value, int op) const;
                IDMEFValue clone() const;
                const std::string toString() const;

                operator int32_t() const;
                operator uint32_t() const;
                operator int64_t() const;
                operator uint64_t() const;
                operator double() const;
                operator std::vector<IDMEFValue>() const;
                operator const char*() const;
                operator float() const;
                operator Prelude::IDMEFTime() const;
                operator idmef_value_t *() const;

                IDMEFValue & operator=(const IDMEFValue &p);

                bool operator == (const std::vector<IDMEFValue> &vlist) const;
                bool operator <= (const IDMEFValue &value) const;
                bool operator >= (const IDMEFValue &value) const;
                bool operator < (const IDMEFValue &value) const;
                bool operator > (const IDMEFValue &value) const;
                bool operator == (const IDMEFValue &value) const;
                bool operator != (const IDMEFValue &value) const;
        };
};

#endif
