/*****
*
* Copyright (C) 1999, 2000 Yoann Vandoorselaere <yoann@mandrakesoft.com>
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
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>

#include "daemonize.h"
#include "common.h"



static int lockfile_set_pid(const char *lockfile, pid_t pid) 
{
        int ret;
        FILE *fd;
        
        fd = fopen(lockfile, "w");
        if ( ! fd ) {
                log(LOG_ERR, "couldn't open %s for writing : %m.\n", lockfile);
                return -1;
        }

        ret = fprintf(fd, "%d\n", pid);
        if ( ret < 0 ) {
                log(LOG_ERR, "couldn't write PID to lockfile.\n");
                fclose(fd);
                return -1;
        }

        fclose(fd);

        return 0;
}




int daemon_start(const char *lockfile)
{
	pid_t pid;

	pid = fork();
	if (pid < 0) {
                log(LOG_ERR, "fork failed.\n");
		return -1;
	}

        else if (pid) {
		log(LOG_INFO, "Daemon started, PID is %d.\n", (int) pid);
                if ( lockfile )
                        lockfile_set_pid(lockfile, pid);
		exit(0);
        }
        
        setsid();
        chdir("/");
        umask(0);

        return 0;
}




