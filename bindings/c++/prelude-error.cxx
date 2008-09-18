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

#include <string>
#include <iostream>

#include "prelude-error.hxx"
#include <prelude.h>


using namespace Prelude;


PreludeError::PreludeError(const std::string message) throw()
{
        _error = -1;
        _message = message;
}


PreludeError::PreludeError(int error) throw()
{
        _error = error;
        _message = prelude_strerror(error);
}


const char *PreludeError::what() const throw()
{
        return _message.c_str();
}


PreludeError::operator const char *()
{
        return _message.c_str();
}


PreludeError::operator const std::string() const
{
        return _message;
}
