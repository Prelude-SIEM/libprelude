/*****
*
* Copyright (C) 2002-2005,2006,2007 PreludeIDS Technologies. All Rights Reserved.
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
#include "libmissing.h"

#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

#if !((defined _WIN32 || defined __WIN32__) && !defined __CYGWIN__)
# include <sys/mman.h>
#endif

#include "prelude-error.h"
#include "idmef.h"
#include "prelude-log.h"
#include "common.h"



extern char _prelude_init_cwd[PATH_MAX];



static prelude_string_t *get_message_ident(prelude_ident_t *ident)
{
        int ret;
        prelude_string_t *str;

        ret = prelude_string_new(&str);
        if ( ret < 0 )
                return NULL;

        ret = prelude_ident_generate(ident, str);
        if ( ret < 0 ) {
                prelude_string_destroy(str);
                return NULL;
        }

        return str;
}



static int find_absolute_path(const char *cwd, const char *file, char **path)
{
        int ret;
        char buf[PATH_MAX];
        const char *ptr;
        char *pathenv = strdup(getenv("PATH")), *old = pathenv;

        while ( (ptr = strsep(&pathenv, ":")) ) {

                ret = strcmp(ptr, ".");
                if ( ret == 0 ) {
                        if ( *cwd == 0 )
                                continue;

                        ptr = cwd;
                }

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
 * _prelude_realloc:
 * @ptr: Pointer on a memory block.
 * @size: New size.
 *
 * prelude_realloc() changes the size of the memory block pointed by @ptr
 * to @size bytes. The contents will be unchanged to the minimum of the old
 * and new sizes; newly allocated memory will be uninitialized.  If ptr is NULL,
 * the call is equivalent to malloc(@size); if @size is equal to zero, the call
 * is equivalent to free(ptr). Unless ptr is NULL, it must have been returned by
 * an earlier call to malloc(), calloc() or realloc().
 *
 * This function exists because some versions of realloc() don't handle the
 * case where @ptr is NULL. Even though ANSI requires it.
 *
 * Returns: a pointer to the newly allocated memory, which is suitably
 * aligned for any kind of variable and may be different from ptr, or NULL if the
 * request fails. If size was equal to 0, either NULL or a pointer suitable to be
 * passed to free() is returned.  If  realloc() fails, the original block is left
 * untouched - it is not freed nor moved.
 */
void *_prelude_realloc(void *ptr, size_t size)
{
        if ( ptr == NULL )
                return malloc(size);
        else
                return realloc(ptr, size);
}




/**
 * prelude_read_multiline:
 * @fd: File descriptor to read input from.
 * @line: Pointer to a line counter.
 * @buf: Pointer to a buffer where the line should be stored.
 * @size: Size of the @buf buffer.
 *
 * This function handles line reading separated by the '\' character.
 *
 * Returns: 0 on success, -1 if an error occured.
 */
int prelude_read_multiline(FILE *fd, unsigned int *line, char *buf, size_t size)
{
        size_t i, j, len;
        prelude_bool_t eol, has_data = FALSE, miss_eol=FALSE;

        while ( size > 1 ) {
                if ( ! fgets(buf, size, fd) )
                        return (has_data) ? 0 : prelude_error(PRELUDE_ERROR_EOF);

                len = strlen(buf);
                if ( ! len )
                        continue;

                eol = FALSE;
                for ( i = len - 1; isspace((int) buf[i]); i-- ) {

                        if ( buf[i] == '\n' || buf[i] == '\r' ) {
                                buf[i] = 0;
                                if ( ! eol ) {
                                        eol = TRUE;
                                        (*line)++;
                                }
                        }

                        if ( i == 0 )
                                break;
                }

                if ( miss_eol && eol && i == 0 )
                        continue;

                /*
                 * We don't want to handle multilines in case this is a comment.
                 */
                for ( j = 0; buf[j] != '\0' && isspace((int) buf[j]); j++ );
                if ( buf[j] == '#' )
                        continue;

                /*
                 * Multiline found, continue reading.
                 */
                if ( buf[i] != '\\' ) {
                        if ( eol )
                                return 0;

                        if ( len == size - 1 )
                                break;

                        has_data = TRUE;
                }

                if ( ! eol )
                        miss_eol = TRUE;

                buf += i;
                size -= i;
        }

        return prelude_error_verbose(PRELUDE_ERROR_EINVAL, "buffer is too small to store input line");
}



