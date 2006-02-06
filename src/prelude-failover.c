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
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <fcntl.h>
#include <errno.h>

#include "prelude-log.h"
#include "prelude-io.h"
#include "prelude-msg.h"
#include "prelude-failover.h"

#define PRELUDE_ERROR_SOURCE_DEFAULT PRELUDE_ERROR_SOURCE_FAILOVER
#include "prelude-error.h"


struct prelude_failover {
        char *directory;
        prelude_io_t *fd;
        
        size_t cur_size;
        size_t size_limit;
        int prev_was_a_fragment;
        
        /*
         * Newer allocated index for a saved message.
         */
        unsigned long newer_index;

        /*
         * Older allocated index for a saved message.
         */
        unsigned long older_index;

        /*
         *
         */
        size_t to_be_deleted_size;
};




static int open_failover_fd(prelude_failover_t *failover,
                            char *filename, size_t size, unsigned long index, int flags)
{
        int fd;

        snprintf(filename, size, "%s/%lu", failover->directory, index);
        
        fd = open(filename, flags, S_IRUSR|S_IWUSR);        
        if ( fd < 0 )
                return prelude_error_verbose(PRELUDE_ERROR_GENERIC, "could not open '%s' for writing: %s", filename, strerror(errno));
        
        prelude_io_set_sys_io(failover->fd, fd);

        return 0;
}




static int get_current_directory_index(prelude_failover_t *failover, const char *dirname) 
{
        int ret;
        DIR *dir;
        struct stat st;
        unsigned long tmp;
        char filename[256];
        struct dirent *item;
        
        dir = opendir(dirname);
        if ( ! dir )
                return prelude_error_verbose(PRELUDE_ERROR_GENERIC, "could not open directory '%s': %s", dirname, strerror(errno));

        failover->older_index = ~0;

        while ( (item = readdir(dir)) ) {

                ret = sscanf(item->d_name, "%lu", &tmp);
                if ( ret != 1 )
                        continue;
                
                snprintf(filename, sizeof(filename), "%s/%s", dirname, item->d_name);
                
                ret = stat(filename, &st);
                if ( ret < 0 )
                        return prelude_error_verbose(PRELUDE_ERROR_GENERIC, "error stating '%s': %s", filename, strerror(errno));

                failover->cur_size += st.st_size;
                
                failover->older_index = MIN(failover->older_index, tmp);
                failover->newer_index = MAX(failover->newer_index, tmp + 1);
        }

        closedir(dir);

        if ( failover->older_index == ~0 )
                failover->older_index = 1;

        if ( ! failover->newer_index )
                failover->newer_index = 1;
        
        return 0;
}




static int failover_apply_quota(prelude_failover_t *failover, prelude_msg_t *new, unsigned long older_index)
{
        int ret;
        struct stat st;
        char filename[256];

        if ( (failover->cur_size + prelude_msg_get_len(new)) <= failover->size_limit ) {
                failover->older_index = older_index;
                return 0;
        }
        
        /*
         * check length of the oldest entry.
         */
        snprintf(filename, sizeof(filename), "%s/%lu", failover->directory, older_index);
        
        ret = stat(filename, &st);
        if ( ret < 0 )
                return prelude_error_verbose(PRELUDE_ERROR_GENERIC, "error stating '%s': %s", filename, strerror(errno));
        
        unlink(filename);
        failover->cur_size -= st.st_size;
                
        return failover_apply_quota(failover, new, older_index + 1);
}




static void delete_current(prelude_failover_t *failover)
{
        char filename[256];

        if ( (failover->older_index - 1) == 0 )
                return;
        
        snprintf(filename, sizeof(filename), "%s/%lu", failover->directory, failover->older_index - 1);
        unlink(filename);
        
        failover->cur_size -= failover->to_be_deleted_size;
}



static ssize_t get_file_size(const char *filename)
{
	int ret;
	struct stat st;

	ret = stat(filename, &st);
	if ( ret < 0 ) 
		return prelude_error_verbose(PRELUDE_ERROR_GENERIC, "error stating '%s': %s", filename, strerror(errno));

	return st.st_size;
}



