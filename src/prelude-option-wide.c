/*****
*
* Copyright (C) 2004 Yoann Vandoorselaere <yoann@prelude-ids.org>
* All Rights Reserved
*
* This file is part of the Prelude program.
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2, or (at your option)
* any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; see the file COPYING.  If not, write to
* the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
*
*****/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <netinet/in.h>

#include "libmissing.h"
#include "prelude-error.h"
#include "prelude-log.h"
#include "prelude-extract.h"
#include "prelude-io.h"
#include "prelude-msgbuf.h"
#include "prelude-client.h"
#include "prelude-message-id.h"
#include "prelude-option.h"
#include "prelude-option-wide.h"
#include "common.h"
#include "config-engine.h"



static int config_save_value(config_t *cfg, int rtype, prelude_option_t *last,
                             char **prev, const char *option, const char *value)
{
        int ret;
        char buf[1024];
        
        if ( ! (prelude_option_get_type(last) & PRELUDE_OPTION_TYPE_CFG) )
                return -1;
        
        if ( rtype != PRELUDE_MSG_OPTION_SET && rtype != PRELUDE_MSG_OPTION_DESTROY )
                return -1;
        
        if ( prelude_option_has_optlist(last) ) {
                
                snprintf(buf, sizeof(buf), "%s=%s", option, (value) ? value : "default");
                
                if ( *prev )
                        free(prev);
                
                *prev = strdup(buf);

                if ( rtype == PRELUDE_MSG_OPTION_SET )
                        return config_set(cfg, buf, NULL, NULL);
                else
                        return config_del(cfg, buf, NULL);
                        
        }

        if ( rtype == PRELUDE_MSG_OPTION_SET )
                ret = config_set(cfg, *prev, option, value);
        else
                ret = config_del(cfg, *prev, option);
        
        return ret;
}



static int parse_single(void **context, prelude_option_t **last, int is_last_cmd,
                        int rtype, const char *option, const char *value, char *out, size_t size)
{
        int ret = 0;
	
        *last = prelude_option_search(*last, option, PRELUDE_OPTION_TYPE_WIDE, 0);
        if ( ! *last ) {
                snprintf(out, size, "Unknown option: %s.\n", option);
                return -1;
        }
        
        if ( rtype == PRELUDE_MSG_OPTION_SET )
                ret = prelude_option_invoke_set(context, *last, value, out, size);
        
        else if ( is_last_cmd ) {
		
                if ( rtype == PRELUDE_MSG_OPTION_DESTROY ) 
                        ret = prelude_option_invoke_destroy(*context, *last, value, out, size);
                
                else if ( rtype == PRELUDE_MSG_OPTION_GET )
                        ret = prelude_option_invoke_get(*context, *last, value, out, size);
        
		else if ( rtype == PRELUDE_MSG_OPTION_COMMIT )
			ret = prelude_option_invoke_commit(*context, *last, value, out, size);
	}
	
        return ret;
}




