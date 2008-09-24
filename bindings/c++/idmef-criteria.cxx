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

#include "idmef.hxx"
#include "idmef-criteria.hxx"
#include "prelude-error.hxx"

using namespace Prelude;

IDMEFCriteria::~IDMEFCriteria()
{
        if ( _criteria )
                idmef_criteria_destroy(_criteria);
}


IDMEFCriteria::IDMEFCriteria(const IDMEFCriteria &criteria)
{
        _criteria = (criteria._criteria) ? idmef_criteria_ref(criteria._criteria) : NULL;
}


IDMEFCriteria::IDMEFCriteria(idmef_criteria_t *criteria)
{
        _criteria = criteria;
}


IDMEFCriteria::IDMEFCriteria(const char *criteria)
{
        int ret;

        ret = idmef_criteria_new_from_string(&_criteria, criteria);
        if ( ret < 0 )
                throw PreludeError(ret);
}


IDMEFCriteria::IDMEFCriteria(const std::string &criteria)
{
        int ret;

        ret = idmef_criteria_new_from_string(&_criteria, criteria.c_str());
        if ( ret < 0 )
                throw PreludeError(ret);
}


IDMEFCriteria::IDMEFCriteria()
{
        int ret;

        ret = idmef_criteria_new(&_criteria);
        if ( ret < 0 )
                throw PreludeError(ret);
}


IDMEFCriteria IDMEFCriteria::Clone()
{
        int ret;
        idmef_criteria_t *cl;

        ret = idmef_criteria_clone(this->_criteria, &cl);
        if ( ret < 0 )
                throw PreludeError(ret);

        return IDMEFCriteria(cl);
}


void IDMEFCriteria::ANDCriteria(const IDMEFCriteria &criteria)
{
        idmef_criteria_and_criteria(this->_criteria, criteria._criteria);
}


void IDMEFCriteria::ORCriteria(const IDMEFCriteria &criteria)
{
        idmef_criteria_or_criteria(this->_criteria, criteria._criteria);
}


int IDMEFCriteria::Match(IDMEF *message)
{
        int ret;

        ret = idmef_criteria_match(this->_criteria, *message);
        if ( ret < 0 )
                throw PreludeError(ret);

        return ret;
}


const std::string IDMEFCriteria::ToString() const
{
        int ret;
        std::string s;
        prelude_string_t *str;

        ret = prelude_string_new(&str);
        if ( ret < 0 )
                throw PreludeError(ret);

        ret = idmef_criteria_to_string(_criteria, str);
        if ( ret < 0 ) {
                prelude_string_destroy(str);
                throw PreludeError(ret);
        }

        s = prelude_string_get_string(str);
        prelude_string_destroy(str);

        return s;
}


IDMEFCriteria::operator const std::string() const
{
        return ToString();
}


IDMEFCriteria &IDMEFCriteria::operator=(const IDMEFCriteria &criteria)
{
        if ( this != &criteria && _criteria != criteria._criteria ) {
                if ( _criteria )
                        idmef_criteria_destroy(_criteria);

                _criteria = (criteria._criteria) ? idmef_criteria_ref(criteria._criteria) : NULL;
        }

        return *this;
}
