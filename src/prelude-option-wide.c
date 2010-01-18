/*****
*
* Copyright (C) 2004,2005 PreludeIDS Technologies. All Rights Reserved.
* Author: Yoann Vandoorselaere <yoann.v@prelude-ids.com>
*
* This file is part of the Prelude library.
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

#include "config.h"
#include "libmissing.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <netinet/in.h>

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



static int config_save_value(config_t *cfg, int rtype, prelude_option_t *last, int is_last_cmd,
                             char **prev, const char *option, const char *value, unsigned int *line)
{
        int ret = 0;
        char buf[1024];

        if ( ! (prelude_option_get_type(last) & PRELUDE_OPTION_TYPE_CFG) )
                return -1;

        if ( rtype != PRELUDE_MSG_OPTION_SET && rtype != PRELUDE_MSG_OPTION_DESTROY )
                return -1;

        if ( prelude_option_has_optlist(last) ) {

                if ( prelude_option_get_type(last) & PRELUDE_OPTION_TYPE_CONTEXT )
                        snprintf(buf, sizeof(buf), "%s=%s", option, (value) ? value : "default");
                else
                        snprintf(buf, sizeof(buf), "%s", option);

                if ( *prev )
                        free(*prev);

                *prev = strdup(buf);
                if ( ! *prev )
                        return prelude_error_from_errno(errno);

                if ( rtype == PRELUDE_MSG_OPTION_SET )
                        return _config_set(cfg, buf, NULL, NULL, line);

                else if ( is_last_cmd )
                        return _config_del(cfg, buf, NULL);

        }

        if ( rtype == PRELUDE_MSG_OPTION_SET )
                ret = _config_set(cfg, *prev, option, value, line);

        else if ( is_last_cmd )
                ret = _config_del(cfg, *prev, option);

        return ret;
}



static int parse_single(void **context, prelude_option_t **last, int is_last_cmd,
                        int rtype, const char *option, const char *value, prelude_string_t *out)
{
        int ret = 0;

        *last = prelude_option_search(*last, option, PRELUDE_OPTION_TYPE_WIDE, 0);
        if ( ! *last ) {
                prelude_string_sprintf(out, "Unknown option: %s.\n", option);
                return -1;
        }

        if ( rtype == PRELUDE_MSG_OPTION_SET )
                ret = prelude_option_invoke_set(*last, value, out, context);

        else if ( is_last_cmd ) {

                if ( rtype == PRELUDE_MSG_OPTION_DESTROY )
                        ret = prelude_option_invoke_destroy(*last, value, out, *context);

                else if ( rtype == PRELUDE_MSG_OPTION_GET )
                        ret = prelude_option_invoke_get(*last, value, out, *context);

                else if ( rtype == PRELUDE_MSG_OPTION_COMMIT )
                        ret = prelude_option_invoke_commit(*last, value, out, *context);
        }

        return ret;
}




static char *option_strsep(char **request)
{
        char *start = *request;
        prelude_bool_t ignore = FALSE;

        if ( ! *request )
                return NULL;

        while ( **request ) {
                if ( ignore == TRUE && **request == ']' )
                        ignore = FALSE;

                if ( ignore == FALSE && **request == '[' )
                        ignore = TRUE;

                if ( ignore == FALSE && **request == '.' ) {
                        **request = 0;
                        *request = *request + 1;
                        return start;
                }

                (*request)++;
        }

        if ( start != *request ) {
                *request = NULL;
                return start;
        }

        return NULL;
}



static int parse_request(prelude_client_t *client, int rtype, char *request, prelude_string_t *out)
{
        config_t *cfg;
        void *context = client;
        char pname[256], iname[256];
        prelude_option_t *last = NULL;
        int ret = 0, last_cmd = 0, ent;
        char *str, *value, *prev = NULL, *ptr = NULL;
        unsigned int line = 0;

        ret = _config_open(&cfg, prelude_client_get_config_filename(client));
        if ( ret < 0 )
                return ret;

        value = request;
        strsep(&value, "=");

        while ( (str = (option_strsep(&request))) ) {

                if ( ! request ) {
                        last_cmd = 1;
                        ptr = value;
                }

                *pname = 0;
                *iname = 0;

                ent = ret = sscanf(str, "%255[^[][%255[^]]", pname, iname);
                if ( ret < 1 ) {
                        prelude_string_sprintf(out, "Error parsing option path");
                        break;
                }

                ret = parse_single(&context, &last, last_cmd, rtype, pname, (ent == 2) ? iname : ptr, out);
                if ( ret < 0 )
                        break;

                config_save_value(cfg, rtype, last, last_cmd, &prev, pname, (ent == 2) ? iname : ptr, &line);
        }

        _config_close(cfg);
        free(prev);

        return ret;
}



static void send_string(prelude_msgbuf_t *msgbuf, prelude_string_t *out, int type)
{
        size_t len;

        len = prelude_string_is_empty(out) ? 0 : (prelude_string_get_len(out) + 1);
        if ( type == PRELUDE_MSG_OPTION_VALUE && ! len )
                return;

        prelude_msgbuf_set(msgbuf, type, len, prelude_string_get_string(out));
}



static void send_error(prelude_msgbuf_t *msgbuf, const char *fmt, ...)
{
        int ret;
        va_list ap;
        prelude_string_t *out;

        va_start(ap, fmt);

        ret = prelude_string_new(&out);
        if ( ret < 0 )
                return;

        prelude_string_vprintf(out, fmt, ap);

        va_end(ap);

        send_string(msgbuf, out, PRELUDE_MSG_OPTION_ERROR);
        prelude_string_destroy(out);
}



static int read_option_request(prelude_client_t *client, prelude_msgbuf_t *msgbuf, prelude_msg_t *msg)
{
        void *buf;
        uint8_t tag;
        char *request;
        uint32_t len, hop;
        int ret, type = -1;
        prelude_string_t *out;

        while ( prelude_msg_get(msg, &tag, &len, &buf) == 0 ) {

                switch (tag) {

                case PRELUDE_MSG_OPTION_SET:
                case PRELUDE_MSG_OPTION_GET:
                case PRELUDE_MSG_OPTION_COMMIT:
                case PRELUDE_MSG_OPTION_DESTROY:
                        type = tag;
                        break;

                case PRELUDE_MSG_OPTION_HOP:
                        ret = prelude_extract_uint32_safe(&hop, buf, len);
                        if ( ret < 0 )
                                return ret;

                        hop = htonl(hop - 1);
                        prelude_msgbuf_set(msgbuf, tag, len, &hop);
                        break;

                case PRELUDE_MSG_OPTION_TARGET_ID:
                case PRELUDE_MSG_OPTION_TARGET_INSTANCE_ID:
                case PRELUDE_MSG_OPTION_REQUEST_ID:
                        prelude_msgbuf_set(msgbuf, tag, len, buf);
                        break;

                case PRELUDE_MSG_OPTION_LIST:
                        return prelude_option_wide_send_msg(msgbuf, client);

                case PRELUDE_MSG_OPTION_VALUE:
                        ret = prelude_extract_characters_safe((const char **) &request, buf, len);
                        if ( ret < 0 )
                                return ret;

                        if ( type < 0 || ! request ) {
                                send_error(msgbuf, "No request specified");
                                return -1;
                        }

                        ret = prelude_string_new(&out);
                        if ( ret < 0 )
                                return ret;

                        ret = parse_request(client, type, request, out);
                        send_string(msgbuf, out, (ret < 0) ? PRELUDE_MSG_OPTION_ERROR : PRELUDE_MSG_OPTION_VALUE);

                        prelude_string_destroy(out);
                        break;

                default:
                        send_error(msgbuf, "Unknown option tag: %d", tag);
                        return -1;
                }
        }

        return 0;
}



static int read_option_list(prelude_msg_t *msg, prelude_option_t *opt, uint64_t *source_id)
{
        int ret;
        void *buf;
        uint8_t tag;
        const char *tmp = NULL;
        uint32_t dlen, tmpint = 0;
        prelude_option_t *newopt;

        if ( ! opt )
                return -1;

        while ( (ret = prelude_msg_get(msg, &tag, &dlen, &buf)) == 0 ) {

                switch (tag) {

                case PRELUDE_MSG_OPTION_START:
                        ret = prelude_option_new(opt, &newopt);
                        if ( ret < 0 )
                                break;

                        read_option_list(msg, newopt, source_id);
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
                        ret = prelude_extract_uint32_safe(&tmpint, buf, dlen);
                        if ( ret < 0 )
                                return ret;

                        prelude_option_set_has_arg(opt, tmpint);
                        break;

                case PRELUDE_MSG_OPTION_TYPE:
                        ret = prelude_extract_uint32_safe(&tmpint, buf, dlen);
                        if ( ret < 0 )
                                return ret;

                        prelude_option_set_type(opt, tmpint);
                        break;

                case PRELUDE_MSG_OPTION_INPUT_TYPE:
                        ret = prelude_extract_uint32_safe(&tmpint, buf, dlen);
                        if ( ret < 0 )
                                return ret;

                        prelude_option_set_input_type(opt, tmpint);
                        break;

                default:
                        /*
                         * for compatibility purpose, don't return an error on unknow tag.
                         */
                        prelude_log(PRELUDE_LOG_WARN, "unknown option tag %d.\n", tag);
                }
        }

        return 0;
}



