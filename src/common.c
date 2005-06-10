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
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <errno.h>
#include <time.h>

#include "prelude-error.h"
#include "libmissing.h"

#include "idmef.h"
#include "prelude-log.h"
#include "common.h"



static prelude_string_t *get_message_ident(prelude_ident_t *ident)
{
        int ret;
        prelude_string_t *str;

        ret = prelude_string_new(&str);
        if ( ret < 0 )
                return NULL;

        ret = prelude_string_sprintf(str, "%" PRELUDE_PRIu64, prelude_ident_inc(ident));
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
 *
 * Returns: a pointer to the newly allocated memory.
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
        size_t i;

        if ( ! fgets(buf, size, fd) )
                return -1;

        (*line)++;
         
        /*
         * We don't want to handle multilines in case this is a comment.
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



int _prelude_get_file_name_and_path(const char *str, char **name, char **path)
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

                return _prelude_get_file_name_and_path(buf, name, path);
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
        
        if ( sa->sa_family == AF_INET )
                ret = &((struct sockaddr_in *) sa)->sin_addr;

#ifdef HAVE_IPV6
        else if ( sa->sa_family == AF_INET6 )
                ret = &((struct sockaddr_in6 *) sa)->sin6_addr;
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
                PRELUDE_MSG_PRIORITY_LOW,  /* IDMEF_IMPACT_SEVERITY_LOW    -> 1 */
                PRELUDE_MSG_PRIORITY_MID,  /* IDMEF_IMPACT_SEVERITY_MEDIUM -> 2 */
                PRELUDE_MSG_PRIORITY_HIGH, /* IDMEF_IMPACT_SEVERITY_HIGH   -> 3 */
                PRELUDE_MSG_PRIORITY_LOW   /* IDMEF_IMPACT_SEVERITY_INFO   -> 4 */
        };
                
        if ( severity >= (sizeof(priority) / sizeof(*priority)) )
                return PRELUDE_MSG_PRIORITY_NONE;
        
        return priority[severity];
}



int _idmef_message_assign_messageid(idmef_message_t *msg, prelude_ident_t *ident)
{
        idmef_alert_t *alert;
        idmef_heartbeat_t *heartbeat;
        
        if ( idmef_message_get_type(msg) == IDMEF_MESSAGE_TYPE_ALERT ) {
                alert = idmef_message_get_alert(msg);
                
                if ( ! idmef_alert_get_messageid(alert) )
                        idmef_alert_set_messageid(alert, get_message_ident(ident));
        } else {       
                heartbeat = idmef_message_get_heartbeat(msg);

                if ( ! idmef_heartbeat_get_messageid(heartbeat) )
                        idmef_heartbeat_set_messageid(heartbeat, get_message_ident(ident));
        }

        return 0;
}
