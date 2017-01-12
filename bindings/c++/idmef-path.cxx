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

#include "idmef.hxx"
#include "idmef-path.hxx"
#include "idmef-class.hxx"
#include "prelude-error.hxx"

#include "idmef-object-prv.h"

using namespace Prelude;


IDMEFPath::IDMEFPath(const char *buffer)
{
        int ret;

        ret = idmef_path_new_fast(&_path, buffer);
        if ( ret < 0 )
                throw PreludeError(ret);
}



IDMEFPath::IDMEFPath(IDMEF &idmef, const char *buffer)
{
        int ret;
        idmef_object_t *obj = (idmef_object_t *) idmef;

        ret = idmef_path_new_from_root_fast(&_path, obj->_idmef_object_id, buffer);
        if ( ret < 0 )
                throw PreludeError(ret);
}



IDMEFPath::IDMEFPath(idmef_path_t *path)
{
        _path = path;
}


IDMEFPath::IDMEFPath(const IDMEFPath &path)
{
        _path = (path._path) ? idmef_path_ref(path._path) : NULL;
}


IDMEFPath::~IDMEFPath()
{
        idmef_path_destroy(_path);
}


IDMEFValue IDMEFPath::get(const IDMEF &message) const
{
        int ret;
        idmef_value_t *value;

        ret = idmef_path_get(_path, message, &value);
        if ( ret < 0 )
                throw PreludeError(ret);

        else if ( ret == 0 )
                return IDMEFValue((idmef_value_t *) NULL);

        return IDMEFValue(value);
}



Prelude::IDMEFValue::IDMEFValueTypeEnum IDMEFPath::getValueType(int depth) const
{
        return (Prelude::IDMEFValue::IDMEFValueTypeEnum) idmef_path_get_value_type(_path, depth);
}


int IDMEFPath::checkOperator(idmef_criterion_operator_t op) const
{
        return idmef_path_check_operator(_path, op);
}



idmef_criterion_operator_t IDMEFPath::getApplicableOperators() const
{
        int ret;
        idmef_criterion_operator_t res;

        ret = idmef_path_get_applicable_operators(_path, &res);
        if ( ret < 0 )
                throw PreludeError(ret);

        return res;
}



void IDMEFPath::set(IDMEF &message, const std::vector<IDMEF> &value) const
{
        int ret;
        IDMEFValue v = value;

        ret = idmef_path_set(_path, message, v);
        if ( ret < 0 )
                throw PreludeError(ret);
}


void IDMEFPath::set(IDMEF &message, IDMEF *value) const
{
        int ret;

        if ( ! value )
                ret = idmef_path_set(_path, message, NULL);
        else
                ret = idmef_path_set(_path, message, IDMEFValue(value));

        if ( ret < 0 )
                throw PreludeError(ret);
}


void IDMEFPath::set(IDMEF &message, IDMEFValue *value) const
{
        int ret;

        if ( ! value )
                ret = idmef_path_set(_path, message, NULL);
        else
                ret = idmef_path_set(_path, message, *value);

        if ( ret < 0 )
                throw PreludeError(ret);
}


void IDMEFPath::set(IDMEF &message, const std::vector<IDMEFValue> &value) const
{
        int ret;
        IDMEFValue v = value;

        ret = idmef_path_set(_path, message, v);
        if ( ret < 0 )
                throw PreludeError(ret);
}


void IDMEFPath::set(IDMEF &message, IDMEFValue &value) const
{
        int ret;

        ret = idmef_path_set(_path, message, value);
        if ( ret < 0 )
                throw PreludeError(ret);
}


void IDMEFPath::set(IDMEF &message, const std::string &value) const
{
        int ret;

        ret = idmef_path_set(_path, message, IDMEFValue(value));
        if ( ret < 0 )
                throw PreludeError(ret);
}


