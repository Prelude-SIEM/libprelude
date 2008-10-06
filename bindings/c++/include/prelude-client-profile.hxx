#ifndef __PRELUDE_CLIENT_PROFILE__
#define __PRELUDE_CLIENT_PROFILE__

#include <string>
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
                ClientProfile(const ClientProfile &p);
                ~ClientProfile();

                int GetUid() { return (int)prelude_client_profile_get_uid(_profile); }
                int GetGid() { return (int)prelude_client_profile_get_gid(_profile); }

                const char *GetName() { return prelude_client_profile_get_name(_profile); }
                int SetName(const char *name) { return prelude_client_profile_set_name(_profile,name); }

                /* XXX uint64_t has to be converted */
                uint64_t GetAnalyzerId() { return prelude_client_profile_get_analyzerid(_profile); }
                void GetAnalyzerId(uint64_t id) { prelude_client_profile_set_analyzerid(_profile,id); }

                const std::string GetConfigFilename();
                const std::string GetAnalyzeridFilename();

                const std::string GetTlsKeyFilename();
                const std::string GetTlsServerCaCertFilename();
                const std::string GetTlsServerKeyCertFilename();
                const std::string GetTlsServerCrlFilename();

                const std::string GetTlsClientKeyCertFilename();
                const std::string GetTlsClientTrustedCertFilename();

                const std::string GetBackupDirname();
                const std::string GetProfileDirname();

                void SetPrefix(const char *prefix);
                const std::string GetPrefix();

                operator prelude_client_profile_t *() const;
                ClientProfile &operator=(const ClientProfile &p);
        };
};

#endif

