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



static int parse_request(prelude_client_t *client, int rtype, char *request, char *out, size_t size)
{
        int ret;
        void *context = client;
        char *str, *ptr, *value;
        char pname[256], iname[256];
        prelude_option_t *last = NULL;

        value = request;
        prelude_strsep(&value, "=");
                        
        ptr = NULL;

        while ( (str = (prelude_strsep(&request, "."))) ) {                
                                    
                ret = sscanf(str, "%255[^[][%255[^]]", pname, iname);
                if ( ret <= 0 ) {
                        snprintf(out, size, "error parsing option path");
                        break;
                }
                
                if ( str + strlen(str) + 1 == value ) 
                        ptr = value;
                                
                if ( rtype == PRELUDE_MSG_OPTION_SET )
                        ret = prelude_option_invoke_set(&context, &last, pname, (ret == 2) ? iname : ptr, out, size);
                else 
                        ret = prelude_option_invoke_get(&context, &last, pname, (ret == 2) ? iname : ptr, out, size);
                
                if ( ret < 0 )
                        return -1;
        }
                
        return 0;
}



static prelude_msg_t *send_msg(prelude_msgbuf_t *msgbuf) 
{
        prelude_io_t *fd = prelude_msgbuf_get_data(msgbuf);
        prelude_msg_t *msg = prelude_msgbuf_get_msg(msgbuf);
        
        prelude_msg_write(msg, fd);
        prelude_msg_recycle(msg);
        
        return msg;
}




static int read_option_request(prelude_client_t *client, prelude_msgbuf_t *msgbuf, prelude_msg_t *msg)
{
        int ret, type = -1;
        void *buf;
        uint8_t tag;
        uint32_t len;
        char out[1024] = { 0 }, *request;
        uint32_t request_id;
        uint64_t source_id;

        while ( prelude_msg_get(msg, &tag, &len, &buf) > 0 ) {

                switch (tag) {
                case PRELUDE_MSG_OPTION_LIST:
                        return prelude_option_wide_send_msg(client, msgbuf); 
                        
                case PRELUDE_MSG_OPTION_SET:
                case PRELUDE_MSG_OPTION_GET:
                        type = tag;
                        break;
                
                case PRELUDE_MSG_OPTION_TARGET_ID:
                        break;
                        
                case PRELUDE_MSG_OPTION_SOURCE_ID:
                        ret = extract_uint64_safe(&source_id, buf, len);
                        if ( ret < 0 )
                                return -1;
                        
                        prelude_msgbuf_set(msgbuf, PRELUDE_MSG_OPTION_TARGET_ID, len, buf);
                        break;
                        
                case PRELUDE_MSG_OPTION_ID:
                        ret = extract_uint32_safe(&request_id, buf, len);
                        if ( ret < 0 )
                                return -1;
                        
                        prelude_msgbuf_set(msgbuf, PRELUDE_MSG_OPTION_ID, len, buf);
                        break;
                        
                case PRELUDE_MSG_OPTION_VALUE:
                        ret = extract_characters_safe((const char **) &request, buf, len);
                        if (ret < 0 )
                                return -1;
                                                
                        if ( type < 0 ) {
                                snprintf(out, sizeof(out), "no request type specified.\n");
                                prelude_msgbuf_set(msgbuf, PRELUDE_MSG_OPTION_ERROR, strlen(out) + 1, out);
                                return -1;
                        }
                        
                        ret = parse_request(client, type, request, out, sizeof(out));                        
                        if ( ret < 0 ) 
                                prelude_msgbuf_set(msgbuf, PRELUDE_MSG_OPTION_ERROR, strlen(out) + 1, out);

                        else if ( *out )
                                prelude_msgbuf_set(msgbuf, PRELUDE_MSG_OPTION_VALUE, strlen(out) + 1, out);
                        break;
                        
                default:
                        snprintf(out, sizeof(out), "unknown option tag: %d.\n", tag);
                        prelude_msgbuf_set(msgbuf, PRELUDE_MSG_OPTION_ERROR, strlen(out) + 1, out);
                        return -1;
                }
        }
        
        return 0;
}



