/*****
*
* Copyright (C) 2002-2004 Yoann Vandoorselaere <yoann@prelude-ids.org>
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

#include "prelude-strbuf.h"
#include "prelude-log.h"
#include "common.h"



static int find_absolute_path(const char *cwd, const char *file, char **path)
{
        int ret;
        char buf[PATH_MAX];
        const char *ptr;
        char *pathenv = strdup(getenv("PATH")), *old = pathenv;
        
        while ( (ptr = prelude_strsep(&pathenv, ":")) ) {

                ret = strcmp(ptr, ".");
                if ( ret == 0 )
                        ptr = cwd;
                        
                snprintf(buf, sizeof(buf), "%s/%s", ptr, file);

                ret = access(buf, F_OK);
                if ( ret < 0 )
                        continue;
                
                free(old);
                *path = strdup(ptr);

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

        assert(h->h_length <= sizeof(*addr));
        
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

        if ( errno != EEXIST ) {
                log(LOG_ERR, "couldn't open %s.\n", filename);
                return -1;
        }
                
        ret = lstat(filename, &st);
        if ( ret < 0 ) {
                log(LOG_ERR, "couldn't get FD stat.\n");
                return -1;
        }

        /*
         * There is a race between the lstat() and this open() call.
         * No atomic operation that I know off permit to fix it.
         * And we can't use O_TRUNC.
         */
        if ( S_ISREG(st.st_mode) ) 
                return open(filename, O_CREAT | flags, mode);
        
        else if ( S_ISLNK(st.st_mode) ) {
                log(LOG_INFO, "- symlink attack detected for %s. Overriding.\n", filename);
                
                ret = unlink(filename);
                if ( ret < 0 ) {
                        log(LOG_ERR, "couldn't unlink %s.\n", filename);
                        return -1;
                }
                
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
        for ( i = 0; buf[i] != '\0' && isspace(buf[i]); i++ );
                
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
                        return 0;
                }
        }

        if ( *str != '/' ) {
                ret = snprintf(buf, sizeof(buf), "%s/%s", cwd, (*str == '.') ? str + 2 : str);
                if ( ret < 0 || ret >= sizeof(buf) )
                        return -1;
                
                return prelude_get_file_name_and_path(buf, name, path);
        }
        
        ret = access(str, F_OK);
        if ( ret < 0 )
                return -1;
        
        *ptr = 0;       
        *path = strdup(str);
        *ptr = '/';
        
        *name = strdup(ptr + 1);
                
	return 0;
}




int prelude_get_gmt_offset(time_t time_local, int *gmt_offset)
{
        time_t time_utc;
        struct tm tm_utc;
                
        if ( ! gmtime_r(&time_local, &tm_utc) ) {
                log(LOG_ERR, "error converting local time to utc time.\n");
                return -1;
        }
        
        tm_utc.tm_isdst = -1;
        time_utc = mktime(&tm_utc);
        *gmt_offset = time_local - time_utc;
        
        return 0;
}



char *prelude_strndup(const char *src, size_t n)
{
	size_t len;
	char *dst;

	len = strlen(src);
	if ( n < len )
		len = n;

	dst = malloc(len + 1);
	if ( ! dst )
		return NULL;

	memcpy(dst, src, len);
	dst[len] = 0;

	return dst;
}




char *prelude_strnstr(const char *str, const char *needed, size_t len)
{
        char *ptr;
        size_t nlen = strlen(needed);
        
        if ( ! *needed )
                return NULL;
        
        while ( (ptr = memchr(str, *needed, len)) ) {

                len -= (ptr - str);
                if ( len < nlen )
                        break;
                
                if ( memcmp(ptr, needed, nlen) == 0 )
                        return ptr;

                len--;
                str = ptr + 1;
        }

        return NULL;
}



char *prelude_strsep(char **stringp, const char *delim)
{
        size_t len;
        char *ptr, *start = *stringp;
        
        if ( ! start )
                return NULL;

        ptr = strstr(start, delim);
        if ( ! ptr ) {
                *stringp = NULL;
                return start;
        }
        
        len = strlen(delim);
        
        *ptr = 0;
        *stringp = ptr + len;

        return start;
}
