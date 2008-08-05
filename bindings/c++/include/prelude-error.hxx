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

#ifndef _LIBPRELUDE_PRELUDE_ERROR_HXX
#define _LIBPRELUDE_PRELUDE_ERROR_HXX

#include <string>
#include <exception>

#define prelude_except_if_fail(cond) do {                                   \
        if ( ! (cond) )                                                     \
                throw PreludeError(prelude_error(PRELUDE_ERROR_ASSERTION)); \
} while(0)


namespace Prelude {
        class PreludeError: public std::exception {
            private:
                int _error;
                std::string _message;

            public:
                virtual ~PreludeError() throw() {};
                PreludeError(int error) throw();
                PreludeError(const std::string message) throw();

                virtual const char *what() const throw();
                operator const char *();
                operator const std::string() const;
        };
};

#endif
