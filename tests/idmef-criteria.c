#include "config.h"

#include <stdlib.h>
#include <assert.h>
#include "prelude.h"

static void test_criteria(idmef_message_t *idmef, const char *criteria_str, int expect_create, int expect_match)
{
        idmef_criteria_t *criteria;

        if ( expect_create < 0 ) {
                assert(idmef_criteria_new_from_string(&criteria, criteria_str) < 0);
                return;
        } else
                assert(idmef_criteria_new_from_string(&criteria, criteria_str) == 0);

        assert(idmef_criteria_match(criteria, idmef) == expect_match);
        idmef_criteria_destroy(criteria);

}

int main(void)
{
        idmef_alert_t *alert;
        idmef_message_t *idmef;
        idmef_classification_t *classification;
        prelude_string_t *str;

        assert(prelude_string_new_ref(&str, "A") == 0);

        assert(idmef_message_new(&idmef) == 0);
        assert(idmef_message_new_alert(idmef, &alert) == 0);
        assert(idmef_alert_new_classification(alert, &classification) == 0);
        idmef_classification_set_text(classification, str);

        test_criteria(idmef, "alert", 0, 1);
        test_criteria(idmef, "heartbeat", 0, 0);
        test_criteria(idmef, "alert || heartbeat", 0, 1);
        test_criteria(idmef, "alert.classification.txt == A", -1, -1);
        test_criteria(idmef, "alert.classification.text = (A || B || C || D) || heartbeat", 0, 1);
        test_criteria(idmef, "(alert.classification.text == A || heartbeat", -1, -1);

        prelude_string_set_ref(str, "My String");

        test_criteria(idmef, "alert.classification.text != 'My String'", 0, 0);
        test_criteria(idmef, "alert.classification.text != 'random'", 0, 1);

        test_criteria(idmef, "alert.classification.text == 'My String'", 0, 1);
        test_criteria(idmef, "alert.classification.text <> 'My'", 0, 1);
        test_criteria(idmef, "alert.classification.text <> 'my'", 0, 0);
        test_criteria(idmef, "alert.classification.text <>* 'my'", 0, 1);

        test_criteria(idmef, "alert.classification.text ~ 'My String'", 0, 1);
        test_criteria(idmef, "alert.classification.text ~ 'My (String|Check)'", 0, 1);
        test_criteria(idmef, "alert.classification.text ~ 'my'", 0, 0);
        test_criteria(idmef, "alert.classification.text ~* 'my'", 0, 1);

        exit(0);
}
