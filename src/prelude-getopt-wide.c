#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <netinet/in.h>

#include "prelude-log.h"
#include "extract.h"
#include "prelude-io.h"
#include "prelude-message.h"
#include "prelude-message-buffered.h"
#include "prelude-client.h"
#include "prelude-message-id.h"
#include "prelude-getopt.h"
#include "prelude-getopt-wide.h"
#include "common.h"




static prelude_msg_t *client_send_msg(prelude_msgbuf_t *msgbuf) 
{        
        prelude_client_send_msg(prelude_msgbuf_get_data(msgbuf), prelude_msgbuf_get_msg(msgbuf));
        return NULL;
}




static int send_option_reply(prelude_client_t *client, uint64_t admin_id,
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



static int read_option_request(prelude_client_t *client, prelude_msg_t *msg, uint64_t *admin_id,
                               uint32_t *request_id, int *type, const char **option, const char **value) 
{
        int ret;
        void *buf;
        uint8_t tag;
        uint32_t len;
        
        ret = prelude_msg_get(msg, &tag, &len, &buf);
        if ( ret <= 0 )
                return ret;

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
        
        return read_option_request(client, msg, admin_id, request_id, type, option, value);
}




static int handle_option_request(prelude_client_t *client, prelude_msg_t *msg)
{
        uint64_t admin_id;
        uint32_t request_id;
        int ret = 0, type = -1;
        const char *error = NULL;
        char getbuf[1024], *getptr = NULL;
        const char *option = NULL, *value = NULL;
        
        ret = read_option_request(client, msg, &admin_id, &request_id, &type, &option, &value);
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
        
        return send_option_reply(client, admin_id, request_id, getptr, error);
}



static int read_option_list(prelude_msg_t *msg, prelude_option_t *opt, uint64_t *source_id) 
{
        int ret;
        void *buf;
        uint32_t dlen;
        const char *tmp;
        uint8_t tag, tmpint;

        if ( ! opt )
                return -1;
        
        while ( (ret = prelude_msg_get(msg, &tag, &dlen, &buf)) > 0 ) {
                
                switch (tag) {
                
                case PRELUDE_MSG_OPTION_START:
                        read_option_list(msg, prelude_option_new(opt), source_id);
                        break;
                        
                case PRELUDE_MSG_OPTION_END:
                        return 0;
                        
                case PRELUDE_MSG_OPTION_SOURCE_ID:
                        ret = extract_uint64_safe(source_id, buf, dlen);
                        if ( ret < 0 )
                                return -1;
                        break;

                case PRELUDE_MSG_OPTION_NAME:
                        ret = extract_string_safe(&tmp, buf, dlen);
                        if ( ret < 0 )
                                return -1;

                        prelude_option_set_longopt(opt, strdup(tmp));
                        break;
                        
                case PRELUDE_MSG_OPTION_DESC:
                        ret = extract_string_safe(&tmp, buf, dlen);
                        if ( ret < 0 )
                                return -1;
                        
                        prelude_option_set_description(opt, strdup(tmp));
                        break;
                        
                case PRELUDE_MSG_OPTION_HELP:
                        ret = extract_string_safe(&tmp, buf, dlen);
                        if ( ret < 0 )
                                return -1;
                        
                        prelude_option_set_help(opt, strdup(tmp));
                        break;
                        
                case PRELUDE_MSG_OPTION_INPUT_VALIDATION:
                        ret = extract_string_safe(&tmp, buf, dlen);
                        if ( ret < 0 )
                                return -1;
                        
                        prelude_option_set_input_validation_regex(opt, strdup(tmp));
                        break;

                case PRELUDE_MSG_OPTION_HAS_ARG:
                        ret = extract_uint8_safe(&tmpint, buf, dlen);
                        if ( ret < 0 )
                                return -1;
                        
                        break;

                case PRELUDE_MSG_OPTION_INPUT_TYPE:
                        ret = extract_uint8_safe(&tmpint, buf, dlen);
                        if ( ret < 0 )
                                return -1;
                        
                        break;
                        
                default:
                        /*
                         * for compatibility purpose, don't return an error on unknow tag.
                         */
                        log(LOG_ERR, "unknow option tag %d.\n", tag);
                }
        }

        return 0;
}




int prelude_option_process_request(prelude_client_t *client, prelude_msg_t *msg)
{
        uint8_t tag;
        
        tag = prelude_msg_get_tag(msg);

        if ( tag != PRELUDE_MSG_OPTION_REQUEST ) {
                log(LOG_INFO, "[%s] - unknown option tag %d.\n", prelude_client_get_saddr(client), tag);
                return -1;
        }

        return handle_option_request(client, msg);
}





int prelude_option_send_request(prelude_client_t *client, uint32_t request_id, uint64_t target_id, int type, const char *option, const char *value)
{
        uint64_t tmp;
        prelude_msgbuf_t *msg;
        
        msg = prelude_msgbuf_new(0);
        if ( ! msg ) {
                log(LOG_ERR, "error creating option message.\n");
                return -1;
        }

        prelude_msgbuf_set_data(msg, client);
        prelude_msgbuf_set_callback(msg, client_send_msg);
        prelude_msg_set_tag(prelude_msgbuf_get_msg(msg), PRELUDE_MSG_OPTION_REQUEST);
        
        prelude_msgbuf_set(msg, type, 0, 0);

        tmp = prelude_hton64(prelude_client_get_analyzerid());
        prelude_msgbuf_set(msg, PRELUDE_MSG_OPTION_SOURCE_ID, sizeof(tmp), &tmp);

        tmp = prelude_hton64(target_id);
        prelude_msgbuf_set(msg, PRELUDE_MSG_OPTION_TARGET_ID, sizeof(tmp), &tmp);

        request_id = htonl(request_id);
        prelude_msgbuf_set(msg, PRELUDE_MSG_OPTION_ID, sizeof(request_id), &request_id);

        prelude_msgbuf_set(msg, PRELUDE_MSG_OPTION_NAME, strlen(option) + 1, option);

        if ( value )
                prelude_msgbuf_set(msg, PRELUDE_MSG_OPTION_VALUE, strlen(value) + 1, value);
        
        prelude_msgbuf_close(msg);
        
        return 0;
}




int prelude_option_recv_reply(prelude_msg_t *msg, uint32_t *request_id, const char **value, const char **error) 
{
        int ret;
        void *buf;
        uint8_t tag;
        uint32_t dlen;
        
        *value = *error = NULL;
        
        while ( (ret = prelude_msg_get(msg, &tag, &dlen, &buf)) ) {

                switch (tag) {

                case PRELUDE_MSG_OPTION_ID:
                        ret = extract_uint32_safe(request_id, buf, dlen);
                        if ( ret < 0 )
                                return -1;
                        break;

                case PRELUDE_MSG_OPTION_VALUE:
                        ret = extract_string_safe(value, buf, dlen);
                        if ( ret < 0 )
                                return -1;
                        break;

                case PRELUDE_MSG_OPTION_ERROR:
                        ret = extract_string_safe(error, buf, dlen);
                        if ( ret < 0 )
                                return -1;

                case PRELUDE_MSG_OPTION_TARGET_ID:
                        break;
                        
                default:
                        log(LOG_ERR, "unknow tag : %d.\n", tag);
                        return -1;
                }
        }

        if ( ret < 0 ) {
                log(LOG_ERR, "error receiving configuration reply.\n");
                return -1;
        }
        
        return 0;
}




prelude_option_t *prelude_option_read_option_list(prelude_msg_t *msg, uint64_t *source_id)
{
        int ret;
        prelude_option_t *opt;

        opt = prelude_option_new(NULL);
        if ( ! opt )
                return NULL;
                
        ret = read_option_list(msg, opt, source_id);
        if ( ret < 0 ) {
                prelude_option_destroy(opt);
                return NULL;
        }
        
        return opt;
} 


