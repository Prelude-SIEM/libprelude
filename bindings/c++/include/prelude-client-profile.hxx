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

                int getUid() { return (int)prelude_client_profile_get_uid(_profile); }
                int getGid() { return (int)prelude_client_profile_get_gid(_profile); }

                const char *getName() { return prelude_client_profile_get_name(_profile); }
                int setName(const char *name) { return prelude_client_profile_set_name(_profile,name); }

                /* XXX uint64_t has to be converted */
                uint64_t getAnalyzerId() { return prelude_client_profile_get_analyzerid(_profile); }
                void setAnalyzerId(uint64_t id) { prelude_client_profile_set_analyzerid(_profile,id); }

                const std::string getConfigFilename();
                const std::string getAnalyzeridFilename();

                const std::string getTlsKeyFilename();
                const std::string getTlsServerCaCertFilename();
                const std::string getTlsServerKeyCertFilename();
                const std::string getTlsServerCrlFilename();

                const std::string getTlsClientKeyCertFilename();
                const std::string getTlsClientTrustedCertFilename();

                const std::string getBackupDirname();
                const std::string getProfileDirname();

                void setPrefix(const char *prefix);
                const std::string getPrefix();

                operator prelude_client_profile_t *() const;
                ClientProfile &operator=(const ClientProfile &p);
        };
};

#endif

