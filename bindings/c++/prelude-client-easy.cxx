/*****
*
* Copyright (C) 2009-2020 CS-SI. All Rights Reserved.
* Author: Yoann Vandoorselaere <yoannv@gmail.com>
*
* This file is part of the Prelude library.
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU Lesser General Public License as published by
* the Free Software Foundation; either version 2.1, or (at your option)
* any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU Lesser General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public License along
* with this program; if not, write to the Free Software Foundation, Inc.,
* 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*
*****/

#include "idmef.hxx"
#include "prelude-client-profile.hxx"
#include "prelude-client-easy.hxx"
#include "prelude-error.hxx"

using namespace Prelude;

void ClientEasy::setup_analyzer(idmef_analyzer_t *analyzer,
                                const char *_model, const char *_class,
                                const char *_manufacturer, const char *_version)
{
        int ret;
        prelude_string_t *string;

        ret = idmef_analyzer_new_model(analyzer, &string);
        if ( ret < 0 )
                throw PreludeError(ret);
        prelude_string_set_dup(string, _model);

        ret = idmef_analyzer_new_class(analyzer, &string);
        if ( ret < 0 )
                throw PreludeError(ret);
        prelude_string_set_dup(string, _class);

        ret = idmef_analyzer_new_manufacturer(analyzer, &string);
        if ( ret < 0 )
                throw PreludeError(ret);
        prelude_string_set_dup(string, _manufacturer);

        ret = idmef_analyzer_new_version(analyzer, &string);
        if ( ret < 0 )
                throw PreludeError(ret);
        prelude_string_set_dup(string, _version);
}


ClientEasy::ClientEasy(const char *profile,
                       int permission,
                       const char *_model,
                       const char *_class,
                       const char *_manufacturer,
                       const char *_version) : Client(profile)
{
        setRequiredPermission(permission);

        setFlags(getFlags() | Client::FLAGS_ASYNC_TIMER);
        setup_analyzer(prelude_client_get_analyzer(getClient()), _model, _class, _manufacturer, _version);
}
