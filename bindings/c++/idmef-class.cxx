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

#include <string>
#include <prelude.h>
#include <idmef-value-type.h>

#include "prelude.hxx"


using namespace Prelude;



IDMEFClass::IDMEFClass(idmef_class_id_t id)
{
        _id = id;
        _depth = 0;
}


IDMEFClass::IDMEFClass(const IDMEFPath &path)
{
        int i;
        IDMEFClass root;

        for ( i = 0; i < path.getDepth(); i++ )
                root = root.get(path.getName(i));

        *this = root;
}



IDMEFClass::IDMEFClass(const std::string &path)
{
        *this = IDMEFClass(IDMEFPath(path.c_str()));
}


IDMEFClass::IDMEFClass(IDMEFClass &parent, int child_id, int depth)
{
        IDMEFClass::IDMEFClassElem elem;

        if ( depth >= 16 )
                throw PreludeError(prelude_error(PRELUDE_ERROR_IDMEF_PATH_DEPTH));

        _depth = depth;
        _pathelem = parent._pathelem;
        _id = idmef_class_get_child_class(parent._id, child_id);

        elem.parent_id = parent._id;
        elem.idx = child_id;
        _pathelem.push_back(elem);
}


std::string IDMEFClass::toString(void)
{
        unsigned int i = 0;
        std::string s  = "IDMEFClass(" + getName();

        do {
                if ( i > 0 )
                        s += ", ";

                try {
                        s += get(i++).toString();
                } catch(...) {
                        break;
                }
        } while ( TRUE );

        s += "\n)";

        return s;
}


bool IDMEFClass::isList(void)
{
        if ( _pathelem.size() == 0 )
                throw PreludeError("Already in rootclass, cannot retrieve parents info");

        return idmef_class_is_child_list(_pathelem.back().parent_id, _pathelem.back().idx);
}


bool IDMEFClass::isKeyedList(void)
{
        if ( _pathelem.size() == 0 )
                throw PreludeError("Already in rootclass, cannot retrieve parents info");

        return idmef_class_is_child_keyed_list(_pathelem.back().parent_id, _pathelem.back().idx);
}


Prelude::IDMEFValue::IDMEFValueTypeEnum IDMEFClass::getValueType(void)
{
        if ( _pathelem.size() == 0 )
                throw PreludeError("Already in rootclass, cannot retrieve parents info");

        return (Prelude::IDMEFValue::IDMEFValueTypeEnum) idmef_class_get_child_value_type(_pathelem.back().parent_id, _pathelem.back().idx);
}


std::string IDMEFClass::getName(void)
{
        if ( _pathelem.size() == 0 )
                return idmef_class_get_name(_id);

        return idmef_class_get_child_name(_pathelem.back().parent_id, _pathelem.back().idx);
}


size_t IDMEFClass::getDepth(void)
{
        return _pathelem.size();
}


std::string IDMEFClass::getPath(int rootidx, int depth, const std::string &sep, const std::string &listidx)
{
        std::string out;

        if ( depth >= 0 ) {
                if ( (depth + 1) == _pathelem.size() )
                        return getName();

                IDMEFClassElem elem = _pathelem[depth];
                return idmef_class_get_child_name(elem.parent_id, elem.idx);
        }

        for ( std::vector<IDMEFClassElem>::iterator it = _pathelem.begin() + rootidx; it != _pathelem.end(); it++) {
                out += idmef_class_get_child_name((*it).parent_id, (*it).idx);

                if ( idmef_class_is_child_list((*it).parent_id, (*it).idx) )
                        out += listidx;

                if ( it + 1 != _pathelem.end() )
                        out += sep;
        }

        return out;
}



IDMEFClass IDMEFClass::get(const std::string &name)
{
        int i = idmef_class_find_child(_id, name.c_str());

        if ( i < 0 )
                throw PreludeError(i);

        return get(i);
}


IDMEFClass IDMEFClass::get(int i)
{
        idmef_class_id_t cl;
        idmef_value_type_id_t vl;

        cl = idmef_class_get_child_class(_id, i);
        if ( cl < 0 ) {
                vl = (idmef_value_type_id_t) idmef_class_get_child_value_type(_id, i);
                if ( vl < 0 )
                        throw PreludeError(vl);
        }

        return IDMEFClass(*this, i, _depth + 1);
}



std::vector<std::string> IDMEFClass::getEnumValues(void)
{
        int i = 0;
        const char *ret;
        std::vector<std::string> ev;

        if ( getValueType() != IDMEFValue::TYPE_ENUM )
                throw PreludeError("Input class is not enumeration");

        do {
                ret = idmef_class_enum_to_string(_id, i++);
                if ( ret )
                        ev.push_back(ret);

        } while ( ret || i == 1 ); /* entry 0 might be NULL, if the enumeration has no default value */

        return ev;
}



IDMEFCriterion::IDMEFCriterionOperatorEnum IDMEFClass::getApplicableOperator(void)
{
        int ret;
        idmef_criterion_operator_t op;

        ret = idmef_value_type_get_applicable_operators((idmef_value_type_id_t) getValueType(), &op);
        if ( ret < 0 )
                throw PreludeError(ret);

        return (IDMEFCriterion::IDMEFCriterionOperatorEnum) ret;
}