int prelude_failover_save_msg(prelude_failover_t *failover, prelude_msg_t *msg)
{
        int ret;
        ssize_t size;
        char filename[256];
        int flags = O_CREAT|O_EXCL|O_WRONLY;
                                              
        if ( failover->size_limit ) {
                ret = failover_apply_quota(failover, msg, failover->older_index);
                if ( ret < 0 )
                        return ret;
        }
        
        if ( failover->prev_was_a_fragment ) {
                failover->newer_index--;
                flags = O_APPEND|O_WRONLY;
        }
        
        ret = open_failover_fd(failover, filename, sizeof(filename), failover->newer_index, flags);        
        if ( ret < 0 ) 
                return ret;
        
        do {
                size = prelude_msg_write(msg, failover->fd);
        } while ( size < 0 && errno == EINTR );
        
        prelude_io_close(failover->fd);

        if ( size < 0 ) {
                unlink(filename);
                return prelude_error_verbose(PRELUDE_ERROR_GENERIC, "error writing message to '%s': %s",
                                             filename, prelude_strerror((prelude_error_t) size));
        }
        
        failover->cur_size += size;
                
        failover->newer_index++;
        failover->prev_was_a_fragment = prelude_msg_is_fragment(msg);
        
        return 0;
}




ssize_t prelude_failover_get_saved_msg(prelude_failover_t *failover, prelude_msg_t **msg)
{
        int ret;
        ssize_t size;
        char filename[256];
        
        delete_current(failover);
        
        if ( failover->older_index == failover->newer_index ) {
                failover->older_index = failover->newer_index = 1;
                return 0;
        }

        ret = open_failover_fd(failover, filename, sizeof(filename), failover->older_index, O_RDONLY);
        if ( ret < 0 ) {
                failover->older_index++;
                failover->to_be_deleted_size = get_file_size(filename);
                return ret;
        }

        *msg = NULL;
        ret = prelude_msg_read(msg, failover->fd);
        prelude_io_close(failover->fd);
        
        if ( ret < 0 ) {
                ret = prelude_error_verbose(PRELUDE_ERROR_GENERIC, "error reading message index '%d': %s",
                                            failover->older_index, prelude_strerror(ret));

                failover->older_index++;
                failover->to_be_deleted_size = get_file_size(filename);
                return ret;
        }

        failover->older_index++;
        size = prelude_msg_get_len(*msg);
        failover->to_be_deleted_size = size;

        return size;
}



        
int prelude_failover_new(prelude_failover_t **out, const char *dirname)
{
        int ret;
        prelude_failover_t *new;
        
        new = calloc(1, sizeof(*new));
        if ( ! new )
                return prelude_error_from_errno(errno);
        
        ret = prelude_io_new(&new->fd);
        if ( ret < 0 ) {
                free(new);
                return ret;
        }
        
        new->directory = strdup(dirname);
        if ( ! new->directory ) {
                prelude_io_destroy(new->fd);
                free(new);
                return prelude_error_from_errno(errno);
        }

        ret = mkdir(dirname, S_IRWXU|S_IRWXG);
        if ( ret < 0 && errno != EEXIST ) {
                prelude_failover_destroy(new);
                return prelude_error_verbose(PRELUDE_ERROR_GENERIC,
                                             "could not create directory '%s': %s", dirname, strerror(errno));
        }
        
        ret = get_current_directory_index(new, dirname);
        if ( ret < 0 ) {
                prelude_failover_destroy(new);
                return ret;
        }

        *out = new;

        return 0;
}



void prelude_failover_destroy(prelude_failover_t *failover)
{
        prelude_io_destroy(failover->fd);
        free(failover->directory);
        free(failover);
}




void prelude_failover_set_quota(prelude_failover_t *failover, size_t limit) 
{
        failover->size_limit = limit;
}



unsigned long prelude_failover_get_deleted_msg_count(prelude_failover_t *failover)
{
        unsigned long available = prelude_failover_get_available_msg_count(failover);
        return (failover->newer_index - 1) - available;
}



unsigned long prelude_failover_get_available_msg_count(prelude_failover_t *failover)
{
        return failover->newer_index - failover->older_index;
}
