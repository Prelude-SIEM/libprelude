/*****
*
* Copyright (C) 1998,1999,2000 Vandoorselaere Yoann
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

#include <stdarg.h>
#include <errno.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>



#define do_init(func, name) do {                         \
        log(LOG_INFO, "- %s\n", name);                   \
        if ( (func) < 0 )                                \
               exit(1);                                  \
} while(0);


#define do_init_nofail(func, name) do {                  \
        log(LOG_INFO, "- %s\n", name);                   \
        (func);                                          \
} while(0);



extern int config_quiet;


#define log(priority, args...) do {                                                                    \
        if ( config_quiet ) {                                                                          \
                if ( priority == LOG_ERR )                                                              \
                        syslog(priority, "%s:%s:%d : (errno=%m) : ", __FILE__, __FUNCTION__, __LINE__);\
                syslog(priority, args);                                                                \
        } else {                                                                                       \
                FILE *out;                                                                             \
                                                                                                       \
                if ( priority == LOG_ERR ) {                                                           \
                        out = stderr;                                                                  \
                        fprintf(out, "%s:%s:%d : (errno=%m) : ", __FILE__, __FUNCTION__, __LINE__);    \
                } else                                                                                 \
                        out = stdout;                                                                  \
                                                                                                       \
                fprintf(out, args);                                                                    \
        }                                                                                              \
} while(0)



