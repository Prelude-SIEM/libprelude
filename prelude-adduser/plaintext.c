#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>

#include "prelude-inttypes.h"
#include "prelude-client.h"
#include "prelude-io.h"
#include "prelude-message.h"
#include "prelude-message-id.h"
#include "prelude-auth.h"

#include "plaintext.h"

static int read_plaintext_creation_result(prelude_io_t *fd) 
{
        int ret;
        void *buf;
        uint8_t tag;
        uint32_t dlen;
        prelude_msg_t *msg = NULL;

        ret = prelude_msg_read(&msg, fd);
        if ( ret <= 0 ) {
                fprintf(stderr, "error reading plaintext account creation result.\n");
                return -1;
        }

        ret = prelude_msg_get(msg, &tag, &dlen, &buf);
        prelude_msg_destroy(msg);
        
        if ( ret <= 0 ) {
                fprintf(stderr, "error reading plaintext creation result.\n");
                return -1;
        }
        
        if ( tag == PRELUDE_MSG_AUTH_SUCCEED ) {
                fprintf(stderr, "Plaintext account creation succeed with Prelude Manager.\n");
                return 0;
        }

        else if ( tag == PRELUDE_MSG_AUTH_EXIST )
                fprintf(stderr, "Plaintext account creation failed with Prelude Manager "
                        "(user already exist with different password).\n");
        
        else
                fprintf(stderr, "Plaintext account creation failed with Prelude Manager.\n");

        return -1;
}



static int setup_plaintext(prelude_io_t *fd, const char *oneshot,
                           const char *user, const char *pass) 
{
        ssize_t ret;
        prelude_msg_t *msg;
        size_t len, ulen, plen, olen;

        ulen = strlen(user) + 1;
        plen = strlen(pass) + 1;
        olen = strlen(oneshot) + 1;
        
        len = ulen + plen + olen;
        
        msg = prelude_msg_new(3, len, PRELUDE_MSG_AUTH, 0);
        if ( ! msg )
                return -1;
        
        prelude_msg_set(msg, PRELUDE_MSG_AUTH_PLAINTEXT, olen, oneshot);
        prelude_msg_set(msg, PRELUDE_MSG_AUTH_USERNAME, ulen, user);
        prelude_msg_set(msg, PRELUDE_MSG_AUTH_PASSWORD, plen, pass);
        
        ret = prelude_msg_write(msg, fd);
        prelude_msg_destroy(msg);

        return read_plaintext_creation_result(fd);
}




int create_plaintext_user_account(prelude_client_t *client, prelude_io_t *fd, const char *oneshot) 
{
        int ret;
        char buf[256];
        char *user, *pass;
                
        prelude_client_get_auth_filename(client, buf, sizeof(buf));

        /*
         * we don't want to keep an old user entry (each sensor have
         * only one user).
         */
        unlink(buf);
        
        ret = prelude_auth_create_account(buf, &user, &pass, 0,
                                          prelude_client_get_uid(client),
                                          prelude_client_get_gid(client));
        if ( ret < 0 )
                return -1;

        ret = setup_plaintext(fd, oneshot, user, pass);
        if ( ret < 0 )
                return -1;

        return 0;
}