static int handle_option_request(prelude_client_t *client, prelude_io_t *fd, prelude_msg_t *msg)
{
        int ret = 0;
        prelude_msgbuf_t *msgbuf;
        uint64_t analyzerid = prelude_client_get_analyzerid(client);
            
        msgbuf = prelude_msgbuf_new(client);
        if ( ! msgbuf ) 
                return -1;
        
        prelude_msgbuf_set_data(msgbuf, fd);
        prelude_msgbuf_set_callback(msgbuf, send_msg);
        prelude_msg_set_tag(prelude_msgbuf_get_msg(msgbuf), PRELUDE_MSG_OPTION_REPLY);

        prelude_msgbuf_set(msgbuf, PRELUDE_MSG_OPTION_SOURCE_ID, sizeof(analyzerid), &analyzerid);
        
        ret = read_option_request(client, msgbuf, msg);        
        prelude_msgbuf_close(msgbuf);

        return ret;
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
                        
                        prelude_option_set_value(opt, tmp);
                        break;

                case PRELUDE_MSG_OPTION_NAME:
                        ret = extract_characters_safe(&tmp, buf, dlen);
                        if ( ret < 0 )
                                return -1;
                        
                        prelude_option_set_longopt(opt, tmp);
                        break;
                        
                case PRELUDE_MSG_OPTION_DESC:
                        ret = extract_characters_safe(&tmp, buf, dlen);
                        if ( ret < 0 )
                                return -1;

                        prelude_option_set_description(opt, tmp);
                        break;
                        
                case PRELUDE_MSG_OPTION_HELP:
                        ret = extract_characters_safe(&tmp, buf, dlen);
                        if ( ret < 0 )
                                return -1;
                        
                        prelude_option_set_help(opt, tmp);
                        break;
                        
                case PRELUDE_MSG_OPTION_INPUT_VALIDATION:
                        ret = extract_characters_safe(&tmp, buf, dlen);
                        if ( ret < 0 )
                                return -1;
                        
                        prelude_option_set_input_validation_regex(opt, tmp);
                        break;

                case PRELUDE_MSG_OPTION_HAS_ARG:
                        ret = extract_uint8_safe(&tmpint, buf, dlen);
                        if ( ret < 0 )
                                return -1;
                        
                        prelude_option_set_has_arg(opt, tmpint);
                        break;

                case PRELUDE_MSG_OPTION_FLAGS:
                        ret = extract_uint8_safe(&tmpint, buf, dlen);
                        if ( ret < 0 )
                                return -1;

                        prelude_option_set_flags(opt, tmpint);
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




int prelude_option_push_request(prelude_msgbuf_t *msg, int type, const char *request)
{        
        prelude_msgbuf_set(msg, type, 0, 0);
        
        if ( request ) 
                prelude_msgbuf_set(msg, PRELUDE_MSG_OPTION_VALUE, strlen(request) + 1, request);
                
        return 0;
}



int prelude_option_new_request(prelude_client_t *client, prelude_msgbuf_t *msgbuf, uint32_t request_id, uint64_t target_id)
{
        uint64_t tmp;
        
        prelude_msg_set_tag(prelude_msgbuf_get_msg(msgbuf), PRELUDE_MSG_OPTION_REQUEST);

        tmp = prelude_hton64(prelude_client_get_analyzerid(client));
        prelude_msgbuf_set(msgbuf, PRELUDE_MSG_OPTION_SOURCE_ID, sizeof(tmp), &tmp);

        tmp = prelude_hton64(target_id);
        prelude_msgbuf_set(msgbuf, PRELUDE_MSG_OPTION_TARGET_ID, sizeof(tmp), &tmp);

        request_id = htonl(request_id);
        prelude_msgbuf_set(msgbuf, PRELUDE_MSG_OPTION_ID, sizeof(request_id), &request_id);
                
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
