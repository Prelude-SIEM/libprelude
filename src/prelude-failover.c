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
#include "prelude-message.h"
#include "prelude-failover.h"


#define MIN(x, y) (((x) < (y)) ? (x) : (y))
#define MAX(x, y) (((x) > (y)) ? (x) : (y))


struct prelude_failover {
        char *directory;
        prelude_io_t *fd;
        
        size_t cur_size;
        size_t size_limit;
        int prev_was_a_fragment;
        
        /*
         * Newer allocated index for a saved message.
         */
        unsigned int newer_index;

        /*
         * Older allocated index for a saved message.
         */
        unsigned int older_index;

        /*
         *
         */
        size_t to_be_deleted_size;
};




static int open_failover_fd(prelude_failover_t *failover,
                            char *filename, size_t size, int index, int flags)
{
        int fd;

        snprintf(filename, size, "%s/%u", failover->directory, index);
        
        fd = open(filename, flags, S_IRUSR|S_IWUSR);
        if ( fd < 0 ) {
                log(LOG_ERR, "couldn't open %s for writing.\n", filename);
                return -1;
        }

        prelude_io_set_sys_io(failover->fd, fd);

        return 0;
}




static int get_current_directory_index(prelude_failover_t *failover, const char *dirname) 
{
        int ret;
        DIR *dir;
        uint32_t tmp;
        struct stat st;
        char filename[256];
        struct dirent *item;
        
        dir = opendir(dirname);
        if ( ! dir ) {
                log(LOG_ERR, "couldn't open %s.\n", dirname);
                return -1;
        }

        failover->older_index = ~0;

        while ( (item = readdir(dir)) ) {

                ret = sscanf(item->d_name, "%u", &tmp);
                if ( ret != 1 )
                        continue;
                
                snprintf(filename, sizeof(filename), "%s/%s", dirname, item->d_name);
                
                ret = stat(filename, &st);
                if ( ret < 0 ) {
                        log(LOG_ERR, "error trying to stat %s.\n", filename);
                        return -1;
                }

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




static int failover_apply_quota(prelude_failover_t *failover, prelude_msg_t *new, unsigned int older_index)
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
        snprintf(filename, sizeof(filename), "%s/%u", failover->directory, older_index);
        
        ret = stat(filename, &st);
        if ( ret < 0 ) {
                log(LOG_ERR, "error trying to stat %s.\n", filename);
                return -1;
        }
        
        unlink(filename);
        failover->cur_size -= st.st_size;
                
        return failover_apply_quota(failover, new, older_index + 1);
}




static void delete_current(prelude_failover_t *failover)
{
        char filename[256];

        if ( (failover->older_index - 1) == 0 )
                return;
        
        snprintf(filename, sizeof(filename), "%s/%u", failover->directory, failover->older_index - 1);
        unlink(filename);
        
        failover->cur_size -= failover->to_be_deleted_size;
}




int prelude_failover_save_msg(prelude_failover_t *failover, prelude_msg_t *msg)
{
        int ret;
        ssize_t size;
        char filename[256];
        int flags = O_CREAT|O_EXCL|O_WRONLY;
                                              
        if ( failover->size_limit )
                failover_apply_quota(failover, msg, failover->older_index);

        if ( failover->prev_was_a_fragment ) {
                failover->newer_index--;
                flags = O_APPEND|O_WRONLY;
        }

        ret = open_failover_fd(failover, filename, sizeof(filename), failover->newer_index, flags);
        if ( ret < 0 ) 
                return -1;
        
        do {
                size = prelude_msg_write(msg, failover->fd);
        } while ( size < 0 && errno == EINTR );
        
        prelude_io_close(failover->fd);

        if ( size < 0 ) {
                log(LOG_ERR, "error writing message to %s.\n", prelude_msg_is_fragment(msg));
                unlink(filename);
                return -1;
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
        if ( ret < 0 ) 
                return -1;

        *msg = NULL;
        while ( (ret = prelude_msg_read(msg, failover->fd)) == prelude_msg_unfinished );

        prelude_io_close(failover->fd);
        
        if ( ret == prelude_msg_error ) {
                log(LOG_ERR, "error reading message index=%d.\n", failover->older_index);
                return -1;
        }

        size = prelude_msg_get_len(*msg);
        failover->to_be_deleted_size = size;
        
        failover->older_index++;
        
        return size;
}



        
prelude_failover_t *prelude_failover_new(const char *dirname)
{
        int ret;
        prelude_failover_t *new;
        
        new = calloc(1, sizeof(*new));
        if ( ! new ) {
                log(LOG_ERR, "memory exhausted.\n");
                return NULL;
        }
        
        new->fd = prelude_io_new();
        if ( ! new->fd ) {
                free(new);
                return NULL;
        }
        
        new->directory = strdup(dirname);
        if ( ! new->directory ) {
                log(LOG_ERR, "memory exhausted.\n");
                prelude_io_destroy(new->fd);
                free(new);
                return NULL;
        }

        ret = mkdir(dirname, S_IRUSR|S_IWUSR);
        if ( ret < 0 && errno != EEXIST ) {
                log(LOG_ERR, "error creating %s.\n", dirname);
                prelude_failover_destroy(new);
                return NULL;
        }
 
        ret = get_current_directory_index(new, dirname);
        if ( ret < 0 ) {
                prelude_failover_destroy(new);
                return NULL;
        }
                
        return new;
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



unsigned int prelude_failover_get_deleted_msg_count(prelude_failover_t *failover)
{
        return failover->newer_index - (failover->newer_index - (failover->older_index - 1));
}



unsigned int prelude_failover_get_available_msg_count(prelude_failover_t *failover)
{
        return failover->newer_index - failover->older_index;
}
