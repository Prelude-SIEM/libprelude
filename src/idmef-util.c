/*****
*
* Copyright (C) 2002, 2003 Yoann Vandoorselaere <yoann@mandrakesoft.com>
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
#include <time.h>
#include <unistd.h>
#include <limits.h>
#include <sys/time.h>
#include <inttypes.h>
#include <netinet/in.h> /* for extract.h */
#include <string.h>
#include <stdarg.h>

#include "list.h"
#include "prelude-log.h"
#include "extract.h"
#include "idmef-tree.h"
#include "prelude-ident.h"

#include "idmef-string.h"
#include "idmef-time.h"
#include "idmef-data.h"
#include "idmef-type.h"
#include "idmef-value.h"
#include "idmef-tree.h"
#include "idmef-tree-wrap.h"
#include "prelude-strbuf.h"


#include "config.h"
#include "ntp.h"
#include "idmef-util.h"


/*
 * Some of the code here was inspired from libidmef code.
 */







/**
 * idmef_additionaldata_data_to_string:
 * @ad: An additional data object.
 *
 * This function take care of converting the IDMEF AdditionalData data
 * member to a string suitable to be outputed in the IDMEF database.
 *
 * Returns: NULL on error, string written otherwise
 *
 */
const char *idmef_additionaldata_data_to_string(idmef_additional_data_t *ad)
{
	static char buffer[128];
	idmef_data_t *data;
        int retval = 0;

	data = idmef_additional_data_get_data(ad);
	if ( idmef_data_is_empty(data) )
		return "";

        switch ( idmef_additional_data_get_type(ad) ) {

        case byte:
                /*
                 * FIXME:
                 *
                 * from section 4.3.2.2 of the IDMEF specs:
                 *
                 * Any character defined by the ISO/IEC 10646 and Unicode standards may
                 * be included in an XML document by the use of a character reference.
                 *
                 * A character reference is started with the characters '&' and '#', and
                 * ended with the character ';'.  Between these characters, the
                 * character code for the character inserted.
                 *
                 * If the character code is preceded by an 'x' it is interpreted in
                 * hexadecimal (base 16), otherwise, it is interpreted in decimal (base
                 * 10).  For instance, the ampersand (&) is encoded as &#38; or &#x0026;
                 * and the less-than sign (<) is encoded as &#60; or &#x003C;.
                 *
                 * Any one-, two-, or four-byte character specified in the ISO/IEC 10646
                 * and Unicode standards can be included in a document using this
                 * technique.
                 */
                break;

        case character:
                retval = snprintf(buffer, sizeof (buffer), "%c", *(const char *) idmef_data_get_data(data));
                break;

        case integer: {
		uint32_t out32;

                retval = extract_uint32_safe(&out32, idmef_data_get_data(data), idmef_data_get_len(data));
                if ( retval < 0 )
                        return NULL;
                
                retval = snprintf(buffer, sizeof (buffer), "%u", out32);
                break;
	}
                
        case ntpstamp: {
                union {
                        uint64_t w_buf;
                        uint32_t r_buf[2];
                } d;

                retval = extract_uint64_safe(&d.w_buf, idmef_data_get_data(data), idmef_data_get_len(data));
                if ( retval < 0 )
                        return NULL;
                
                retval = snprintf(buffer, sizeof (buffer), "0x%08ux.0x%08ux", d.r_buf[0], d.r_buf[1]);
                break;
	}

        case real: {
		uint32_t out32;

                retval = extract_uint32_safe(&out32, idmef_data_get_data(data), idmef_data_get_len(data));
                if ( retval < 0 )
                        return NULL;
                
                retval = snprintf(buffer, sizeof (buffer), "%f", (float) out32);
                break;
	}

        case boolean:
        case date_time:
        case portlist:
        case string:
        case xml: {
		const char *ptr;

                retval = extract_characters_safe(&ptr, idmef_data_get_data(data), idmef_data_get_len(data));

		return (retval < 0) ? NULL : ptr;
	}

        default:
                log(LOG_ERR, "Unknown data type: %d.\n", idmef_additional_data_get_type(ad));
                return NULL;
        }

        /*
         * Since glibc 2.1 snprintf follow the C99 standard and return
         * the number of characters (excluding the trailing '\0') which
         * would have been written to the final string if enought space
         * had been available.
         */

	return (retval < 0 || retval >= sizeof (buffer)) ? NULL : buffer;
}



int prelude_get_process_name_and_path(const char *str, char **name, char **path)
{
	char cwd[PATH_MAX];
	prelude_strbuf_t *strbuf = NULL;
	char *ptr;

	*name = NULL;
	*path = NULL;

	ptr = strrchr(str, '/');
	if ( ptr ) {
		*name = strdup(ptr + 1);
		if ( ! *name ) {
			log(LOG_ERR, "memory exhausted.\n");
			return -1;
		}

		strbuf = prelude_strbuf_new();
		if ( ! strbuf )
			goto error;

		if ( *str != '/' ) {
			if ( getcwd(cwd, sizeof (cwd)) < 0 ) {
				free(*name);
				return -1;
			}

			if ( prelude_strbuf_sprintf(strbuf, "%s/", cwd) < 0 )
				goto error;
		}

		if ( prelude_strbuf_ncat(strbuf, str, (ptr == str) ? 1 : (ptr - str)) < 0 )
			goto error;

		prelude_strbuf_dont_own(strbuf);
		*path = prelude_strbuf_get_string(strbuf);
		prelude_strbuf_destroy(strbuf);

		return 2;

	}

	*name = strdup(str);
	if ( ! *name ) {
		log(LOG_ERR, "memory exhausted.\n");
		return -1;
	}

	return 1;

 error:
	if ( *name )
		free(*name);

	if ( *path )
		free(*path);

	if ( strbuf )
		prelude_strbuf_destroy(strbuf);

	return -1;
}



int prelude_get_gmt_offset(int *gmt_offset)
{
        struct tm tm_utc;
        time_t time_utc;
	time_t time_local;

	time(&time_local);

        if ( ! gmtime_r(&time_local, &tm_utc) ) {
                log(LOG_ERR, "error converting local time to utc time.\n");
                return -1;
        }

        tm_utc.tm_isdst = -1;
        time_utc = mktime(&tm_utc);
        *gmt_offset = time_local - time_utc;

        return 0;
}
