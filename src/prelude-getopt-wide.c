#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <netinet/in.h>

#include "prelude-log.h"
#include "extract.h"
#include "prelude-io.h"
#include "prelude-message.h"
#include "prelude-client.h"
#include "prelude-message-id.h"
#include "prelude-getopt.h"
#include "prelude-getopt-wide.h"



static int reply_option_request(prelude_client_t *client, uint64_t admin_id,
                                uint32_t request_id, const char *getdata, const char *error) 
{
        int msgnum, msglen;
        prelude_msg_t *msg;
        int errorlen = 0, getdatalen = 0;
        
        msgnum = 2; /* id + target */
        msglen = sizeof(request_id) + sizeof(admin_id);
	
        if ( getdata ) {
                msgnum++;
                getdatalen = strlen(getdata) + 1;
                msglen += getdatalen;
        }

        if ( error ) {
		msgnum++;
                errorlen = strlen(error) + 1;
		msglen += errorlen;
	}
         
	msg = prelude_msg_new(msgnum, msglen, PRELUDE_MSG_OPTION_REPLY, 0);
	if ( ! msg ) {
		log(LOG_ERR, "error creating reply message.\n");
		return -1;
	}
	
	prelude_msg_set(msg, PRELUDE_MSG_OPTION_ID, sizeof(request_id), &request_id);
	prelude_msg_set(msg, PRELUDE_MSG_OPTION_TARGET_ID, sizeof(admin_id), &admin_id);
	
	if ( getdata )
		prelude_msg_set(msg, PRELUDE_MSG_OPTION_VALUE, getdatalen, getdata);

        if ( error )
		prelude_msg_set(msg, PRELUDE_MSG_OPTION_ERROR, errorlen, error);

	return prelude_msg_write(msg, prelude_client_get_fd(client));
}



static int get_option_request(prelude_client_t *client, prelude_msg_t *msg, uint64_t *admin_id,
                              uint32_t *request_id, int *type, const char **option, const char **value) 
{
        int ret;
        void *buf;
        uint8_t tag;
        uint32_t len;
        
        ret = prelude_msg_get(msg, &tag, &len, &buf);
        if ( ret == 0 )
                return 0;

        switch (tag) {
                
        case PRELUDE_MSG_OPTION_SET:
        case PRELUDE_MSG_OPTION_GET:
                *type = tag;
                break;
                
        case PRELUDE_MSG_OPTION_TARGET_ID:
                break;
                        
        case PRELUDE_MSG_OPTION_SOURCE_ID:
                if ( len != sizeof(uint64_t) )
                        return -1;
                
                *admin_id = align_uint64(buf);
                break;
                
        case PRELUDE_MSG_OPTION_ID:
                if ( len != sizeof(uint32_t) )
                        return -1;
                
                *request_id = align_uint32(buf);
                break;
                
        case PRELUDE_MSG_OPTION_NAME:
                ret = extract_string_safe(option, buf, len);
                if (ret < 0 )
                        return -1;
                break;

        case PRELUDE_MSG_OPTION_VALUE:
                ret = extract_string_safe(value, buf, len);
                if (ret < 0 )
                        return -1;
                break;

        default:
                log(LOG_INFO, "[%s] - unknown option tag %d.\n", prelude_client_get_saddr(client), tag);
        }
        
        return get_option_request(client, msg, admin_id, request_id, type, option, value);
}




static int handle_option_request(prelude_client_t *client, prelude_msg_t *msg)
{
        uint64_t admin_id;
        uint32_t request_id;
        int ret = 0, type = -1;
        const char *error = NULL;
        char getbuf[1024], *getptr = NULL;
        const char *option = NULL, *value = NULL;
        
        ret = get_option_request(client, msg, &admin_id, &request_id, &type, &option, &value);
        if ( ret < 0 )
                return -1;
        
	if ( type == PRELUDE_MSG_OPTION_SET ) 
		ret = prelude_option_invoke_set(option, value);
        
	else if ( type == PRELUDE_MSG_OPTION_GET ) {
                getptr = getbuf;
		ret = prelude_option_invoke_get(option, getbuf, sizeof(getbuf));
        }

        else
                error = "Invalid option type";

        if ( ret < 0 )
                error = "Error setting option";
        
        return reply_option_request(client, admin_id, request_id, getptr, error);
}




int prelude_option_wide_process_request(prelude_client_t *client)
{
        uint8_t tag;
        int ret = -1;
        prelude_msg_t *msg = NULL;
        prelude_msg_status_t status;

	status = prelude_msg_read(&msg, prelude_client_get_fd(client));
        if ( status != prelude_msg_finished )
		return -1;

        tag = prelude_msg_get_tag(msg);
	switch ( tag ) {
                
	case PRELUDE_MSG_OPTION_REQUEST:
		ret = handle_option_request(client, msg);
		break;

        default:
		log(LOG_INFO, "[%s] - unknown option tag %d.\n", prelude_client_get_saddr(client), tag);

        }

        prelude_msg_destroy(msg);

        return ret;
}







