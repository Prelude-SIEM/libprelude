#include "config.h"

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "prelude.h"


static void set_value_check(idmef_message_t *idmef, const char *paths,
                            const char *str_value, prelude_bool_t verify_get)
{
        idmef_path_t *path;
        idmef_value_t *value;
        prelude_string_t *str;
        prelude_string_t *res;

        assert(idmef_path_new_fast(&path, paths) == 0);
        assert(prelude_string_new_ref(&str, str_value) == 0);
        assert(idmef_value_new_string(&value, str) == 0);

        if ( verify_get )
                assert(idmef_path_set(path, idmef, value) == 0);
        else
                assert(idmef_path_set(path, idmef, value) < 0);

        idmef_value_destroy(value);

        if ( ! verify_get ) {
                idmef_path_destroy(path);
                return;
        }

        assert(idmef_path_get(path, idmef, &value) > 0);

        assert(prelude_string_new(&res) == 0);
        assert(idmef_value_to_string(value, res) >= 0);
        assert(strcmp(str_value, prelude_string_get_string(res)) == 0);
        prelude_string_destroy(res);

        idmef_value_destroy(value);
        idmef_path_destroy(path);
}

int main(void)
{
        int i, ret;
        idmef_value_t *value;
        idmef_path_t *path;
        idmef_message_t *idmef;
        struct {
                const char *path;
                int depth;
                prelude_bool_t has_list;
                prelude_bool_t ambiguous;
                prelude_bool_t successful;
        } plist[] = {
                { "alert.classification.text", 3, FALSE, FALSE, TRUE },
                { "alert.assessment.impact.severity", 4, FALSE, FALSE, TRUE },
                { "alert.source.node.name", 4, TRUE, TRUE, TRUE },
                { "alert.target(0).node.name", 4, TRUE, FALSE, TRUE },
                { "alert.invalid.path", 0, FALSE, FALSE, FALSE }
        };

        assert(idmef_message_new(&idmef) == 0);

        for ( i = 0; i < sizeof(plist) / sizeof(*plist); i++ ) {
                ret = idmef_path_new_fast(&path, plist[i].path);
                assert((plist[i].successful == TRUE && ret == 0) || (plist[i].successful == FALSE && ret < 0));

                if ( ret < 0 )
                        continue;

                assert(strcmp(plist[i].path, idmef_path_get_name(path, -1)) == 0);
                assert(idmef_path_get_depth(path) == plist[i].depth);
                assert(idmef_path_has_lists(path) == plist[i].has_list);
                assert(idmef_path_is_ambiguous(path) == plist[i].ambiguous);

                if ( plist[i].ambiguous ) {
                        idmef_path_destroy(path);
                        continue;
                }

                /*
                 * Check whether setting NULL value work.
                 */
                ret = idmef_path_set(path, idmef, NULL);
                assert(ret == 0);

                ret = idmef_path_get(path, idmef, &value);
                assert(ret == 0); /* empty value */

                idmef_path_destroy(path);
        }

        set_value_check(idmef, "alert.classification.text", "Random value", TRUE);
        set_value_check(idmef, "alert.assessment.impact.severity", "high", TRUE);
        set_value_check(idmef, "alert.assessment.impact.severity", "Invalid enumeration", FALSE);

        exit(0);
}
