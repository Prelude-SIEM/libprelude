/*****
*
* Copyright (C) 2014-2017 CS-SI. All Rights Reserved.
* Author: Yoann Vandoorselaere <yoannv@gmail.com>
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

#ifndef _LIBPRELUDE_IDMEF_CLASS_HXX
#define _LIBPRELUDE_IDMEF_CLASS_HXX

#include <string>
#include <vector>

#include "idmef-criteria.hxx"
#include "idmef-path.hxx"

namespace Prelude {
        class IDMEFClass {
            private:
                class IDMEFClassElem {
                        public:
                                int idx;
                                idmef_class_id_t parent_id;
                };

                int _depth;
                idmef_class_id_t _id;
                std::vector<IDMEFClass::IDMEFClassElem> _pathelem;

                IDMEFClass(IDMEFClass &parent, int child_id, int depth=0);
            public:
                IDMEFClass(idmef_class_id_t id=IDMEF_CLASS_ID_MESSAGE);
                IDMEFClass(const IDMEFPath &path);
                IDMEFClass(const std::string &path);

                size_t getDepth(void);
                IDMEFClass get(int child);
                IDMEFClass get(const std::string &name);

                size_t getChildCount() { return idmef_class_get_child_count(_id); };

                /* main object operation */
                bool isList(void);
                bool isKeyedList(void);
                std::string getName(void);
                std::string toString(void);
                Prelude::IDMEFValue::IDMEFValueTypeEnum getValueType(void);
                std::string getPath(int rootidx=0, int depth=-1, const std::string &sep = ".", const std::string &listidx="");
                std::vector<std::string> getEnumValues(void);
                Prelude::IDMEFCriterion::IDMEFCriterionOperatorEnum getApplicableOperator(void);
        };
};

#endif
