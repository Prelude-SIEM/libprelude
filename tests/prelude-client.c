#include "config.h"

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "prelude.h"


int main(int argc, char **argv)
{
        int ret;
        prelude_client_t *client;

        assert(prelude_init(&argc, argv) == 0);
        assert(prelude_client_new(&client, "Client that does not exist") == 0);
        assert((ret = prelude_client_start(client)) < 0);
        assert(prelude_error_get_code(ret) == PRELUDE_ERROR_PROFILE);

        prelude_client_destroy(client, PRELUDE_CLIENT_EXIT_STATUS_FAILURE);
        prelude_deinit();

        return 0;
}
