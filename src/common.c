/*****
*
* Copyright (C) 2002-2005 PreludeIDS Technologies. All Rights Reserved.
* Author: Yoann Vandoorselaere <yoann@prelude-ids.com>
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

#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <assert.h>
#include <errno.h>
#include <stdarg.h>
#include <time.h>

#include "prelude-error.h"
#include "libmissing.h"

#include "idmef.h"
#include "prelude-log.h"
#include "common.h"


static int find_absolute_path(const char *cwd, const char *file, char **path)
{
        int ret;
        char buf[PATH_MAX];
        const char *ptr;
        char *pathenv = strdup(getenv("PATH")), *old = pathenv;
        
        while ( (ptr = strsep(&pathenv, ":")) ) {

                ret = strcmp(ptr, ".");
                if ( ret == 0 )
                        ptr = cwd;
                        
                snprintf(buf, sizeof(buf), "%s/%s", ptr, file);

                ret = access(buf, F_OK);
                if ( ret < 0 )
                        continue;
                
                *path = strdup(ptr);
                free(old);

                return 0;
        }

        free(old);
        
        return -1;
}




/**
 * prelude_resolve_addr:
 * @hostname: Hostname to lookup.
 * @addr: Pointer on an in_addr structure to store the result in.
 *
 * Lookup @hostname, and store the resolved IP address in @addr.
 *
 * Returns: 0 on success, -1 if an error occured.
 */ 
int prelude_resolve_addr(const char *hostname, struct in_addr *addr) 
{
        int ret;
        struct hostent *h;

        /*
         * This is not an hostname. No need to resolve.
         */
        ret = inet_aton(hostname, addr);
        if ( ret != 0 ) 
                return 0;
        
        h = gethostbyname(hostname);
        if ( ! h )
                return -1;

        assert((unsigned int) h->h_length <= sizeof(*addr));
        
        memcpy(addr, h->h_addr, h->h_length);
                
        return 0;
}



/**
 * prelude_realloc:
 * @ptr: Pointer on a memory block.
 * @size: New size.
 *
 * prelude_realloc() changes the size of the memory block pointed to by @ptr
 * to @size bytes. The contents will be unchanged to the minimum of the old
 * and new sizes; newly allocated memory will be uninitialized.  If ptr is NULL,
 * the call is equivalent to malloc(@size); if @size is equal to zero, the call
 * is equivalent to free(ptr). Unless ptr is NULL, it must have been returned by
 * an earlier call to malloc(), calloc() or realloc().
 *
 * This function exist because some version of realloc() doesn't handle the
 * case where @ptr is NULL. Even thought ANSI allow it.
 *
 * Returns: returns a pointer to the newly allocated memory, which is suitably
 * aligned for any kind of variable and may be different from ptr, or NULL if the
 * request fails. If size was equal to 0, either NULL or a pointer suitable to be
 * passed to free() is returned.  If  realloc() fails the original block is left
 * untouched - it is not freed or moved.
 *
 * Returns: a pointer to the newly allocated memory.
 */
void *prelude_realloc(void *ptr, size_t size) 
{
        if ( ptr == NULL )
                return malloc(size);
        else
                return realloc(ptr, size);
}




/**
 * prelude_open_persistant_tmpfile:
 * @filename: Path to the file to open.
 * @flags: Flags that should be used to open the file.
 * @mode: Mode that should be used to open the file.
 *
 * Open a *possibly persistant* file for writing,
 * trying to avoid symlink attack as much as possible.
 *
 * The file is created if it does not exist.
 * Refer to open(2) for @flags and @mode meaning.
 *
 * Returns: A valid file descriptor on success, -1 if an error occured.
 */
int prelude_open_persistant_tmpfile(const char *filename, int flags, mode_t mode) 
{
        int fd, ret;
        struct stat st;
        int secure_flags;
        
        /*
         * We can't rely on O_EXCL to avoid symlink attack,
         * as it could be perfectly normal that a file would already exist
         * and we would be open to a race condition between the time we lstat
         * it (to see if it's a link) and the time we open it, this time without
         * O_EXCL.
         */
        secure_flags = flags | O_CREAT | O_EXCL;
        
        fd = open(filename, secure_flags, mode);
        if ( fd >= 0 )
                return fd;

        if ( errno != EEXIST )
                return prelude_error_from_errno(errno);
                
        ret = lstat(filename, &st);
        if ( ret < 0 )
                return prelude_error_from_errno(errno);

        /*
         * There is a race between the lstat() and this open() call.
         * No atomic operation that I know off permit to fix it.
         * And we can't use O_TRUNC.
         */
        if ( S_ISREG(st.st_mode) ) 
                return open(filename, O_CREAT | flags, mode);
        
        else if ( S_ISLNK(st.st_mode) ) {
                prelude_log(PRELUDE_LOG_WARN, "- symlink attack detected for %s. Overriding.\n", filename);
                
                ret = unlink(filename);
                if ( ret < 0 )
                        return prelude_error_from_errno(errno);
                
                return prelude_open_persistant_tmpfile(filename, flags, mode);
                
        }
        
        return -1;
}




/**
 * prelude_read_multiline:
 * @fd: File descriptor to read input from.
 * @line: Pointer to a line counter.
 * @buf: Pointer to a buffer where the line should be stored.
 * @size: Size of the @buf buffer.
 *
 * This function handle reading line separated by the '\' character.
 *
 * Returns: 0 on success, -1 if an error ocurred.
 */
