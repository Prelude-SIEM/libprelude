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

#include "prelude-error.hxx"
#include "idmef-time.hxx"

using namespace Prelude;


IDMEFTime::~IDMEFTime()
{
        if ( _time )
                idmef_time_destroy(_time);
}


IDMEFTime::IDMEFTime(const IDMEFTime &time)
{
        _time = (time._time) ? idmef_time_ref(time._time) : NULL;
}


IDMEFTime::IDMEFTime()
{
        int ret;

        ret = idmef_time_new_from_gettimeofday(&_time);
        if ( ret < 0 )
                throw PreludeError(ret);

}


IDMEFTime::IDMEFTime(idmef_time_t *time)
{
        _time = time;
}


IDMEFTime::IDMEFTime(const time_t *time)
{
        int ret;

        ret = idmef_time_new_from_time(&_time, time);
        if ( ret < 0 )
                throw PreludeError(ret);
}


IDMEFTime::IDMEFTime(const char *str)
{
        int ret;

        ret = idmef_time_new_from_string(&_time, str);
        if ( ret < 0 )
                throw PreludeError(ret);
}


IDMEFTime::IDMEFTime(const struct timeval *tv)
{
        int ret;

        ret = idmef_time_new_from_timeval(&_time, tv);
        if ( ret < 0 )
                throw PreludeError(ret);
}



void IDMEFTime::Set(const time_t *t)
{
        idmef_time_set_from_time(_time, t);
}


void IDMEFTime::Set(const struct timeval *tv)
{
        int ret;

        ret = idmef_time_set_from_timeval(_time, tv);
        if ( ret < 0 )
                throw PreludeError(ret);
}



void IDMEFTime::Set(const char *str)
{
        int ret;

        ret = idmef_time_set_from_string(_time, str);
        if ( ret < 0 )
                throw PreludeError(ret);
}


void IDMEFTime::Set()
{
        int ret;

        ret = idmef_time_set_from_gettimeofday(_time);
        if ( ret < 0 )
                throw PreludeError(ret);
}


void IDMEFTime::SetSec(uint32_t sec)
{
        idmef_time_set_sec(_time, sec);
}


void IDMEFTime::SetUSec(uint32_t usec)
{
        idmef_time_set_usec(_time, usec);
}


void IDMEFTime::SetGmtOffset(int32_t gmtoff)
{
        idmef_time_set_gmt_offset(_time, gmtoff);
}


uint32_t IDMEFTime::GetSec() const
{
        return idmef_time_get_sec(_time);
}


uint32_t IDMEFTime::GetUSec() const
{
        return idmef_time_get_usec(_time);
}


int32_t IDMEFTime::GetGmtOffset() const
{
        return idmef_time_get_gmt_offset(_time);
}


IDMEFTime IDMEFTime::Clone()
{
        int ret;
        idmef_time_t *clone;

        ret = idmef_time_clone(_time, &clone);
        if ( ret < 0 )
                throw PreludeError(ret);

        return IDMEFTime(clone);
}


const std::string IDMEFTime::ToString() const
{
        int ret;
        std::string cs;
        prelude_string_t *str = NULL;

        ret = prelude_string_new(&str);
        if ( ret < 0 )
                throw PreludeError(ret);

        ret = idmef_time_to_string(_time, str);
        if ( ret < 0 )
                throw PreludeError(ret);

        cs = prelude_string_get_string(str);
        prelude_string_destroy(str);

        return cs;
}


bool IDMEFTime::operator <= (const IDMEFTime &time)
{
        return ( (double) *this <= (double) time );
}


bool IDMEFTime::operator < (const IDMEFTime &time)
{
        return ( (double) *this < (double) time );
}


bool IDMEFTime::operator >= (const IDMEFTime &time)
{
        return ( (double) *this >= (double) time );
}


bool IDMEFTime::operator > (const IDMEFTime &time)
{
        return ( (double) *this > (double) time );
}


bool IDMEFTime::operator != (const IDMEFTime &time)
{
        return ( (double) *this != (double) time );
}


bool IDMEFTime::operator == (const IDMEFTime &time)
{
        return ( (double) *this == (double) time );
}


IDMEFTime::operator int() const
{
        return GetSec();
}


IDMEFTime::operator long() const
{
        return GetSec();
}


IDMEFTime::operator double() const
{
        return GetSec() + (GetUSec() * 1e-6);
}


IDMEFTime::operator const std::string() const
{
        return ToString();
}


IDMEFTime::operator idmef_time_t *() const
{
        return _time;
}


IDMEFTime &IDMEFTime::operator=(const IDMEFTime &time)
{
        if ( this != &time && _time != time._time ) {
                if ( _time )
                        idmef_time_destroy(_time);

                _time = (time._time) ? idmef_time_ref(time._time) : NULL;
        }

        return *this;
}