/**
 * prelude_read_multiline2:
 * @fd: File descriptor to read input from.
 * @line: Pointer to a line counter.
 * @out: Pointer to a #prelude_string_t object where the line should be stored.
 *
 * This function handles line reading separated by the '\' character.
 *
 * Returns: 0 on success, a negative value if an error occured.
 */
int prelude_read_multiline2(FILE *fd, unsigned int *line, prelude_string_t *out)
{
        int ret, r;
        char buf[8192];

        prelude_string_clear(out);

        do {
                ret = prelude_read_multiline(fd, line, buf, sizeof(buf));
                if ( ret < 0 && (r = prelude_error_get_code(ret)) != PRELUDE_ERROR_EINVAL ) {
                        if ( r == PRELUDE_ERROR_EOF && ! prelude_string_is_empty(out) )
                                ret = 0;

                        break;
                }

                r = prelude_string_cat(out, buf);
                if ( r < 0 )
                        return r;

        } while ( ret < 0 );

        return ret;
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

#ifdef PRELUDE_WORDS_BIGENDIAN
        tmp = val;
#else
        union {
                uint64_t val64;
                uint32_t val32[2];
        } combo_r, combo_w;

        combo_r.val64 = val;

        /*
         * Puts in network byte order
         */
        combo_w.val32[0] = htonl(combo_r.val32[1]);
        combo_w.val32[1] = htonl(combo_r.val32[0]);
        tmp = combo_w.val64;
#endif

        return tmp;
}


uint32_t prelude_htonf(float fval)
{
        union {
                float fval;
                uint32_t ival;
        } val;

        val.fval = fval;

        return htonl(val.ival);
}


static void normalize_path(char *path)
{
        int cnt;
        char *ptr, *end;

        while ( (ptr = strstr(path, "./")) ) {

                end = ptr + 2;

                if ( ptr == path || *(ptr - 1) != '.' ) {
                        memmove(ptr, end, strlen(end) + 1);
                        continue;
                }

                cnt = 0;
                while ( ptr != path ) {

                        if ( *(ptr - 1) == '/' && ++cnt == 2 )
                                break;

                        ptr--;
                }

                memmove(ptr == path ? ptr + 1 : ptr, end, strlen(end) + 1);
        }
}



int _prelude_get_file_name_and_path(const char *str, char **name, char **path)
{
        int ret = 0;
        char *ptr, pathname[PATH_MAX] = { 0 };

        ptr = strrchr(str, '/');
        if ( ! ptr ) {
                ret = find_absolute_path(_prelude_init_cwd, str, path);
                if ( ret < 0 )
                        return ret;

                *name = strdup(str);
                return (*name) ? 0 :  prelude_error_from_errno(errno);
        }

        if ( *str != '/' ) {
                char needsep = 0;
                size_t cwdlen = strlen(_prelude_init_cwd);

                if ( cwdlen )
                        needsep = (_prelude_init_cwd[cwdlen - 1] != '/' ) ? '/' : '\0';

                ret = snprintf(pathname, sizeof(pathname), "%s%c", _prelude_init_cwd, needsep);
                if ( ret < 0 || (size_t) ret >= sizeof(pathname) )
                        return prelude_error_from_errno(errno);
        }

        strncat(pathname, str, sizeof(pathname) - strlen(pathname));
        normalize_path(pathname);

        ret = access(pathname, F_OK);
        if ( ret < 0 )
                return prelude_error_from_errno(errno);

        ptr = strrchr(pathname, '/');

        *path = strndup(pathname, ptr - pathname);
        if ( ! *path )
                return prelude_error_from_errno(errno);

        *name = strdup(ptr + 1);
        if ( ! *name ) {
                free(*path);
                return prelude_error_from_errno(errno);
        }

        return 0;
}



