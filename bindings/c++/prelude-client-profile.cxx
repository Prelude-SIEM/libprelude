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


const std::string ClientProfile::GetConfigFilename()
{
        _RETURN_NEW_BUFFER_FROM_FUNCTION_BUFFERSIZE(prelude_client_profile_get_config_filename);
}

const std::string ClientProfile::GetAnalyzeridFilename()
{
        _RETURN_NEW_BUFFER_FROM_FUNCTION_BUFFERSIZE(prelude_client_profile_get_analyzerid_filename);
}

const std::string ClientProfile::GetTlsKeyFilename()
{
        _RETURN_NEW_BUFFER_FROM_FUNCTION_BUFFERSIZE(prelude_client_profile_get_tls_key_filename);
}

const std::string ClientProfile::GetTlsServerCaCertFilename()
{
        _RETURN_NEW_BUFFER_FROM_FUNCTION_BUFFERSIZE(prelude_client_profile_get_tls_server_ca_cert_filename);
}

const std::string ClientProfile::GetTlsServerKeyCertFilename()
{
        _RETURN_NEW_BUFFER_FROM_FUNCTION_BUFFERSIZE(prelude_client_profile_get_tls_server_keycert_filename);
}

const std::string ClientProfile::GetTlsServerCrlFilename()
{
        _RETURN_NEW_BUFFER_FROM_FUNCTION_BUFFERSIZE(prelude_client_profile_get_tls_server_crl_filename);
}

const std::string ClientProfile::GetTlsClientKeyCertFilename()
{
        _RETURN_NEW_BUFFER_FROM_FUNCTION_BUFFERSIZE(prelude_client_profile_get_tls_client_keycert_filename);
}

const std::string ClientProfile::GetTlsClientTrustedCertFilename()
{
        _RETURN_NEW_BUFFER_FROM_FUNCTION_BUFFERSIZE(prelude_client_profile_get_tls_client_trusted_cert_filename);
}

const std::string ClientProfile::GetBackupDirname()
{
        _RETURN_NEW_BUFFER_FROM_FUNCTION_BUFFERSIZE(prelude_client_profile_get_backup_dirname);
}

const std::string ClientProfile::GetProfileDirname()
{
        _RETURN_NEW_BUFFER_FROM_FUNCTION_BUFFERSIZE(prelude_client_profile_get_profile_dirname);
}


void ClientProfile::SetPrefix(const char *prefix)
{
        int ret;

        ret = prelude_client_profile_set_prefix(_profile, prefix);
        if ( ret < 0 )
                throw PreludeError(ret);
}


const std::string ClientProfile::GetPrefix()
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
