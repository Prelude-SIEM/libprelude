#ifndef __PRELUDE_CLIENT_PROFILE__
#define __PRELUDE_CLIENT_PROFILE__

#include "prelude.h"

namespace Prelude {
        class ClientProfile {
            protected:
                bool _own_data;
                prelude_client_profile_t *_profile;

            public:
                ClientProfile();
                ClientProfile(const char *profile);
                ClientProfile(prelude_client_profile_t * profile);
                ~ClientProfile();

                int GetUid() { return (int)prelude_client_profile_get_uid(_profile); }
                int GetGid() { return (int)prelude_client_profile_get_gid(_profile); }

                const char *GetName() { return prelude_client_profile_get_name(_profile); }
                int SetName(const char *name) { return prelude_client_profile_set_name(_profile,name); }

                /* XXX uint64_t has to be converted */
                uint64_t GetAnalyzerId() { return prelude_client_profile_get_analyzerid(_profile); }
                void GetAnalyzerId(uint64_t id) { prelude_client_profile_set_analyzerid(_profile,id); }

                char * GetConfigFilename();
                char * GetAnalyzeridFilename();

                char * GetTlsKeyFilename();
                char * GetTlsServerCaCertFilename();
                char * GetTlsServerKeyCertFilename();
                char * GetTlsServerCrlFilename();

                char * GetTlsClientKeyCertFilename();
                char * GetTlsClientTrustedCertFilename();

                char * GetBackupDirname();
                char * GetProfileDirname();

                operator prelude_client_profile_t *() const;
        };
};

#endif

