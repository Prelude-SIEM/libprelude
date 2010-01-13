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

#ifndef _LIBPRELUDE_IDMEF_VALUE_HXX
#define _LIBPRELUDE_IDMEF_VALUE_HXX

#include <vector>
#include "idmef-time.hxx"

namespace Prelude {
        class IDMEFValue {
            private:
                void _InitFromString(const char *value);
                std::string convert_string() const;

            protected:
                idmef_value_t *_value;
                std::string _myconv;

            public:
                idmef_value_type_id_t GetType() const;
                bool IsNull() const;

                IDMEFValue();
                ~IDMEFValue();
                IDMEFValue(const IDMEFValue &value);
                IDMEFValue(std::vector<IDMEFValue> value);
                IDMEFValue(idmef_value_t *value);
                IDMEFValue(std::string value);
                IDMEFValue(const char *value);
                IDMEFValue(int8_t value);
                IDMEFValue(uint8_t value);
                IDMEFValue(int16_t value);
                IDMEFValue(uint16_t value);
                IDMEFValue(int32_t value);
                IDMEFValue(uint32_t value);
                IDMEFValue(int64_t value);
                IDMEFValue(uint64_t value);
                IDMEFValue(float value);
                IDMEFValue(double value);
                IDMEFValue(Prelude::IDMEFTime &time);

                int Match(const IDMEFValue &value, int op);
                IDMEFValue Clone() const;

                operator int32_t() const;
                operator uint32_t() const;
                operator int64_t() const;
                operator uint64_t() const;
                operator double() const;
                //operator std::string();
                operator std::vector<IDMEFValue>() const;
                operator const char*() const;
                operator float() const;
                operator Prelude::IDMEFTime() const;
                operator idmef_value_t *() const;

                IDMEFValue & operator=(const IDMEFValue &p);
        };
};

#endif
