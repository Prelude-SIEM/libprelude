#include <stdlib.h>
#include <assert.h>
#include "prelude.h"

#define TEST_STR "abcdefghijklmnopqrstuvwxyz"


int main(void)
{
        time_t now;
        idmef_time_t *ctime;
        idmef_alert_t *alert;
        idmef_message_t *idmef;

        assert(idmef_message_new(&idmef) == 0);
        assert(idmef_message_new_alert(idmef, &alert) == 0);

        ctime = idmef_alert_get_create_time(alert);
        assert(ctime != NULL);

        now = time(NULL);
        assert(idmef_time_get_sec(ctime) == now);

        exit(0);
}
