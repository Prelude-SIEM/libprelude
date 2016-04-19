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
        idmef_time_t *ctime;
        idmef_alert_t *alert;
        idmef_message_t *idmef;
        idmef_classification_t *classification;
        prelude_string_t *str;

        assert(prelude_string_new_ref(&str, "A") == 0);

        assert(idmef_message_new(&idmef) == 0);
        assert(idmef_message_new_alert(idmef, &alert) == 0);
        assert(idmef_alert_new_classification(alert, &classification) == 0);
        idmef_classification_set_text(classification, str);

        idmef_message_set_string(idmef, "alert.analyzer(0).name", "A");
        idmef_message_set_string(idmef, "alert.analyzer(1).name", "B");
        idmef_message_set_string(idmef, "alert.analyzer(1).ostype", "C");
        idmef_message_set_string(idmef, "alert.analyzer(1).process.arg(0)", "a0");
        idmef_message_set_string(idmef, "alert.analyzer(1).process.arg(2)", "a2");

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

        idmef_alert_new_create_time(alert, &ctime);
        assert(idmef_time_set_from_string(ctime, "2015-05-03 1:59:08") == 0);

        /*
         * Regular time operator check
         */
        test_criteria(idmef, "alert.create_time == '2015-05-03 1:59:08'", 0, 1);
        test_criteria(idmef, "alert.create_time != '2015-05-03 1:59:08'", 0, 0);
        test_criteria(idmef, "alert.create_time < '2015-05-03 1:59:08'", 0, 0);
        test_criteria(idmef, "alert.create_time > '2015-05-03 1:59:08'", 0, 0);
        test_criteria(idmef, "alert.create_time <= '2015-05-03 1:59:08'", 0, 1);
        test_criteria(idmef, "alert.create_time >= '2015-05-03 1:59:08'", 0, 1);

        test_criteria(idmef, "alert.create_time == '2015-05-03 1:59:07'", 0, 0);
        test_criteria(idmef, "alert.create_time != '2015-05-03 1:59:07'", 0, 1);
        test_criteria(idmef, "alert.create_time < '2015-05-03 1:59:07'", 0, 0);
        test_criteria(idmef, "alert.create_time > '2015-05-03 1:59:07'", 0, 1);
        test_criteria(idmef, "alert.create_time <= '2015-05-03 1:59:07'", 0, 0);
        test_criteria(idmef, "alert.create_time >= '2015-05-03 1:59:07'", 0, 1);

        test_criteria(idmef, "alert.create_time < '2015-05-03 1:59:09'", 0, 1);
        test_criteria(idmef, "alert.create_time > '2015-05-03 1:59:09'", 0, 0);
        test_criteria(idmef, "alert.create_time <= '2015-05-03 1:59:09'", 0, 1);
        test_criteria(idmef, "alert.create_time >= '2015-05-03 1:59:09'", 0, 0);

        /*
         * Broken down time check
         */
        assert(idmef_time_set_from_string(ctime, "2015-05-04 00:00:00+00:00") == 0);
        test_criteria(idmef, "alert.create_time == 'month:may mday:3'", 0, 0);
        test_criteria(idmef, "alert.create_time != 'month:may mday:3'", 0, 1);
        test_criteria(idmef, "alert.create_time < 'month:may mday:3'", 0, 0);
        test_criteria(idmef, "alert.create_time > 'month:may mday:3'", 0, 1);
        test_criteria(idmef, "alert.create_time <= 'month:may mday:3'", 0, 0);
        test_criteria(idmef, "alert.create_time >= 'month:may mday:3'", 0, 1);

        test_criteria(idmef, "alert.create_time == 'month:may mday:4'", 0, 1);
        test_criteria(idmef, "alert.create_time != 'month:may mday:4'", 0, 0);
        test_criteria(idmef, "alert.create_time < 'month:may mday:4'", 0, 0);
        test_criteria(idmef, "alert.create_time > 'month:may mday:4'", 0, 0);
        test_criteria(idmef, "alert.create_time <= 'month:may mday:4'", 0, 1);
        test_criteria(idmef, "alert.create_time >= 'month:may mday:4'", 0, 1);

        test_criteria(idmef, "alert.create_time == 'month:may mday:5'", 0, 0);
        test_criteria(idmef, "alert.create_time != 'month:may mday:5'", 0, 1);
        test_criteria(idmef, "alert.create_time < 'month:may mday:5'", 0, 1);
        test_criteria(idmef, "alert.create_time > 'month:may mday:5'", 0, 0);
        test_criteria(idmef, "alert.create_time <= 'month:may mday:5'", 0, 1);
        test_criteria(idmef, "alert.create_time >= 'month:may mday:5'", 0, 0);

        /*
         * Broken down time special wday/yday fields
         */
        test_criteria(idmef, "alert.create_time == 'wday:monday'", 0, 1);
        test_criteria(idmef, "alert.create_time != 'wday:monday'", 0, 0);
        test_criteria(idmef, "alert.create_time == 'wday:tuesday'", 0, 0);
        test_criteria(idmef, "alert.create_time != 'wday:tuesday'", 0, 1);

        test_criteria(idmef, "alert.create_time == 'wday:monday mday:3'", 0, 0);
        test_criteria(idmef, "alert.create_time != 'wday:monday mday:3'", 0, 1);
        test_criteria(idmef, "alert.create_time < 'wday:monday mday:3'", 0, 0);
        test_criteria(idmef, "alert.create_time > 'wday:monday mday:3'", 0, 1);
        test_criteria(idmef, "alert.create_time <= 'wday:monday mday:3'", 0, 0);
        test_criteria(idmef, "alert.create_time >= 'wday:monday mday:3'", 0, 1);

        test_criteria(idmef, "alert.create_time == 'wday:monday mday:4'", 0, 1);
        test_criteria(idmef, "alert.create_time != 'wday:monday mday:4'", 0, 0);
        test_criteria(idmef, "alert.create_time < 'wday:monday mday:4'", 0, 0);
        test_criteria(idmef, "alert.create_time > 'wday:monday mday:4'", 0, 0);
        test_criteria(idmef, "alert.create_time <= 'wday:monday mday:4'", 0, 1);
        test_criteria(idmef, "alert.create_time >= 'wday:monday mday:4'", 0, 1);

        test_criteria(idmef, "alert.create_time == 'wday:monday mday:5'", 0, 0);
        test_criteria(idmef, "alert.create_time != 'wday:monday mday:5'", 0, 1);
        test_criteria(idmef, "alert.create_time < 'wday:monday mday:5'", 0, 1);
        test_criteria(idmef, "alert.create_time > 'wday:monday mday:5'", 0, 0);
        test_criteria(idmef, "alert.create_time <= 'wday:monday mday:5'", 0, 1);
        test_criteria(idmef, "alert.create_time >= 'wday:monday mday:5'", 0, 0);

        /*
         * Test on listed object without specific index
         */
        test_criteria(idmef, "alert.analyzer(*).name == 'A'", 0, 1);
        test_criteria(idmef, "alert.analyzer(*).name != 'A'", 0, 0);
        test_criteria(idmef, "alert.analyzer(*).name == 'NOT EXIST'", 0, 0);
        test_criteria(idmef, "alert.analyzer(*).name != 'NOT EXIST'", 0, 1);
        test_criteria(idmef, "alert.analyzer(*).ostype == 'C'", 0, 1);
        test_criteria(idmef, "alert.analyzer(*).ostype != 'C'", 0, 0);
        test_criteria(idmef, "alert.analyzer(*).ostype == 'NOT EXIST'", 0, 0);
        test_criteria(idmef, "alert.analyzer(*).ostype != 'NOT EXIST'", 0, 1);

        test_criteria(idmef, "alert.analyzer(*).class", 0, 0);
        test_criteria(idmef, "! alert.analyzer(*).class", 0, 1);

        test_criteria(idmef, "alert.analyzer(*).name", 0, 1);
        test_criteria(idmef, "! alert.analyzer(*).name", 0, 0);

        test_criteria(idmef, "alert.analyzer(*).ostype", 0, 1);
        test_criteria(idmef, "! alert.analyzer(*).ostype", 0, 0);

        test_criteria(idmef, "alert.source(*).interface", 0, 0);
        test_criteria(idmef, "! alert.source(*).interface", 0, 1);

        test_criteria(idmef, "alert.source", 0, 0);
        test_criteria(idmef, "! alert.source", 0, 1);

        test_criteria(idmef, "alert.analyzer", 0, 1);
        test_criteria(idmef, "! alert.analyzer", 0, 0);

        test_criteria(idmef, "alert.analyzer(*).process.arg", 0, 1);
        test_criteria(idmef, "! alert.analyzer(*).process.arg", 0, 0);
        test_criteria(idmef, "alert.analyzer(*).process.arg == 'a0'", 0, 1);
        test_criteria(idmef, "alert.analyzer(*).process.arg != 'a0'", 0, 0);
        test_criteria(idmef, "alert.analyzer(*).process.arg == 'NOT EXIST'", 0, 0);
        test_criteria(idmef, "alert.analyzer(*).process.arg != 'NOT EXIST'", 0, 1);

        idmef_message_destroy(idmef);
        exit(0);
}