static int parse_request(prelude_client_t *client, int rtype, char *request, char *out, size_t size)
{
        config_t *cfg;
        char pname[256], iname[256];
        int ret = 0, last_cmd = 0, ent;
        prelude_option_t *last = NULL;
        char *str, *value, *prev = NULL, *ptr = NULL;
        void *context = client;
	
        cfg = config_open(prelude_client_get_config_filename(client));
        if ( ! cfg ) {
                log(LOG_ERR, "error opening %s.\n", prelude_client_get_config_filename(client));
                return -1;
        }
        
        value = request;
        strsep(&value, "=");
        
        while ( (str = (strsep(&request, "."))) ) {
                
                if ( ! request ) {
                        ptr = value;
                        last_cmd = 1;
                }

                ent = sscanf(str, "%255[^[][%255[^]]", pname, iname);
                if ( ent <= 0 ) {
                        snprintf(out, size, "error parsing option path");
                        return -1;
                }

                ret = parse_single(&context, &last, last_cmd, rtype, pname, (ent == 2) ? iname : ptr, out, size);
                if ( ret < 0 )
                        break;
                
                config_save_value(cfg, rtype, last, &prev, pname, (ent == 2) ? iname : ptr);
        }
        
        config_close(cfg);
        free(prev);
        
        return ret;
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
                case PRELUDE_MSG_OPTION_COMMIT:
                case PRELUDE_MSG_OPTION_DESTROY:
                        type = tag;
                        break;
                
                case PRELUDE_MSG_OPTION_TARGET_ID:
                        break;
                        
                case PRELUDE_MSG_OPTION_SOURCE_ID:
                        ret = prelude_extract_uint64_safe(&source_id, buf, len);
                        if ( ret < 0 )
                                return ret;
                        
                        prelude_msgbuf_set(msgbuf, PRELUDE_MSG_OPTION_TARGET_ID, len, buf);
                        break;
                        
                case PRELUDE_MSG_OPTION_ID:
                        ret = prelude_extract_uint32_safe(&request_id, buf, len);
                        if ( ret < 0 )
                                return ret;
                        
                        prelude_msgbuf_set(msgbuf, PRELUDE_MSG_OPTION_ID, len, buf);
                        break;
                        
                case PRELUDE_MSG_OPTION_VALUE:
                        ret = prelude_extract_characters_safe((const char **) &request, buf, len);
                        if (ret < 0 )
                                return ret;
                                                
                        if ( type < 0 ) {
                                snprintf(out, sizeof(out), "no request type specified.\n");
                                prelude_msgbuf_set(msgbuf, PRELUDE_MSG_OPTION_ERROR, strlen(out) + 1, out);
                                return -1;
                        }
                        
                        ret = parse_request(client, type, request, out, sizeof(out));                                                
                        if ( ret < 0 )
                                prelude_msgbuf_set(msgbuf, PRELUDE_MSG_OPTION_ERROR, *out ? strlen(out) + 1 : 0, out);
				
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
                        ret = prelude_extract_characters_safe(&tmp, buf, dlen);
                        if ( ret < 0 ) 
                                return ret;
                        
                        prelude_option_set_value(opt, tmp);
                        break;

                case PRELUDE_MSG_OPTION_NAME:
                        ret = prelude_extract_characters_safe(&tmp, buf, dlen);
                        if ( ret < 0 )
                                return ret;
                        
                        prelude_option_set_longopt(opt, tmp);
                        break;
                        
                case PRELUDE_MSG_OPTION_DESC:
                        ret = prelude_extract_characters_safe(&tmp, buf, dlen);
                        if ( ret < 0 )
                                return ret;

                        prelude_option_set_description(opt, tmp);
                        break;
                        
                case PRELUDE_MSG_OPTION_HELP:
                        ret = prelude_extract_characters_safe(&tmp, buf, dlen);
                        if ( ret < 0 )
                                return ret;
                        
                        prelude_option_set_help(opt, tmp);
                        break;
                        
                case PRELUDE_MSG_OPTION_INPUT_VALIDATION:
                        ret = prelude_extract_characters_safe(&tmp, buf, dlen);
                        if ( ret < 0 )
                                return ret;
                        
                        prelude_option_set_input_validation_regex(opt, tmp);
                        break;

                case PRELUDE_MSG_OPTION_HAS_ARG:
                        ret = prelude_extract_uint8_safe(&tmpint, buf, dlen);
                        if ( ret < 0 )
                                return ret;

                        prelude_option_set_has_arg(opt, tmpint);
                        break;

                case PRELUDE_MSG_OPTION_TYPE:
                        ret = prelude_extract_uint8_safe(&tmpint, buf, dlen);
                        if ( ret < 0 )
                                return ret;

                        prelude_option_set_type(opt, tmpint);
                        break;
                        
                case PRELUDE_MSG_OPTION_INPUT_TYPE:
                        ret = prelude_extract_uint8_safe(&tmpint, buf, dlen);
                        if ( ret < 0 )
                                return ret;
                        
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
                        
                        ret = prelude_extract_uint32_safe(request_id, buf, dlen);
                        if ( ret < 0 )
                                return ret;
                        break;
                        
                case PRELUDE_MSG_OPTION_VALUE:
                        type = PRELUDE_OPTION_REPLY_TYPE_GET;
                        
                        ret = prelude_extract_characters_safe((const char **) value, buf, dlen);                        
                        if ( ret < 0 )
                                return ret;
                        break;

                case PRELUDE_MSG_OPTION_ERROR:
                        type = PRELUDE_OPTION_REPLY_TYPE_ERROR;
                        if ( ! dlen ) {
                                *value = "No error message";	
                                break;
                        }
			
                        ret = prelude_extract_characters_safe((const char **) value, buf, dlen);
                        if ( ret < 0 )
                                return ret;
                        break;
                        
                case PRELUDE_MSG_OPTION_SOURCE_ID:
                        ret = prelude_extract_uint64_safe(source_id, buf, dlen);
                        if ( ret < 0 )
                                return ret;
                        break;
                        
                case PRELUDE_MSG_OPTION_TARGET_ID:
                        break;

                case PRELUDE_MSG_OPTION_LIST:
                        type = PRELUDE_OPTION_REPLY_TYPE_LIST;
                        *value = prelude_option_new(NULL);
                        
                        ret = read_option_list(msg, *value, NULL);
                        if ( ret < 0 )
                                return ret;
                        break;
                        
                default:
                        log(LOG_ERR, "unknow tag : %d.\n", tag);
                        return -1;
                }
        }
        
        return type;
}