int prelude_get_gmt_offset_from_time(const time_t *utc, long *gmtoff)
{
        time_t local;
        struct tm lt;

        if ( ! localtime_r(utc, &lt) )
                return prelude_error_from_errno(errno);

        local = timegm(&lt);

        *gmtoff = local - *utc;

        return 0;
}



int prelude_get_gmt_offset_from_tm(struct tm *tm, long *gmtoff)
{
        int tmp;
        time_t local, utc;

        /*
         * timegm will reset tm_isdst to 0
         */
        tmp = tm->tm_isdst;
        utc = timegm(tm);
        tm->tm_isdst = tmp;

        local = mktime(tm);
        if ( local == (time_t) -1 )
                return prelude_error_from_errno(errno);

        *gmtoff = utc - mktime(tm);

        return 0;
}



int prelude_get_gmt_offset(long *gmtoff)
{
        time_t t = time(NULL);
        return prelude_get_gmt_offset_from_time(&t, gmtoff);
}



time_t prelude_timegm(struct tm *tm)
{
        return timegm(tm);
}



void *prelude_sockaddr_get_inaddr(struct sockaddr *sa)
{
        void *ret = NULL;
        union {
                struct sockaddr *sa;
                struct sockaddr_in *sa4;
#ifdef HAVE_IPV6
                struct sockaddr_in6 *sa6;
#endif
        } val;

        val.sa = sa;
        if ( sa->sa_family == AF_INET )
                ret = &val.sa4->sin_addr;

#ifdef HAVE_IPV6
        else if ( sa->sa_family == AF_INET6 )
                ret = &val.sa6->sin6_addr;
#endif

        return ret;
}



int prelude_parse_address(const char *str, char **addr, unsigned int *port)
{
        char *input, *endptr = NULL;
        char *ptr, *port_ptr;

        ptr = strchr(str, '[');
        if ( ! ptr ) {
                input = strdup(str);
                port_ptr = input;
        }

        else {
                input = strdup(ptr + 1);

                ptr = strchr(input, ']');
                if ( ! ptr ) {
                        free(input);
                        return -1;
                }

                *ptr = 0;
                port_ptr = ptr + 1;
        }

        *addr = input;

        ptr = strrchr(port_ptr, ':');
        if ( ptr ) {
                *port = strtoul(ptr + 1, &endptr, 10);
                if ( endptr && *endptr != 0 ) {
                        free(input);
                        return -1;
                }

                *ptr = 0;
        }

        return 0;
}



/*
 * keep this function consistant with idmef_impact_severity_t value.
 */
prelude_msg_priority_t _idmef_impact_severity_to_msg_priority(idmef_impact_severity_t severity)
{
        static const prelude_msg_priority_t priority[] = {
                PRELUDE_MSG_PRIORITY_NONE, /* not bound                         */
                PRELUDE_MSG_PRIORITY_LOW,  /* IDMEF_IMPACT_SEVERITY_INFO   -> 1 */
                PRELUDE_MSG_PRIORITY_LOW,  /* IDMEF_IMPACT_SEVERITY_LOW    -> 2 */
                PRELUDE_MSG_PRIORITY_MID,  /* IDMEF_IMPACT_SEVERITY_MEDIUM -> 3 */
                PRELUDE_MSG_PRIORITY_HIGH, /* IDMEF_IMPACT_SEVERITY_HIGH   -> 4 */
        };

        if ( severity < 0 || (size_t) severity >= (sizeof(priority) / sizeof(*priority)) )
                return PRELUDE_MSG_PRIORITY_NONE;

        return priority[severity];
}



