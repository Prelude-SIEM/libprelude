#include "config.h"

#include <stdlib.h>
#include <assert.h>
#include "prelude.h"

#define TEST_STR "abcdefghijklmnopqrstuvwxyz"
#define MAX_LAG_SEC 3


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
        assert(now - idmef_time_get_sec(ctime) < MAX_LAG_SEC);

        exit(0);
}
