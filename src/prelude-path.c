#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>

#include "prelude-path.h"


/*
 * directory where backup are made (in case Manager are down).
 */
#define BACKUP_DIR "/var/spool/prelude-sensors"

/*
 * directory where plaintext authentication file are stored.
 */
#define AUTH_DIR "/var/lib/prelude-sensors/plaintext"

/*
 * directory where SSL authentication file are stored.
 */
#define SSL_DIR "/var/lib/prelude-sensors/ssl"

/*
 * Path to the Prelude Unix socket.
 */
#define UNIX_SOCKET "/var/lib/socket"




static const char *sensorname = NULL;



void prelude_get_auth_filename(char *buf, size_t size) 
{
        snprintf(buf, size, "%s/%s.%d", AUTH_DIR, sensorname, getuid());
}


void prelude_get_ssl_cert_filename(char *buf, size_t size) 
{
        snprintf(buf, size, "%s/%s-cert.%d", SSL_DIR, sensorname, getuid());
}


void prelude_get_ssl_key_filename(char *buf, size_t size) 
{
        snprintf(buf, size, "%s/%s-key.%d", SSL_DIR, sensorname, getuid());
}


void prelude_get_backup_filename(char *buf, size_t size) 
{
        snprintf(buf, size, BACKUP_DIR"/backup.%d", getuid());
}


char *prelude_get_socket_filename(void) 
{
        return UNIX_SOCKET;
}



void prelude_set_program_name(const char *sname) 
{
        sensorname = sname;
}
