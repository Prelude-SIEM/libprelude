/* Copyright (C) 2005 Free Software Foundation, Inc.
 * Written by Yoann Vandoorselaere <yoann@prelude-ids.org>
 *
 * The file is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This file is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this file; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA.
 */

#ifdef HAVE_CONFIG_H
 #include <config.h>
#endif

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>

#include "ftw_.h"
#include "pathmax.h"


static int get_path_infos(const char *path, struct stat *st, int *flag)
{
        int ret;
        
        ret = stat(path, st);
        if ( ret < 0 ) {
                *flag = FTW_NS;
                return -1;
        }
        
        if ( S_ISREG(st->st_mode) )
                *flag = FTW_F;
                
        else if ( S_ISDIR(st->st_mode) )
                *flag = FTW_D;

#ifdef S_ISLNK        
        else if ( S_ISLNK(st->st_mode) )
                *flag = FTW_SL;
#endif

        return ret;
}



int ftw(const char *dir,
        int (*fn)(const char *file, const struct stat *sb, int flag), int nopenfd) 
{
        DIR *d;
        size_t len;
        struct stat st;
        struct dirent *de;
        int flag, ret = 0;
        char filename[PATH_MAX];

        ret = get_path_infos(dir, &st, &flag);
        if ( ret < 0 )
                return -1;
                        
        d = opendir(dir);
        if ( ! d )
                return (errno == EACCES) ? fn(dir, &st, FTW_DNR) : -1;

        ret = fn(dir, &st, flag);
        if ( ret < 0 ) {
                closedir(d);
                return ret;
        }
        
        while ( (de = readdir(d)) ) {          

                len = snprintf(filename, sizeof(filename), "%s/%s", dir, de->d_name);
                if ( len < 0 || len >= sizeof(filename) ) {
                        errno = ENAMETOOLONG;
                        return -1;
                }
                
                ret = get_path_infos(filename, &st, &flag);
                if ( ret < 0 )
                        break;
                                
                if ( flag == FTW_D ) {
                        if ( strcmp(de->d_name, "..") == 0 || strcmp(de->d_name, ".") == 0 )
                                continue;

                        ret = ftw(filename, fn, nopenfd);
                        if ( ret < 0 )
                                break;
                        
                        continue;
                }
                
                ret = fn(filename, (flag == FTW_NS) ? NULL : &st, flag);
                if ( ret < 0 )
                        break;
        }

        closedir(d);

        return ret;
}
