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




static prelude_msg_t *send_msg(prelude_msgbuf_t *msgbuf) 
{
        prelude_io_t *fd = prelude_msgbuf_get_data(msgbuf);
        prelude_msg_t *msg = prelude_msgbuf_get_msg(msgbuf);
        
        prelude_msg_write(msg, fd);
        prelude_msg_recycle(msg);
        
        return msg;
}



static int read_option_request(prelude_msg_t *msg, uint64_t *admin_id, uint32_t *request_id,
                               int *type, const char **option, const char **value) 
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
        case PRELUDE_MSG_OPTION_LIST:
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
                ret = extract_characters_safe(option, buf, len);
                if (ret < 0 )
                        return -1;
                break;

        case PRELUDE_MSG_OPTION_VALUE:
                ret = extract_characters_safe(value, buf, len);
                if (ret < 0 )
                        return -1;
                break;

        default:
                return -1;
        }
        
        return read_option_request(msg, admin_id, request_id, type, option, value);
}



static int handle_set_request(void *context, const char *option, const char *value, char *out, size_t size) 
{
        int ret;
        
        ret = prelude_option_invoke_set(context, option, value);
        if ( ret < 0 )
                return -1;

        ret = prelude_option_invoke_get(context, option, out, size);
        if ( ret < 0 )
                return -1;
        
        return 0;
}



