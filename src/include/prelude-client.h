typedef struct prelude_client prelude_client_t;

void prelude_client_destroy(prelude_client_t *client);

prelude_client_t *prelude_client_new(const char *addr, uint16_t port);

int prelude_client_send_msg(prelude_client_t *client, prelude_msg_t *msg);

