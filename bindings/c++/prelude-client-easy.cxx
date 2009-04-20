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
        prelude_string_set_ref(string, _model);

        ret = idmef_analyzer_new_class(analyzer, &string);
        if ( ret < 0 )
                throw PreludeError(ret);
        prelude_string_set_ref(string, _class);

        ret = idmef_analyzer_new_manufacturer(analyzer, &string);
        if ( ret < 0 )
                throw PreludeError(ret);
        prelude_string_set_ref(string, _manufacturer);

        ret = idmef_analyzer_new_version(analyzer, &string);
        if ( ret < 0 )
                throw PreludeError(ret);
        prelude_string_set_ref(string, _version);
}


ClientEasy::ClientEasy(const char *profile,
                       int permission,
                       const char *_model,
                       const char *_class,
                       const char *_manufacturer,
                       const char *_version) : Client(profile)
{
        SetRequiredPermission(permission);

        SetFlags(GetFlags() | Client::FLAGS_ASYNC_TIMER);
        setup_analyzer(prelude_client_get_analyzer(GetClient()), _model, _class, _manufacturer, _version);
}
