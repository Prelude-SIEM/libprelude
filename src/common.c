/*****
*
* Copyright (C) 2002-2017 CS-SI. All Rights Reserved.
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
* You should have received a copy of the GNU General Public License along
* with this program; if not, write to the Free Software Foundation, Inc.,
* 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
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
        int ret;
        prelude_string_t *str;
        idmef_analyzer_t *analyzer = NULL;
        uint64_t wanted_analyzerid, analyzerid;

        wanted_analyzerid = prelude_client_profile_get_analyzerid(prelude_client_get_profile(client));

        while ( (analyzer = geta(top, analyzer)) ) {
                str = idmef_analyzer_get_analyzerid(analyzer);
                if ( ! str )
                        continue;

                analyzerid = strtoull(prelude_string_get_string(str), NULL, 10);
                if ( analyzerid == wanted_analyzerid )
                        return 0;
        }

        ret = idmef_analyzer_clone(prelude_client_get_analyzer(client), &analyzer);
        if ( ret < 0 )
                return ret;

        return insa(top, analyzer, IDMEF_LIST_PREPEND);
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


double prelude_simple_strtod(const char *s, char **endptr)
{
        double neg = 1, ret = 0, fp = 1.0;
        int got_point = 0, d;

        if ( *s == '-' ) {
                neg = -1;
                s++;
        }

        for ( ; *s; s++) {
                if ( *s == '.' ) {
                        got_point = 1;
                        continue;
                }

                d = *s - '0';
                if ( d < 0 || d > 9 )
                        break;

                else if ( ! got_point )
                        ret = (10 * ret) + d;

                else {
                        fp *= 0.1;
                        ret += fp * d;
                }
        }

        if ( endptr )
                *endptr = s;

        return ret * neg;
}


/*
 * Table of CRCs of all 8-bit messages. Generated by running code
 * from RFC 1952 modified to print out the table.
 */
static uint32_t crc32_tab[] = {
    0x00000000, 0x77073096, 0xee0e612c, 0x990951ba, 0x076dc419, 0x706af48f,
    0xe963a535, 0x9e6495a3, 0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988,
    0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91, 0x1db71064, 0x6ab020f2,
    0xf3b97148, 0x84be41de, 0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7,
    0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec, 0x14015c4f, 0x63066cd9,
    0xfa0f3d63, 0x8d080df5, 0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172,
    0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b, 0x35b5a8fa, 0x42b2986c,
    0xdbbbc9d6, 0xacbcf940, 0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59,
    0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116, 0x21b4f4b5, 0x56b3c423,
    0xcfba9599, 0xb8bda50f, 0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924,
    0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d, 0x76dc4190, 0x01db7106,
    0x98d220bc, 0xefd5102a, 0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433,
    0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818, 0x7f6a0dbb, 0x086d3d2d,
    0x91646c97, 0xe6635c01, 0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e,
    0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457, 0x65b0d9c6, 0x12b7e950,
    0x8bbeb8ea, 0xfcb9887c, 0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65,
    0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2, 0x4adfa541, 0x3dd895d7,
    0xa4d1c46d, 0xd3d6f4fb, 0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0,
    0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9, 0x5005713c, 0x270241aa,
    0xbe0b1010, 0xc90c2086, 0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
    0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4, 0x59b33d17, 0x2eb40d81,
    0xb7bd5c3b, 0xc0ba6cad, 0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a,
    0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683, 0xe3630b12, 0x94643b84,
    0x0d6d6a3e, 0x7a6a5aa8, 0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1,
    0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe, 0xf762575d, 0x806567cb,
    0x196c3671, 0x6e6b06e7, 0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc,
    0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5, 0xd6d6a3e8, 0xa1d1937e,
    0x38d8c2c4, 0x4fdff252, 0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b,
    0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60, 0xdf60efc3, 0xa867df55,
    0x316e8eef, 0x4669be79, 0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236,
    0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f, 0xc5ba3bbe, 0xb2bd0b28,
    0x2bb45a92, 0x5cb36a04, 0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d,
    0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a, 0x9c0906a9, 0xeb0e363f,
    0x72076785, 0x05005713, 0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38,
    0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21, 0x86d3d2d4, 0xf1d4e242,
    0x68ddb3f8, 0x1fda836e, 0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777,
    0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c, 0x8f659eff, 0xf862ae69,
    0x616bffd3, 0x166ccf45, 0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2,
    0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db, 0xaed16a4a, 0xd9d65adc,
    0x40df0b66, 0x37d83bf0, 0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
    0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6, 0xbad03605, 0xcdd70693,
    0x54de5729, 0x23d967bf, 0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94,
    0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d
};



/*
 * The following function was modified from RFC 1952.
 *
 *    Copyright (c) 1996 L. Peter Deutsch
 *
 *    Permission is granted to copy and distribute this document for
 *    any purpose and without charge, including translations into
 *    other languages and incorporation into compilations, provided
 *    that the copyright notice and this notice are preserved, and
 *    that any substantive changes or deletions from the original are
 *    clearly marked.
 *
 * The copyright on RFCs, and consequently the function below, are
 * supposedly also retroactively claimed by the Internet Society
 * (according to rfc-editor@rfc-editor.org), with the following
 * copyright notice:
 *
 *    Copyright (C) The Internet Society.  All Rights Reserved.
 *
 *    This document and translations of it may be copied and furnished
 *    to others, and derivative works that comment on or otherwise
 *    explain it or assist in its implementation may be prepared,
 *    copied, published and distributed, in whole or in part, without
 *    restriction of any kind, provided that the above copyright
 *    notice and this paragraph are included on all such copies and
 *    derivative works.  However, this document itself may not be
 *    modified in any way, such as by removing the copyright notice or
 *    references to the Internet Society or other Internet
 *    organizations, except as needed for the purpose of developing
 *    Internet standards in which case the procedures for copyrights
 *    defined in the Internet Standards process must be followed, or
 *    as required to translate it into languages other than English.
 *
 *    The limited permissions granted above are perpetual and will not be
 *    revoked by the Internet Society or its successors or assigns.
 *
 *    This document and the information contained herein is provided
 *    on an "AS IS" basis and THE INTERNET SOCIETY AND THE INTERNET
 *    ENGINEERING TASK FORCE DISCLAIMS ALL WARRANTIES, EXPRESS OR
 *    IMPLIED, INCLUDING BUT NOT LIMITED TO ANY WARRANTY THAT THE USE
 *    OF THE INFORMATION HEREIN WILL NOT INFRINGE ANY RIGHTS OR ANY
 *    IMPLIED WARRANTIES OF MERCHANTABILITY OR FITNESS FOR A
 *    PARTICULAR PURPOSE.
 *
 */
uint32_t prelude_crc32(const unsigned char *data, size_t size)
{
        uint32_t crc = 0 ^ 0xffffffffL;

        while ( size-- )
                crc = crc32_tab[(crc ^ *data++) & 0xff] ^ (crc >> 8);

        return crc ^ 0xffffffffL;
}