int prelude_option_process_request(prelude_client_t *client, prelude_msg_t *msg, prelude_msgbuf_t *out)
{
        uint8_t tag;

        tag = prelude_msg_get_tag(msg);

        if ( tag != PRELUDE_MSG_OPTION_REQUEST )
                return -1;

        prelude_msg_set_tag(prelude_msgbuf_get_msg(out), PRELUDE_MSG_OPTION_REPLY);

        return read_option_request(client, out, msg);
}




int prelude_option_push_request(prelude_msgbuf_t *msg, int type, const char *request)
{
        prelude_msgbuf_set(msg, type, 0, 0);

        if ( request )
                prelude_msgbuf_set(msg, PRELUDE_MSG_OPTION_VALUE, strlen(request) + 1, request);

        return 0;
}



int prelude_option_new_request(prelude_msgbuf_t *msgbuf,
                               uint32_t request_id, uint64_t *target_id, size_t size)
{
        size_t i;
        uint32_t hop, instance_id = 0;

        prelude_msg_set_tag(prelude_msgbuf_get_msg(msgbuf), PRELUDE_MSG_OPTION_REQUEST);

        /*
         * the caller is supposed to provide a full path,
         * from him to the destination, to the original hop is 1.
         */
        hop = htonl(1);

        for ( i = 0; i < size; i++ )
                target_id[i] = prelude_hton64(target_id[i]);

        request_id = htonl(request_id);
        prelude_msgbuf_set(msgbuf, PRELUDE_MSG_OPTION_REQUEST_ID, sizeof(request_id), &request_id);
        prelude_msgbuf_set(msgbuf, PRELUDE_MSG_OPTION_TARGET_ID, i * sizeof(*target_id), target_id);
        prelude_msgbuf_set(msgbuf, PRELUDE_MSG_OPTION_TARGET_INSTANCE_ID, sizeof(instance_id), &instance_id);
        prelude_msgbuf_set(msgbuf, PRELUDE_MSG_OPTION_HOP, sizeof(hop), &hop);

        return 0;
}



