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

#ifndef _LIBPRELUDE_IDMEF_PATH_HXX
#define _LIBPRELUDE_IDMEF_PATH_HXX

#include "prelude.h"
#include "idmef-path.h"

#include "idmef.hxx"
#include "prelude-error.hxx"
#include "idmef-value.hxx"


namespace Prelude {
        class IDMEFClass;

        class IDMEFPath {
            private:
                idmef_path_t *_path;

            public:
                IDMEFPath(const char *buffer);
                IDMEFPath(Prelude::IDMEF &idmef, const char *buffer);

                IDMEFPath(idmef_path_t *path);
                IDMEFPath(const IDMEFPath &path);
                ~IDMEFPath();

                Prelude::IDMEFValue get(const Prelude::IDMEF &message) const;
                void set(Prelude::IDMEF &message, const std::vector<Prelude::IDMEF> &value) const;
                void set(Prelude::IDMEF &message, Prelude::IDMEF *value) const;
                void set(Prelude::IDMEF &message, const std::vector<Prelude::IDMEFValue> &value) const;
                void set(Prelude::IDMEF &message, Prelude::IDMEFValue *value) const;
                void set(Prelude::IDMEF &message, Prelude::IDMEFValue &value) const;
                void set(Prelude::IDMEF &message, Prelude::IDMEFTime &time) const;
                void set(Prelude::IDMEF &message, const std::string &value) const;
                void set(Prelude::IDMEF &message, const char *value) const;
                void set(Prelude::IDMEF &message, int32_t value) const;
                void set(Prelude::IDMEF &message, int64_t value) const;
                void set(Prelude::IDMEF &message, uint64_t value) const;
                void set(Prelude::IDMEF &message, float value) const;
                void set(Prelude::IDMEF &message, double value) const;

                Prelude::IDMEFClass getClass(int depth=-1) const;
                Prelude::IDMEFValue::IDMEFValueTypeEnum getValueType(int depth=-1) const;
                int setIndex(unsigned int index, int depth=-1);
                int undefineIndex(int depth=-1);
                int getIndex(int depth=-1) const;
                int makeChild(const char *child_name, unsigned int index);
                int makeParent();
                int compare(IDMEFPath *path, int depth=-1) const;
                IDMEFPath clone() const;

                int checkOperator(idmef_criterion_operator_t op) const;
                idmef_criterion_operator_t getApplicableOperators() const;

                const char *getName(int depth=-1) const;
                bool isAmbiguous() const;
                int hasLists() const;
                bool isList(int depth=-1) const;
                unsigned int getDepth() const;

                IDMEFPath &operator = (const IDMEFPath &path);
                operator idmef_path_t*() const;
        };
};

#endif
