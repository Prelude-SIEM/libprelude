/*****
*
* Copyright (C) 2001 Yoann Vandoorselaere <yoann@mandrakesoft.com>
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
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>

#include "prelude-io.h"
#include "prelude-auth.h"
#include "ssl-register.h"


int config_quiet = 0;


int main(int argc, char **argv) 
{
        int ret;
        char buf[1024];

        fprintf(stderr, "\n\nAuthentication method (cipher/plaintext) [cipher] : ");

        fgets(buf, sizeof(buf), stdin);
        buf[strlen(buf) - 1] = '\0';
        
        ret = strcmp(buf, "plaintext");
        if ( ret == 0 )
                ret = prelude_auth_create_account(SENSORS_AUTH_FILE);
        else {
#ifdef HAVE_SSL
                ret = ssl_add_certificate();
#else
                ret = -1;
                fprintf(stderr, "SSL support is not compiled in.\n");
#endif
        }
        
        exit(ret);
}