int prelude_option_recv_reply(prelude_msg_t *msg, uint64_t *source_id,
                              uint32_t *request_id, void **value)
{
        void *buf;
        uint8_t tag;
        uint32_t dlen;
        int ret, type = -1;

        *value = NULL;

        while ( (ret = prelude_msg_get(msg, &tag, &dlen, &buf)) == 0 ) {

                switch (tag) {
                case PRELUDE_MSG_OPTION_HOP:
                        break;

                case PRELUDE_MSG_OPTION_REQUEST_ID:
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

                case PRELUDE_MSG_OPTION_TARGET_ID:
                        if ( dlen % sizeof(uint64_t) != 0 || dlen < (2 * sizeof(uint64_t)) )
                                return -1;

                        *source_id = prelude_extract_uint64((unsigned char *) buf + (dlen - sizeof(uint64_t)));
                        break;

                case PRELUDE_MSG_OPTION_LIST:
                        type = PRELUDE_OPTION_REPLY_TYPE_LIST;

                        ret = prelude_option_new(NULL, (prelude_option_t **) value);
                        if ( ret < 0 )
                                return ret;

                        ret = read_option_list(msg, *value, NULL);
                        if ( ret < 0 )
                                return ret;
                        break;

                default:
                        prelude_log(PRELUDE_LOG_WARN, "unknown option tag %d.\n", tag);
                }
        }

        return type;
}
