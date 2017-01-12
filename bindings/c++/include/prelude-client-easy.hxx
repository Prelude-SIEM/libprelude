/*****
*
* Copyright (C) 2009-2017 CS-SI. All Rights Reserved.
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

#ifndef _LIBPRELUDE_PRELUDE_CLIENT_EASY_HXX
#define _LIBPRELUDE_PRELUDE_CLIENT_EASY_HXX

#include "prelude.h"
#include "idmef.hxx"
#include "prelude-client.hxx"


namespace Prelude {
        class ClientEasy : public Client {
            private:
                void setup_analyzer(idmef_analyzer *analyzer,
                                    const char *_model,
                                    const char *_class,
                                    const char *_manufacturer,
                                    const char *version);

            public:
                ClientEasy(const char *profile,
                           int permission = Client::IDMEF_WRITE,
                           const char *model = "Unknown model",
                           const char *classname = "Unknown class",
                           const char *manufacturer = "Unknown manufacturer",
                           const char *version = "Unknown version");
        };
};

#endif
