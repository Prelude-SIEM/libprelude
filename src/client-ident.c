#include <stdio.h>
#include <unistd.h>
#include <inttypes.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

#include "common.h"
#include "extract.h"
#include "prelude-io.h"
#include "prelude-message.h"
#include "prelude-message-id.h"
#include "client-ident.h"

#define IDENTITY_DIR "/var/lib/prelude-sensors/idents"

static char identfile[1024];
static uint64_t sensor_ident = 0;
static const char *sensor_name = NULL;


static int generate_filename(const char *sname) 
{
        return snprintf(identfile, sizeof(identfile), "%s/%s.ident", IDENTITY_DIR, sname);
}



static int save_ident(void) 
{
        int fd;
        FILE *fdp;
        
        fd = open(identfile, O_CREAT|O_WRONLY|O_EXCL, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
        if ( fd < 0 ) {
                log(LOG_ERR, "error opening %s for writing.\n", identfile);
                return -1;
        }

        fdp = fdopen(fd, "w");
        if ( ! fdp ) {
                log(LOG_ERR, "error associating fd with file object.\n", identfile);
                return -1;
        }
        
        fprintf(fdp, "%llu\n", sensor_ident);
        fclose(fdp);

        return 0;
}



static int recv_ident(prelude_io_t *pio) 
{
        int ret;
        void *buf;
        uint8_t tag;
        uint32_t len;
        prelude_msg_t *msg = NULL;
        prelude_msg_status_t status;

        /*
         * read until we get the whole message. Or an error occur.
         */
        do {
                status = prelude_msg_read(&msg, pio);
        } while ( status == prelude_msg_unfinished );

        /*
         * handle possible error.
         */
        if ( status != prelude_msg_finished ) {
                log(LOG_ERR, "error reading ident response.\n");
                return -1;
        }
        
        ret = prelude_msg_get(msg, &tag, &len, &buf);
        if ( ret < 0 )
                return -1;
        
        ret = extract_uint64(&sensor_ident, buf, len);
        if ( ret < 0 ) 
                return -1;

        log(LOG_INFO, "- Manager server allocated id %llu.\n", sensor_ident);

        return 0;
}




static int request_ident_from_manager(prelude_io_t *fd) 
{
        int ret;
        prelude_msg_t *msg;

        msg = prelude_msg_new(1, 0, PRELUDE_MSG_ID, 0);
        if ( ! msg )
                return -1;

        prelude_msg_set(msg, PRELUDE_MSG_ID_REQUEST, 0, NULL);
        
        ret = prelude_msg_write(msg, fd);
        if ( ret < 0 )
                return -1;

        ret = recv_ident(fd);
        if ( ret < 0 )
                return -1;

        ret = save_ident();
        if ( ret < 0 )
                return -1;
        
        return 0;
}




static int declare_ident_to_manager(prelude_io_t *fd) 
{
        int ret;
        uint64_t nident;
        prelude_msg_t *msg;
        
        msg = prelude_msg_new(1, sizeof(uint64_t), PRELUDE_MSG_ID, 0);
        if ( ! msg )
                return -1;
        
        /*
         * Put in network byte order
         */
        ((uint32_t *) &nident)[0] = htonl(((uint32_t *) &sensor_ident)[1]);
        ((uint32_t *) &nident)[1] = htonl(((uint32_t *) &sensor_ident)[0]);

        /*
         * send message
         */
        prelude_msg_set(msg, PRELUDE_MSG_ID_DECLARE, sizeof(uint64_t), &nident);
        ret = prelude_msg_write(msg, fd);
        prelude_msg_destroy(msg);

        return ret;
}





int prelude_client_ident_send(prelude_io_t *fd) 
{
        /*
         * we are not a sensor, probably a relay manager.
         * don't set any identity.
         */
        if ( ! sensor_name )
                return 0;

        if ( sensor_ident != 0 )
                return declare_ident_to_manager(fd);
        else
                return request_ident_from_manager(fd);
}




int prelude_client_ident_init(const char *sname) 
{
        int ret;
        FILE *fd;
        
        sensor_name = sname;
        
        ret = generate_filename(sname);
        if ( ret < 0 ) {
                log(LOG_ERR, "error formating filename.\n");
                return -1;
        }

        ret = access(identfile, F_OK|R_OK);
        if ( ret < 0 ) {
                if ( errno == ENOENT )
                        return 0;
                
                log(LOG_ERR, "can't access %s.\n", identfile);
                return -1;
        }

        fd = fopen(identfile, "r");
        if ( ! fd ) {
                log(LOG_ERR, "error opening %s for reading.\n", identfile);
                return -1;
        }

        /*
         * scan the 64 bits sensor ident.
         */
        fscanf(fd, "%llu", &sensor_ident);
        fclose(fd);

        return 0;
}





