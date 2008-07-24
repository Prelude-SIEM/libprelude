#include "prelude-error.hxx"
#include "prelude-client-profile.h"
#include "prelude-client-profile.hxx"


using namespace Prelude;


#define _RETURN_NEW_BUFFER_FROM_FUNCTION_BUFFERSIZE(function) \
        do { \
                char  *buf; \
                size_t s = 1024; \
                buf = new char[1024]; \
                (function)(_profile,buf,s); \
                return buf; \
        } while(0)


ClientProfile::ClientProfile()
{
        _profile = NULL;
        _own_data = FALSE;
}


ClientProfile::ClientProfile(prelude_client_profile_t *profile) : _profile(profile)
{
        _profile = profile;
        _own_data = FALSE;
}


ClientProfile::ClientProfile(const char *profile)
{
        int ret;

        ret = prelude_client_profile_new(&_profile, profile);
        if ( ret < 0 )
                throw PreludeError(ret);

        _own_data = TRUE;
}


ClientProfile::~ClientProfile()
{
        if ( _own_data )
                prelude_client_profile_destroy(_profile);
}


char * ClientProfile::GetConfigFilename()
{
        _RETURN_NEW_BUFFER_FROM_FUNCTION_BUFFERSIZE(prelude_client_profile_get_config_filename);
}

char * ClientProfile::GetAnalyzeridFilename()
{
        _RETURN_NEW_BUFFER_FROM_FUNCTION_BUFFERSIZE(prelude_client_profile_get_analyzerid_filename);
}

char * ClientProfile::GetTlsKeyFilename()
{
        _RETURN_NEW_BUFFER_FROM_FUNCTION_BUFFERSIZE(prelude_client_profile_get_tls_key_filename);
}

char * ClientProfile::GetTlsServerCaCertFilename()
{
        _RETURN_NEW_BUFFER_FROM_FUNCTION_BUFFERSIZE(prelude_client_profile_get_tls_server_ca_cert_filename);
}

char * ClientProfile::GetTlsServerKeyCertFilename()
{
        _RETURN_NEW_BUFFER_FROM_FUNCTION_BUFFERSIZE(prelude_client_profile_get_tls_server_keycert_filename);
}

char * ClientProfile::GetTlsServerCrlFilename()
{
        _RETURN_NEW_BUFFER_FROM_FUNCTION_BUFFERSIZE(prelude_client_profile_get_tls_server_crl_filename);
}

char * ClientProfile::GetTlsClientKeyCertFilename()
{
        _RETURN_NEW_BUFFER_FROM_FUNCTION_BUFFERSIZE(prelude_client_profile_get_tls_client_keycert_filename);
}

char * ClientProfile::GetTlsClientTrustedCertFilename()
{
        _RETURN_NEW_BUFFER_FROM_FUNCTION_BUFFERSIZE(prelude_client_profile_get_tls_client_trusted_cert_filename);
}

char * ClientProfile::GetBackupDirname()
{
        _RETURN_NEW_BUFFER_FROM_FUNCTION_BUFFERSIZE(prelude_client_profile_get_backup_dirname);
}

char * ClientProfile::GetProfileDirname()
{
        _RETURN_NEW_BUFFER_FROM_FUNCTION_BUFFERSIZE(prelude_client_profile_get_profile_dirname);
}


ClientProfile::operator prelude_client_profile_t *() const
{
        return _profile;
}
