/*****
*
* Copyright (C) 1999 - 2002 Yoann Vandoorselaere <yoann@mandrakesoft.com>
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
#include <unistd.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>


#include "daemonize.h"
#include "prelude-log.h"


static char slockfile[256];



static void lockfile_unlink(void) 
{
        int ret;

        ret = unlink(slockfile);
        if ( ret < 0 )
                log(LOG_ERR, "couldn't delete %s.\n", slockfile);
}




static const char *get_absolute_filename(const char *lockfile)  
{
        if ( *lockfile == '/' )
                snprintf(slockfile, sizeof(slockfile), "%s", lockfile);

        else {
                char dir[256];
                
                /*
                 * if lockfile is a relative path,
                 * deletion on exit() will not work because of the chdir("/") call.
                 * That's why we want to conver it to an absolute path.
                 */
                if ( ! getcwd(dir, sizeof(dir)) ) {
                        log(LOG_ERR, "couldn't get current working directory.\n");
                        return NULL;
                }
                
                snprintf(slockfile, sizeof(slockfile), "%s/%s", dir, lockfile);
        }
        
        return slockfile;
}




static int lockfile_get_exclusive(const char *lockfile) 
{
        int ret, fd;
        struct flock lock;

        fd = open(lockfile, O_WRONLY|O_CREAT, S_IRUSR|S_IWUSR);
        if ( fd < 0 ) {                
                log(LOG_ERR, "couldn't open %s for writing.\n", lockfile);
                return -1;
        }

        lock.l_type = F_WRLCK;    /* write lock */
        lock.l_start = 0;         /* from offset 0 */
        lock.l_whence = SEEK_SET; /* at the beggining of the file */
        lock.l_len = 0;           /* until EOF */
        
        ret = fcntl(fd, F_SETLK, &lock);
        if ( ret < 0 ) {
                if ( errno == EACCES || errno == EAGAIN ) 
                        log(LOG_INFO, "program is already running.\n");
                else
                        log(LOG_ERR, "couldn't set write lock.\n");

                return -1;
        }

        /*
         * lock is now held until program exit.
         */
        return fd;     
}



static int lockfile_write_pid(int fd, pid_t pid) 
{
        int ret;
        char buf[50];

        /*
         * Reset file size to 0.
         */
        ret = ftruncate(fd, 0);
        if ( ret < 0 ) {
                log(LOG_ERR, "couldn't truncate lockfile to 0 byte.\n");
                return -1;
        }
        
        snprintf(buf, sizeof(buf), "%d\n", pid);
        
        ret = write(fd, buf, strlen(buf));
        if ( ret < 0 ) {
                log(LOG_ERR, "couldn't write PID to lockfile.\n");
                return -1;
        }

        return 0;
}




/**
 * prelude_daemonize:
 * @lockfile: Filename to a lockfile.
 *
 * Put the caller in the background.
 * If @lockfile is not NULL, a lock for this program is being created.
 *
 * The lockfile is automatically unlinked on exit.
 *
 * Returns: 0 on success, -1 if an error occured.
 */
int prelude_daemonize(const char *lockfile)
{
	pid_t pid;
        int fd = 0;
        
        if ( lockfile ) {
                lockfile = get_absolute_filename(lockfile);
                if ( ! lockfile )
                        return -1;
                
                fd = lockfile_get_exclusive(lockfile);
                if ( fd < 0 )
                        return -1;
                
        }
        
	pid = fork();
	if ( pid < 0 ) {
                log(LOG_ERR, "fork failed.\n");
		return -1;
	}

        else if ( pid ) {
                if ( lockfile )
                        lockfile_write_pid(fd, pid);
                
                log(LOG_INFO, "Daemon started, PID is %d.\n", (int) pid);
		exit(0);
        }
        
        setsid();
        chdir("/");
        umask(0);
        
        /*
         * We want the lock to be unlinked upon normal exit.
         */
        if ( lockfile ) 
                atexit(lockfile_unlink);

        return 0;
}




