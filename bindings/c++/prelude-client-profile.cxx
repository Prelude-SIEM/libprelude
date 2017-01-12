/*****
*
* Copyright (C) 2009-2017 CS-SI. All Rights Reserved.
* Author: Yoann Vandoorselaere <yoann@gmail.com>
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

#include "prelude-error.hxx"
#include "prelude-client-profile.h"
#include "prelude-client-profile.hxx"


using namespace Prelude;



#define _RETURN_NEW_BUFFER_FROM_FUNCTION_BUFFERSIZE(function) do {    \
                char buf[PATH_MAX];                                   \
                                                                      \
                (function)(_profile, buf, sizeof(buf));               \
                                                                      \
                return std::string(buf);                              \
} while(0)


ClientProfile::ClientProfile()
{
        _profile = NULL;
}


ClientProfile::ClientProfile(prelude_client_profile_t *profile) : _profile(profile)
{
        _profile = profile;
}


ClientProfile::ClientProfile(const char *profile)
{
        int ret;

        ret = prelude_client_profile_new(&_profile, profile);
        if ( ret < 0 )
                throw PreludeError(ret);
}

ClientProfile::ClientProfile(const ClientProfile &p)
{
        _profile = (p._profile) ? prelude_client_profile_ref(p._profile) : NULL;
}


ClientProfile::~ClientProfile()
{
        if ( _profile )
                prelude_client_profile_destroy(_profile);
}


const std::string ClientProfile::getConfigFilename()
{
        _RETURN_NEW_BUFFER_FROM_FUNCTION_BUFFERSIZE(prelude_client_profile_get_config_filename);
}

const std::string ClientProfile::getAnalyzeridFilename()
{
        _RETURN_NEW_BUFFER_FROM_FUNCTION_BUFFERSIZE(prelude_client_profile_get_analyzerid_filename);
}

const std::string ClientProfile::getTlsKeyFilename()
{
        _RETURN_NEW_BUFFER_FROM_FUNCTION_BUFFERSIZE(prelude_client_profile_get_tls_key_filename);
}

const std::string ClientProfile::getTlsServerCaCertFilename()
{
        _RETURN_NEW_BUFFER_FROM_FUNCTION_BUFFERSIZE(prelude_client_profile_get_tls_server_ca_cert_filename);
}

const std::string ClientProfile::getTlsServerKeyCertFilename()
{
        _RETURN_NEW_BUFFER_FROM_FUNCTION_BUFFERSIZE(prelude_client_profile_get_tls_server_keycert_filename);
}

const std::string ClientProfile::getTlsServerCrlFilename()
{
        _RETURN_NEW_BUFFER_FROM_FUNCTION_BUFFERSIZE(prelude_client_profile_get_tls_server_crl_filename);
}

const std::string ClientProfile::getTlsClientKeyCertFilename()
{
        _RETURN_NEW_BUFFER_FROM_FUNCTION_BUFFERSIZE(prelude_client_profile_get_tls_client_keycert_filename);
}

const std::string ClientProfile::getTlsClientTrustedCertFilename()
{
        _RETURN_NEW_BUFFER_FROM_FUNCTION_BUFFERSIZE(prelude_client_profile_get_tls_client_trusted_cert_filename);
}

const std::string ClientProfile::getBackupDirname()
{
        _RETURN_NEW_BUFFER_FROM_FUNCTION_BUFFERSIZE(prelude_client_profile_get_backup_dirname);
}

const std::string ClientProfile::getProfileDirname()
{
        _RETURN_NEW_BUFFER_FROM_FUNCTION_BUFFERSIZE(prelude_client_profile_get_profile_dirname);
}


void ClientProfile::setPrefix(const char *prefix)
{
        int ret;

        ret = prelude_client_profile_set_prefix(_profile, prefix);
        if ( ret < 0 )
                throw PreludeError(ret);
}


const std::string ClientProfile::getPrefix()
{
        _RETURN_NEW_BUFFER_FROM_FUNCTION_BUFFERSIZE(prelude_client_profile_get_prefix);
}



ClientProfile::operator prelude_client_profile_t *() const
{
        return _profile;
}


ClientProfile &ClientProfile::operator=(const ClientProfile &p)
{
        if ( this != &p && _profile != p._profile ) {
                if ( _profile )
                        prelude_client_profile_destroy(_profile);

                _profile = (p._profile) ? prelude_client_profile_ref(p._profile) : NULL;
        }

        return *this;
}
