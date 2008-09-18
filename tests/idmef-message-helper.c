#include "config.h"

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "prelude.h"

#define FLOAT_TOLERANCE 0.0001

#ifndef ABS
# define ABS(x) (((x) < 0) ? -(x) : (x))
#endif

#ifndef MAX
# define MAX(x, y) ((x) > (y) ? (x) : (y))
#endif


static double reldif(double a, double b)
{
        double c = ABS(a);
        double d = ABS(b);

        d = MAX(c, d);

        return d == 0.0 ? 0.0 : ABS(a - b) / d;
}


int main(void)
{
        char *res;
        double resd;
        idmef_message_t *idmef;

        assert(idmef_message_new(&idmef) == 0);

        assert(idmef_message_set_string(idmef, "alert.classification.text", "Random value") == 0);
        assert(idmef_message_get_string(idmef, "alert.classification.text", &res) == 0);
        assert(strcmp("Random value", res) == 0);
        free(res);

        assert(idmef_message_set_string(idmef, "alert.assessment.impact.severity", "high") == 0);
        assert(idmef_message_get_string(idmef, "alert.assessment.impact.severity", &res) == 0);
        assert(strcmp("high", res) == 0);
        free(res);

        assert(idmef_message_set_number(idmef, "alert.assessment.confidence.confidence", 10.2) == 0);
        assert(idmef_message_get_number(idmef, "alert.assessment.confidence.confidence", &resd) == 0);
        assert(reldif(resd, 10.2) <= FLOAT_TOLERANCE);

        assert(idmef_message_set_string(idmef, "alert.assessment.impact.severity", "Random value") < 0);

        exit(0);
}
