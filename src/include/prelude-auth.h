#define SENSORS_AUTH_FILE CONFIG_DIR"/prelude-sensors.auth"
#define MANAGER_AUTH_FILE CONFIG_DIR"/prelude-manager.auth"


int prelude_auth_create_account(const char *filename, const int crypted);

int prelude_auth_send(int sock, const char *addr);

int prelude_auth_recv(int sock, const char *addr);