void IDMEFPath::set(IDMEF &message, const char *value) const
{
        int ret;

        if ( value )
                ret = idmef_path_set(_path, message, IDMEFValue(value));
        else
                ret = idmef_path_set(_path, message, (idmef_value_t *) NULL);

        if ( ret < 0 )
                throw PreludeError(ret);
}



void IDMEFPath::set(IDMEF &message, int32_t value) const
{
        int ret;

        ret = idmef_path_set(_path, message, IDMEFValue(value));
        if ( ret < 0 )
                throw PreludeError(ret);
}



void IDMEFPath::set(IDMEF &message, int64_t value) const
{
        int ret;

        ret = idmef_path_set(_path, message, IDMEFValue(value));
        if ( ret < 0 )
                throw PreludeError(ret);
}


void IDMEFPath::set(IDMEF &message, uint64_t value) const
{
        int ret;

        ret = idmef_path_set(_path, message, IDMEFValue(value));
        if ( ret < 0 )
                throw PreludeError(ret);
}


void IDMEFPath::set(IDMEF &message, float value) const
{
        int ret;

        ret = idmef_path_set(_path, message, IDMEFValue(value));
        if ( ret < 0 )
                throw PreludeError(ret);
}


void IDMEFPath::set(IDMEF &message, double value) const
{
        int ret;

        ret = idmef_path_set(_path, message, IDMEFValue(value));
        if ( ret < 0 )
                throw PreludeError(ret);
}


void IDMEFPath::set(IDMEF &message, IDMEFTime &time) const
{
        int ret;

        ret = idmef_path_set(_path, message, IDMEFValue(time));
        if ( ret < 0 )
                throw PreludeError(ret);
}



IDMEFClass IDMEFPath::getClass(int depth) const
{
        return IDMEFClass(idmef_path_get_class(_path, depth));
}


int IDMEFPath::setIndex(unsigned int index, int depth)
{
        if ( depth < 0 )
                depth = getDepth();

        return idmef_path_set_index(_path, depth, index);
}


int IDMEFPath::undefineIndex(int depth)
{
        if ( depth < 0 )
                depth = getDepth();

        return idmef_path_undefine_index(_path, depth);
}


int IDMEFPath::getIndex(int depth) const
{
        if ( depth < 0 )
                depth = getDepth();

        return idmef_path_get_index(_path, depth);
}


int IDMEFPath::makeChild(const char *child_name, unsigned int index=0)
{
        return idmef_path_make_child(_path, child_name, index);
}



int IDMEFPath::makeParent()
{
        return idmef_path_make_parent(_path);
}


int IDMEFPath::compare(IDMEFPath *path, int depth) const
{
        int ret;

        if ( depth < 0 )
                ret = idmef_path_compare(_path, path->_path);
        else
                ret = idmef_path_ncompare(_path, path->_path, depth);

        return ret;
}


IDMEFPath IDMEFPath::clone() const
{
        int ret;
        idmef_path_t *cpath;

        ret = idmef_path_clone(_path, &cpath);
        if ( ret < 0 )
                throw PreludeError(ret);

        return IDMEFPath(cpath);
}


const char *IDMEFPath::getName(int depth) const
{
        return idmef_path_get_name(_path, depth);
}


bool IDMEFPath::isAmbiguous() const
{
        return idmef_path_is_ambiguous(_path);
}


int IDMEFPath::hasLists() const
{
        return idmef_path_has_lists(_path);
}


bool IDMEFPath::isList(int depth) const
{
        return idmef_path_is_list(_path, depth);
}


unsigned int IDMEFPath::getDepth() const
{
        return idmef_path_get_depth(_path);
}


IDMEFPath &IDMEFPath::operator=(const IDMEFPath &path)
{
        if ( this != &path && _path != path._path ) {
                if ( _path )
                        idmef_path_destroy(_path);

                _path = (path._path) ? idmef_path_ref(path._path) : NULL;
        }

        return *this;
}


IDMEFPath::operator idmef_path_t *() const
{
        return _path;
}

