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

#ifndef _LIBPRELUDE_PRELUDE_GETOPT_WIDE_H
#define _LIBPRELUDE_PRELUDE_GETOPT_WIDE_H

#define PRELUDE_OPTION_REPLY_TYPE_SET   0x01
#define PRELUDE_OPTION_REPLY_TYPE_GET   0x02
#define PRELUDE_OPTION_REPLY_TYPE_LIST  0x04
#define PRELUDE_OPTION_REPLY_TYPE_ERROR 0x08


int prelude_option_push_request(prelude_msgbuf_t *msg, int type, const char *request);

int prelude_option_new_request(prelude_client_t *client, prelude_msgbuf_t *msgbuf, uint32_t request_id, uint64_t target_id);

int prelude_option_send_request(prelude_client_t *client, uint32_t request_id,
                                uint64_t target_id, int type, const char *option, const char *value);

int prelude_option_process_request(prelude_client_t *client, prelude_io_t *fd, prelude_msg_t *msg);

prelude_option_t *prelude_option_read_option_list(prelude_msg_t *msg, uint64_t *source_id);

int prelude_option_recv_reply(prelude_msg_t *msg, uint64_t *source_id, uint32_t *request_id, void **value);

#endif /* _LIBPRELUDE_PRELUDE_GETOPT_WIDE_H */

