/*****
*
* Copyright (C) 1999 - 2004 PreludeIDS Technologies. All Rights Reserved.
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

#include "libmissing.h"

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
#include "prelude-error.h"


#ifndef PATH_MAX
# define PATH_MAX 1024
#endif


static char slockfile[256];



static void lockfile_unlink(void) 
{
        int ret;

        ret = unlink(slockfile);
        if ( ret < 0 )
                prelude_log(PRELUDE_LOG_ERR, "couldn't delete lockfile %s.\n", slockfile);
}




static int get_absolute_filename(const char *lockfile)  
{
        if ( *lockfile == '/' )
                snprintf(slockfile, sizeof(slockfile), "%s", lockfile);

        else {
                char dir[PATH_MAX];
                
                /*
                 * if lockfile is a relative path,
                 * deletion on exit() will not work because of the chdir("/") call.
                 * That's why we want to convert it to an absolute path.
                 */
                if ( ! getcwd(dir, sizeof(dir)) )
                        return prelude_error_from_errno(errno);
                
                snprintf(slockfile, sizeof(slockfile), "%s/%s", dir, lockfile);
        }
        
        return 0;
}




static int lockfile_get_exclusive(const char *lockfile) 
{
        int ret, fd;
        struct flock lock;

        fd = open(lockfile, O_WRONLY|O_CREAT, S_IRUSR|S_IWUSR);
        if ( fd < 0 )
                return prelude_error_from_errno(errno);

        lock.l_type = F_WRLCK;    /* write lock */
        lock.l_start = 0;         /* from offset 0 */
        lock.l_whence = SEEK_SET; /* at the beginning of the file */
        lock.l_len = 0;           /* until EOF */
        
        ret = fcntl(fd, F_SETLK, &lock);
        if ( ret < 0 ) {
                if ( errno == EACCES || errno == EAGAIN )
                        return prelude_error(PRELUDE_ERROR_DAEMONIZE_LOCK_HELD);
                
                close(fd);
                return prelude_error_from_errno(errno);
        }

        /*
         * lock is now held until program exits.
         */
        return fd;     
}



static int lockfile_write_pid(int fd, pid_t pid) 
{
        int ret;
        char buf[50];

        /*
         * Resets file size to 0.
         */
        ret = ftruncate(fd, 0);
        if ( ret < 0 )
                return prelude_error_from_errno(errno);
        
        snprintf(buf, sizeof(buf), "%d\n", (int) pid);
        
        ret = write(fd, buf, strlen(buf));
        if ( ret < 0 )
                return prelude_error_from_errno(errno);

        return 0;
}




/**
 * prelude_daemonize:
 * @lockfile: Filename to a lockfile.
 *
 * Puts caller in background.
 * If @lockfile is not NULL, a lock for this program is created.
 *
 * The lockfile is automatically unlinked on exit.
 *
 * Returns: 0 on success, -1 if an error occured.
 */
int prelude_daemonize(const char *lockfile)
{
	pid_t pid;
        int fd = 0, ret;
        
        if ( lockfile ) {
                ret = get_absolute_filename(lockfile);
                if ( ret < 0 )
                        return ret;
                
                fd = lockfile_get_exclusive(slockfile);
                if ( fd < 0 )
                        return fd;
        }
        
        pid = fork();
        if ( pid < 0 )
                return prelude_error_from_errno(errno);

        else if ( pid ) {

                if ( lockfile ) {
                        ret = lockfile_write_pid(fd, pid);
                        if ( ret < 0 )
                                return ret;
                }
                
                _exit(0);
        }
        
        setsid();
        chdir("/");
        umask(0);
        
        fclose(stdin);
        fclose(stdout);
        fclose(stderr);
        
        /*
         * We want the lock to be unlinked upon normal exit.
         */
        if ( lockfile ) 
                atexit(lockfile_unlink);

        return 0;
}




