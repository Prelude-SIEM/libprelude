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

#ifndef _LIBPRELUDE_IDMEF_PATH_HXX
#define _LIBPRELUDE_IDMEF_PATH_HXX

#include "prelude.h"
#include "idmef-path.h"

#include "idmef.hxx"
#include "prelude-error.hxx"
#include "idmef-value.hxx"


namespace Prelude {
        class IDMEFPath {
            private:
                idmef_path_t *_path;

            public:
                IDMEFPath(const char *buffer);
                IDMEFPath(idmef_path_t *path);
                IDMEFPath(const IDMEFPath &path);
                ~IDMEFPath();

                Prelude::IDMEFValue Get(Prelude::IDMEF &message);
                void Set(Prelude::IDMEF &message, std::vector<Prelude::IDMEFValue> value);
                void Set(Prelude::IDMEF &message, Prelude::IDMEFValue *value);
                void Set(Prelude::IDMEF &message, Prelude::IDMEFValue &value);
                void Set(Prelude::IDMEF &message, Prelude::IDMEFTime &time);
                void Set(Prelude::IDMEF &message, std::string value);
                void Set(Prelude::IDMEF &message, const char *value);
                void Set(Prelude::IDMEF &message, int8_t value);
                void Set(Prelude::IDMEF &message, uint8_t value);
                void Set(Prelude::IDMEF &message, int16_t value);
                void Set(Prelude::IDMEF &message, uint16_t value);
                void Set(Prelude::IDMEF &message, int32_t value);
                void Set(Prelude::IDMEF &message, uint32_t value);
                void Set(Prelude::IDMEF &message, int64_t value);
                void Set(Prelude::IDMEF &message, uint64_t value);
                void Set(Prelude::IDMEF &message, float value);
                void Set(Prelude::IDMEF &message, double value);

                idmef_class_id_t GetClass(int depth=-1);
                idmef_value_type_id_t GetValueType(int depth=-1);
                int SetIndex(unsigned int index, int depth=-1);
                int UndefineIndex(int depth=-1);
                int GetIndex(int depth=-1);
                int MakeChild(const char *child_name, unsigned int index);
                int MakeParent();
                int Compare(IDMEFPath *path, int depth=-1);
                IDMEFPath Clone();

                int CheckOperator(idmef_criterion_operator_t op);
                idmef_criterion_operator_t GetApplicableOperators();

                //ref ?
                const char *GetName(int depth=-1);
                bool IsAmbiguous();
                int HasLists();
                bool IsList(int depth=-1);
                unsigned int GetDepth();

                IDMEFPath &operator = (const IDMEFPath &path);
        };
};

#endif
