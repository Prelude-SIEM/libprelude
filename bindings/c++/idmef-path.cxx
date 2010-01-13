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
#include "idmef-path.hxx"
#include "prelude-error.hxx"

using namespace Prelude;


IDMEFPath::IDMEFPath(const char *buffer)
{
        int ret;

        ret = idmef_path_new_fast(&_path, buffer);
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


IDMEFValue IDMEFPath::Get(IDMEF &message)
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



idmef_value_type_id_t IDMEFPath::GetValueType(int depth)
{
        return idmef_path_get_value_type(_path, depth);
}


int IDMEFPath::CheckOperator(idmef_criterion_operator_t op)
{
        return idmef_path_check_operator(_path, op);
}



idmef_criterion_operator_t IDMEFPath::GetApplicableOperators()
{
        int ret;
        idmef_criterion_operator_t res;

        ret = idmef_path_get_applicable_operators(_path, &res);
        if ( ret < 0 )
                throw PreludeError(ret);

        return res;
}


void IDMEFPath::Set(IDMEF &message, IDMEFValue *value)
{
        int ret;

        if ( ! value )
                ret = idmef_path_set(_path, message, NULL);
        else
                ret = idmef_path_set(_path, message, *value);

        if ( ret < 0 )
                throw PreludeError(ret);
}


void IDMEFPath::Set(IDMEF &message, std::vector<IDMEFValue> value)
{
        int ret;
        IDMEFValue v = value;

        ret = idmef_path_set(_path, message, v);
        if ( ret < 0 )
                throw PreludeError(ret);
}


void IDMEFPath::Set(IDMEF &message, IDMEFValue &value)
{
        int ret;

        ret = idmef_path_set(_path, message, value);
        if ( ret < 0 )
                throw PreludeError(ret);
}


void IDMEFPath::Set(IDMEF &message, std::string value)
{
        int ret;

        ret = idmef_path_set(_path, message, IDMEFValue(value));
        if ( ret < 0 )
                throw PreludeError(ret);
}


void IDMEFPath::Set(IDMEF &message, const char *value)
{
        int ret;

        if ( value )
                ret = idmef_path_set(_path, message, IDMEFValue(value));
        else
                ret = idmef_path_set(_path, message, (idmef_value_t *) NULL);

        if ( ret < 0 )
                throw PreludeError(ret);
}



void IDMEFPath::Set(IDMEF &message, int8_t value)
{
        int ret;

        ret = idmef_path_set(_path, message, IDMEFValue(value));
        if ( ret < 0 )
                throw PreludeError(ret);
}


void IDMEFPath::Set(IDMEF &message, uint8_t value)
{
        int ret;

        ret = idmef_path_set(_path, message, IDMEFValue(value));
        if ( ret < 0 )
                throw PreludeError(ret);
}


void IDMEFPath::Set(IDMEF &message, int16_t value)
{
        int ret;

        ret = idmef_path_set(_path, message, IDMEFValue(value));
        if ( ret < 0 )
                throw PreludeError(ret);
}


void IDMEFPath::Set(IDMEF &message, uint16_t value)
{
        int ret;

        ret = idmef_path_set(_path, message, IDMEFValue(value));
        if ( ret < 0 )
                throw PreludeError(ret);
}

void IDMEFPath::Set(IDMEF &message, int32_t value)
{
        int ret;

        ret = idmef_path_set(_path, message, IDMEFValue(value));
        if ( ret < 0 )
                throw PreludeError(ret);
}


void IDMEFPath::Set(IDMEF &message, uint32_t value)
{
        int ret;

        ret = idmef_path_set(_path, message, IDMEFValue(value));
        if ( ret < 0 )
                throw PreludeError(ret);
}

void IDMEFPath::Set(IDMEF &message, int64_t value)
{
        int ret;

        ret = idmef_path_set(_path, message, IDMEFValue(value));
        if ( ret < 0 )
                throw PreludeError(ret);
}


void IDMEFPath::Set(IDMEF &message, uint64_t value)
{
        int ret;

        ret = idmef_path_set(_path, message, IDMEFValue(value));
        if ( ret < 0 )
                throw PreludeError(ret);
}


void IDMEFPath::Set(IDMEF &message, float value)
{
        int ret;

        ret = idmef_path_set(_path, message, IDMEFValue(value));
        if ( ret < 0 )
                throw PreludeError(ret);
}


void IDMEFPath::Set(IDMEF &message, double value)
{
        int ret;

        ret = idmef_path_set(_path, message, IDMEFValue(value));
        if ( ret < 0 )
                throw PreludeError(ret);
}


void IDMEFPath::Set(IDMEF &message, IDMEFTime &time)
{
        int ret;

        ret = idmef_path_set(_path, message, IDMEFValue(time));
        if ( ret < 0 )
                throw PreludeError(ret);
}



idmef_class_id_t IDMEFPath::GetClass(int depth)
{
        return idmef_path_get_class(_path, depth);
}


int IDMEFPath::SetIndex(unsigned int index, int depth)
{
        if ( depth < 0 )
                depth = GetDepth();

        return idmef_path_set_index(_path, depth, index);
}


int IDMEFPath::UndefineIndex(int depth)
{
        if ( depth < 0 )
                depth = GetDepth();

        return idmef_path_undefine_index(_path, depth);
}


int IDMEFPath::GetIndex(int depth)
{
        if ( depth < 0 )
                depth = GetDepth();

        return idmef_path_get_index(_path, depth);
}


int IDMEFPath::MakeChild(const char *child_name, unsigned int index=0)
{
        return idmef_path_make_child(_path, child_name, index);
}



int IDMEFPath::MakeParent()
{
        return idmef_path_make_parent(_path);
}


int IDMEFPath::Compare(IDMEFPath *path, int depth)
{
        int ret;

        if ( depth < 0 )
                ret = idmef_path_compare(_path, path->_path);
        else
                ret = idmef_path_ncompare(_path, path->_path, depth);

        return ret;
}


IDMEFPath IDMEFPath::Clone()
{
        int ret;
        idmef_path_t *cpath;

        ret = idmef_path_clone(_path, &cpath);
        if ( ret < 0 )
                throw PreludeError(ret);

        return IDMEFPath(cpath);
}


const char *IDMEFPath::GetName(int depth)
{
        return idmef_path_get_name(_path, depth);
}


bool IDMEFPath::IsAmbiguous()
{
        return idmef_path_is_ambiguous(_path);
}


int IDMEFPath::HasLists()
{
        return idmef_path_has_lists(_path);
}


bool IDMEFPath::IsList(int depth)
{
        return idmef_path_is_list(_path, depth);
}


unsigned int IDMEFPath::GetDepth()
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