int prelude_read_multiline(FILE *fd, int *line, char *buf, size_t size) 
{
        size_t i;

        if ( ! fgets(buf, size, fd) )
                return -1;

        (*line)++;
         
        /*
         * We don't want to handle multiline in case this is a comment.
         */
        for ( i = 0; buf[i] != '\0' && isspace((int) buf[i]); i++ );
                
        if ( buf[i] == '#' )
                return prelude_read_multiline(fd, line, buf, size);
                
        i = strlen(buf);

        while ( --i > 0 && (buf[i] == ' ' || buf[i] == '\n') );
        
        if ( buf[i] == '\\' )
                return prelude_read_multiline(fd, line, buf + i, size - i);
                
        return 0;
}




/**
 * prelude_hton64:
 * @val: Value to convert to network byte order.
 *
 * The prelude_hton64() function converts the 64 bits unsigned integer @val
 * from host byte order to network byte order.
 *
 * Returns: @val in the network bytes order.
 */

uint64_t prelude_hton64(uint64_t val) 
{
        uint64_t tmp;
        
#ifdef WORDS_BIGENDIAN
        tmp = val;
#else
	union {
		uint64_t val64;
		uint32_t val32[2];
	} combo_r, combo_w;
		
	combo_r.val64 = val;
	
        /*
         * Put in network byte order
         */
	combo_w.val32[0] = htonl(combo_r.val32[1]);
	combo_w.val32[1] = htonl(combo_r.val32[0]);
	tmp = combo_w.val64;
#endif
        
        return tmp;
}



int prelude_get_file_name_and_path(const char *str, char **name, char **path)
{
        int ret = 0;
	char buf[512], *ptr, cwd[PATH_MAX];
                
        getcwd(cwd, sizeof(cwd));
        
        ptr = strrchr(str, '/');
        if ( ! ptr ) {                
                ret = find_absolute_path(cwd, str, path);
                if ( ret == 0 ) {
                        *name = strdup(str);
                        return (*name) ? 0 : prelude_error_from_errno(errno);
                }
        }

        if ( *str != '/' ) {
                while ( *str == '.' && *(str + 1) == '.' && (ptr = strrchr(cwd, '/')) ) {
                        str += 3;
                        *ptr = '\0';
                }
                
                ret = snprintf(buf, sizeof(buf), "%s/%s", cwd, (*str == '.') ? str + 2 : str);
                if ( ret < 0 || ret >= sizeof(buf) )
                        return prelude_error(PRELUDE_ERROR_INVAL_LENGTH);

                return prelude_get_file_name_and_path(buf, name, path);
        }
        
        ret = access(str, F_OK);
        if ( ret < 0 )
                return prelude_error_from_errno(errno);
        
        *ptr = 0;       
        *path = strdup(str);
        if ( ! *path )
                return prelude_error_from_errno(errno);
        
        *ptr = '/';
        
        *name = strdup(ptr + 1);
        if ( ! *name ) {
                free(*path);
                return prelude_error_from_errno(errno);
        }
        
	return 0;
}




int prelude_get_gmt_offset(int32_t *gmtoff)
{
	time_t t = 0;
	struct tm tm_local;

	if ( ! localtime_r(&t, &tm_local) )
		return prelude_error_from_errno(errno);

	*gmtoff = tm_local.tm_hour * 3600 + tm_local.tm_min * 60 + tm_local.tm_sec;

	return 0;
}



time_t prelude_timegm(struct tm *tm)
{ 
        time_t retval;
        char *old, new[128];

        old = getenv("TZ");
        putenv("TZ=\"\"");
        tzset();
        
        retval = mktime(tm);

	if ( old ) {
		snprintf(new, sizeof (new), "TZ=%s", old);
		putenv(new);
	} else {
		putenv("TZ");
	}
        tzset();

        return retval;
}



void *prelude_sockaddr_get_inaddr(struct sockaddr *sa) 
{
        void *ret = NULL;
        
        if ( sa->sa_family == AF_INET )
                ret = &((struct sockaddr_in *) sa)->sin_addr;

#ifdef HAVE_IPV6
        else if ( sa->sa_family == AF_INET6 )
                ret = &((struct sockaddr_in6 *) sa)->sin6_addr;
#endif
        
        return ret;
}




/*
 * keep this function consistant with idmef_impact_severity_t value.
 */
prelude_msg_priority_t idmef_impact_severity_to_msg_priority(idmef_impact_severity_t severity)
{        
        static const prelude_msg_priority_t priority[] = {
                PRELUDE_MSG_PRIORITY_NONE, /* not bound                         */
                PRELUDE_MSG_PRIORITY_LOW,  /* IDMEF_IMPACT_SEVERITY_LOW    -> 1 */
                PRELUDE_MSG_PRIORITY_MID,  /* IDMEF_IMPACT_SEVERITY_MEDIUM -> 2 */
                PRELUDE_MSG_PRIORITY_HIGH, /* IDMEF_IMPACT_SEVERITY_HIGH   -> 3 */
                PRELUDE_MSG_PRIORITY_LOW   /* IDMEF_IMPACT_SEVERITY_INFO   -> 4 */
        };
                
        if ( severity >= (sizeof(priority) / sizeof(*priority)) )
                return PRELUDE_MSG_PRIORITY_NONE;
        
        return priority[severity];
}
