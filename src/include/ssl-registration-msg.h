/*****
*
* Copyright (C) 2001 Jeremie Brebec / Toussaint Mathieu
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

#ifndef SSL_REGISTRATION_MSG_H

#define SSL_REGISTRATION_MSG_H

#include <openssl/des.h>
#include <openssl/buffer.h>
#include <openssl/x509.h>

#define HEAD "PRELUDE_REGISTRATION_REQUEST"
#define HEADLENGTH 29

#define ACK "REGISTRATION_COMPLETE"
#define ACKLENGTH 21

#define BUFMAXSIZE 1400
#define PADMAXSIZE 7

#define SYSTEM_ERROR          -1
#define NOT_INSTALL_MSG       -2
#define INSTALL_MSG_CORRUPTED -3
#define WRONG_SIZE            -4
#define SUCCESS                0

int save_cert(const char *filename, char *cert, int certlen);

int x509_to_msg(X509 * x509, char *msg, int msglen,
		des_key_schedule * key1, des_key_schedule * key2);

X509 *load_x509(const char *certfilename);

int build_install_msg(BUF_MEM * input, char *output, int outputlen,
		      des_key_schedule * key1, des_key_schedule * key2);

int analyse_install_msg(char *input, int inputlen, char *output,
			int outpulen, des_key_schedule * key1,
			des_key_schedule * key2);

int des_generate_2key(des_key_schedule * key1, des_key_schedule * key2,
		      int verify);



int prelude_ssl_recv_cert(prelude_io_t *pio, char *out, int outlen,
                          des_key_schedule *skey1, des_key_schedule *skey2);

int prelude_ssl_send_cert(prelude_io_t *pio, const char *filename,
                          des_key_schedule *skey1, des_key_schedule *skey2);

int prelude_ssl_save_cert(const char *filename, char *cert, int certlen);

#endif