static int add_analyzer(prelude_client_t *client, void *top,
                        void *(*geta)(void *top, idmef_analyzer_t *analyzer),
                        int (*insa)(void *top, idmef_analyzer_t *analyzer, int pos))
{
        prelude_string_t *str;
        uint64_t wanted_analyzerid, analyzerid;
        idmef_analyzer_t *analyzer = NULL, *canalyzer;

        canalyzer = prelude_client_get_analyzer(client);
        wanted_analyzerid = prelude_client_profile_get_analyzerid(prelude_client_get_profile(client));

        while ( (analyzer = geta(top, analyzer)) && analyzer != canalyzer ) {
                str = idmef_analyzer_get_analyzerid(analyzer);
                if ( ! str )
                        continue;

                analyzerid = strtoull(prelude_string_get_string(str), NULL, 10);
                if ( analyzerid == wanted_analyzerid )
                        return 0;
        }

        return insa(top, idmef_analyzer_ref(prelude_client_get_analyzer(client)), IDMEF_LIST_PREPEND);
}


int _idmef_message_assign_missing(prelude_client_t *client, idmef_message_t *msg)
{
        idmef_alert_t *alert;
        idmef_heartbeat_t *heartbeat;
        prelude_ident_t *ident = prelude_client_get_unique_ident(client);

        if ( idmef_message_get_type(msg) == IDMEF_MESSAGE_TYPE_ALERT ) {
                alert = idmef_message_get_alert(msg);

                if ( ! idmef_alert_get_messageid(alert) )
                        idmef_alert_set_messageid(alert, get_message_ident(ident));

                add_analyzer(client, alert, (void *) idmef_alert_get_next_analyzer, (void *) idmef_alert_set_analyzer);
        } else {
                heartbeat = idmef_message_get_heartbeat(msg);

                if ( ! idmef_heartbeat_get_messageid(heartbeat) )
                        idmef_heartbeat_set_messageid(heartbeat, get_message_ident(ident));

                add_analyzer(client, heartbeat, (void *) idmef_heartbeat_get_next_analyzer, (void *)idmef_heartbeat_set_analyzer);
        }

        return 0;
}


int _prelude_load_file(const char *filename, unsigned char **fdata, size_t *outsize)
{
        int ret, fd;
        struct stat st;
        unsigned char *dataptr;

        fd = open(filename, O_RDONLY);
        if ( fd < 0 )
                return prelude_error_from_errno(errno);

        ret = fstat(fd, &st);
        if ( ret < 0 ) {
                close(fd);
                return prelude_error_from_errno(errno);
        }

        if ( st.st_size == 0 ) {
                close(fd);
                return prelude_error_verbose(prelude_error_code_from_errno(EINVAL), "could not load '%s': empty file", filename);
        }

        *outsize = st.st_size;

#if !((defined _WIN32 || defined __WIN32__) && !defined __CYGWIN__)
        dataptr = *fdata = mmap(NULL, st.st_size, PROT_READ, MAP_SHARED, fd, 0);
        if ( dataptr == MAP_FAILED ) {
                close(fd);
                return prelude_error_from_errno(errno);
        }
#else
        dataptr = *fdata = malloc(st.st_size);
        if ( ! dataptr ) {
                close(fd);
                return prelude_error_from_errno(errno);
        }

        _setmode(fd, O_BINARY);

        do {
                ssize_t len;

                len = read(fd, dataptr, st.st_size);
                if ( len < 0 ) {
                        if ( errno == EINTR )
                                continue;

                        close(fd);
                        free(*fdata);

                        return prelude_error_from_errno(errno);
                }

                dataptr += len;
                st.st_size -= len;
        } while ( st.st_size > 0 );

#endif
        close(fd);

        return 0;
}


void _prelude_unload_file(unsigned char *fdata, size_t size)
{
#if !((defined _WIN32 || defined __WIN32__) && !defined __CYGWIN__)
        munmap(fdata, size);
#else
        free(fdata);
#endif
}
