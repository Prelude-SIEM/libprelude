/*****
*
* Copyright (C) 2002, 2003, 2004 Yoann Vandoorselaere <yoann@mandrakesoft.com>
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

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <limits.h>
#include <sys/time.h>
#include <netinet/in.h> /* for extract.h */
#include <string.h>
#include <stdarg.h>

#include "libmissing.h"
#include "prelude-list.h"
#include "prelude-log.h"
#include "prelude-inttypes.h"
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

#include "ntp.h"
#include "idmef-util.h"


const unsigned char *idmef_additionaldata_data_to_string(idmef_additional_data_t *ad,
                                                         unsigned char *buf, size_t *size)
{
        int ret = 0;
        idmef_data_t *data;
        const char *outstr;

        data = idmef_additional_data_get_data(ad);
        if ( idmef_data_is_empty(data) )
                return "";

        switch ( idmef_additional_data_get_type(ad) ) {

        case IDMEF_ADDITIONAL_DATA_TYPE_BYTE:
        case IDMEF_ADDITIONAL_DATA_TYPE_CHARACTER:
                *size = idmef_data_get_len(data);
                return idmef_data_get_data(data);

        case IDMEF_ADDITIONAL_DATA_TYPE_BOOLEAN:
        case IDMEF_ADDITIONAL_DATA_TYPE_DATE_TIME:
        case IDMEF_ADDITIONAL_DATA_TYPE_PORTLIST:
        case IDMEF_ADDITIONAL_DATA_TYPE_STRING:
        case IDMEF_ADDITIONAL_DATA_TYPE_XML:
                ret = extract_characters_safe(&outstr, idmef_data_get_data(data), idmef_data_get_len(data));
                *size = idmef_data_get_len(data) - 1; /* 0 string delimiter is included in len */
                return (ret < 0) ? NULL : outstr;

        case IDMEF_ADDITIONAL_DATA_TYPE_INTEGER: {
                uint32_t out32;

                ret = extract_uint32_safe(&out32, idmef_data_get_data(data), idmef_data_get_len(data));
                if ( ret < 0 )
                        return NULL;

                ret = snprintf(buf, *size, "%u", out32);
                break;
        }

        case IDMEF_ADDITIONAL_DATA_TYPE_NTPSTAMP: {
                union {
                        uint64_t w_buf;
                        uint32_t r_buf[2];
                } d;

                ret = extract_uint64_safe(&d.w_buf, idmef_data_get_data(data), idmef_data_get_len(data));
                if ( ret < 0 )
                        return NULL;
                
                ret = snprintf(buf, *size, "0x%08ux.0x%08ux", d.r_buf[0], d.r_buf[1]);
                break;
        }

        case IDMEF_ADDITIONAL_DATA_TYPE_REAL: {
		uint32_t out32;

                ret = extract_uint32_safe(&out32, idmef_data_get_data(data), idmef_data_get_len(data));
                if ( ret < 0 )
                        return NULL;
                
                ret = snprintf(buf, *size, "%f", (float) out32);
                break;
        }

        default:
                log(LOG_ERR, "Unknown data type: %d.\n", idmef_additional_data_get_type(ad));
                return NULL;
        }

        if ( ret < 0 || ret >= *size )
                return NULL;

        *size = ret;

        return buf;
}