static int handle_option_request(prelude_client_t *client, prelude_io_t *fd, prelude_msg_t *msg)
{
        char out[1024];
        uint64_t admin_id;
        uint32_t request_id;
        int ret = 0, type = -1;
        prelude_msgbuf_t *msgbuf;
        uint64_t analyzerid = prelude_client_get_analyzerid(client);
        const char *option = NULL, *value = NULL, error[] = "remote option error";
        
        ret = read_option_request(msg, &admin_id, &request_id, &type, &option, &value);        
        if ( ret < 0 )
                return -1;
        
        msgbuf = prelude_msgbuf_new(client);
        if ( ! msgbuf ) 
                return -1;

        prelude_msgbuf_set_data(msgbuf, fd);
        prelude_msgbuf_set_callback(msgbuf, send_msg);
        
        prelude_msg_set_tag(prelude_msgbuf_get_msg(msgbuf), PRELUDE_MSG_OPTION_REPLY);
        
        prelude_msgbuf_set(msgbuf, PRELUDE_MSG_OPTION_ID, sizeof(request_id), &request_id);
        prelude_msgbuf_set(msgbuf, PRELUDE_MSG_OPTION_SOURCE_ID, sizeof(analyzerid), &analyzerid);
        prelude_msgbuf_set(msgbuf, PRELUDE_MSG_OPTION_TARGET_ID, sizeof(admin_id), &admin_id);
        
	if ( type == PRELUDE_MSG_OPTION_SET ) 
                ret = handle_set_request(client, option, value, out, sizeof(out));
        
	else if ( type == PRELUDE_MSG_OPTION_GET && option )
                ret = prelude_option_invoke_get(client, option, out, sizeof(out));

        else ret = prelude_option_wide_send_msg(client, msgbuf);

        if ( ret < 0 )
                prelude_msgbuf_set(msgbuf, PRELUDE_MSG_OPTION_ERROR, sizeof(error), error);
        
        else if ( option )
                prelude_msgbuf_set(msgbuf, PRELUDE_MSG_OPTION_VALUE, strlen(out) + 1, out);
        
        prelude_msgbuf_close(msgbuf);

        return 0;
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
                        
                case PRELUDE_MSG_OPTION_VALUE:
                        ret = extract_characters_safe(&tmp, buf, dlen);
                        if ( ret < 0 )
                                return -1;

                        prelude_option_set_value(opt, strdup(tmp));
                        break;

                case PRELUDE_MSG_OPTION_NAME:
                        ret = extract_characters_safe(&tmp, buf, dlen);
                        if ( ret < 0 )
                                return -1;

                        prelude_option_set_longopt(opt, strdup(tmp));
                        break;
                        
                case PRELUDE_MSG_OPTION_DESC:
                        ret = extract_characters_safe(&tmp, buf, dlen);
                        if ( ret < 0 )
                                return -1;
                        
                        prelude_option_set_description(opt, strdup(tmp));
                        break;
                        
                case PRELUDE_MSG_OPTION_HELP:
                        ret = extract_characters_safe(&tmp, buf, dlen);
                        if ( ret < 0 )
                                return -1;
                        
                        prelude_option_set_help(opt, strdup(tmp));
                        break;
                        
                case PRELUDE_MSG_OPTION_INPUT_VALIDATION:
                        ret = extract_characters_safe(&tmp, buf, dlen);
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




int prelude_option_process_request(prelude_client_t *client, prelude_io_t *fd, prelude_msg_t *msg)
{
        uint8_t tag;
        
        tag = prelude_msg_get_tag(msg);
        
        if ( tag != PRELUDE_MSG_OPTION_REQUEST )
                return -1;
        
        return handle_option_request(client, fd, msg);
}




int prelude_option_send_request(prelude_client_t *client, uint32_t request_id,
                                uint64_t target_id, int type, const char *option, const char *value)
{
        uint64_t tmp;
        prelude_msgbuf_t *msg;
        
        msg = prelude_msgbuf_new(client);
        if ( ! msg ) {
                log(LOG_ERR, "error creating option message.\n");
                return -1;
        }
        
        prelude_msg_set_tag(prelude_msgbuf_get_msg(msg), PRELUDE_MSG_OPTION_REQUEST);        
        prelude_msgbuf_set(msg, type, 0, 0);

        tmp = prelude_hton64(prelude_client_get_analyzerid(client));
        prelude_msgbuf_set(msg, PRELUDE_MSG_OPTION_SOURCE_ID, sizeof(tmp), &tmp);

        tmp = prelude_hton64(target_id);
        prelude_msgbuf_set(msg, PRELUDE_MSG_OPTION_TARGET_ID, sizeof(tmp), &tmp);

        request_id = htonl(request_id);
        prelude_msgbuf_set(msg, PRELUDE_MSG_OPTION_ID, sizeof(request_id), &request_id);

        if ( option ) {
                prelude_msgbuf_set(msg, PRELUDE_MSG_OPTION_NAME, strlen(option) + 1, option);
                
                if ( value )
                        prelude_msgbuf_set(msg, PRELUDE_MSG_OPTION_VALUE, strlen(value) + 1, value);
        }
        
        prelude_msgbuf_close(msg);
        
        return 0;
}



int prelude_option_recv_reply(prelude_msg_t *msg, uint64_t *source_id, uint32_t *request_id, void **value)
{
        void *buf;
        uint8_t tag;
        uint32_t dlen;
        int ret, type = -1;
        
        *value = NULL;

        while ( (ret = prelude_msg_get(msg, &tag, &dlen, &buf)) ) {

                switch (tag) {

                case PRELUDE_MSG_OPTION_ID:
                        type = PRELUDE_OPTION_REPLY_TYPE_SET;
                        
                        ret = extract_uint32_safe(request_id, buf, dlen);
                        if ( ret < 0 )
                                return -1;
                        break;
                        
                case PRELUDE_MSG_OPTION_VALUE:
                        type = PRELUDE_OPTION_REPLY_TYPE_GET;
                        
                        ret = extract_characters_safe((const char **) value, buf, dlen);                        
                        if ( ret < 0 )
                                return -1;
                        break;

                case PRELUDE_MSG_OPTION_ERROR:
                        type = PRELUDE_OPTION_REPLY_TYPE_ERROR;

                        ret = extract_characters_safe((const char **) value, buf, dlen);
                        if ( ret < 0 )
                                return -1;
                        break;
                        
                case PRELUDE_MSG_OPTION_SOURCE_ID:
                        ret = extract_uint64_safe(source_id, buf, dlen);
                        if ( ret < 0 )
                                return -1;
                        break;
                        
                case PRELUDE_MSG_OPTION_TARGET_ID:
                        break;

                case PRELUDE_MSG_OPTION_LIST:
                        type = PRELUDE_OPTION_REPLY_TYPE_LIST;
                        *value = prelude_option_new(NULL);
                        
                        ret = read_option_list(msg, *value, NULL);
                        if ( ret < 0 )
                                return -1;
                        break;
                        
                default:
                        log(LOG_ERR, "unknow tag : %d.\n", tag);
                        return -1;
                }
        }
        
        return type;
}
