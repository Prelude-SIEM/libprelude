/*****
*
* Copyright (C) 2001,2002,2003,2004,2005 PreludeIDS Technologies. All Rights Reserved.
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
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>

#if TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# if HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif

#include <gcrypt.h>

#include "prelude-log.h"
#include "prelude-error.h"
#include "prelude-inttypes.h"
#include "prelude-ident.h"


/*
 * Partial (no state, no node since the messageid is local to an
 * analyzer) UUIDv1 implementation.
 *
 * Based on RFC4122 reference implementation.
 */


/*
 * This is the number of UUID the system is capable of generating
 * within one resolution of our system clock (the lower the better).
 *
 * - UUID has 100ns resolution, so one UUID every 100ns max (RFC 4122).
 * - gettimeofday() has microsecond resolution (1000ns).
 * - We can generate 1000 / 100 = 10 UUID per tick.
 */
#define UUIDS_PER_TICK 10


/*
 * Time offset between UUID and Unix Epoch time according to standards.
 * UUID UTC base time is October 15, 1582 - Unix UTC base time is
 * January  1, 1970)
 */
#define UUID_TIMEOFFSET 0x01b21dd213814000


struct prelude_ident {
        uint16_t tick;
        uint64_t last;
        uint16_t clockseq;

        struct {
                uint32_t time_low;                  /* bits  0-31 of time field */
                uint16_t time_mid;                  /* bits 32-47 of time field */
                uint16_t time_hi_and_version;       /* bits 48-59 of time field plus 4 bit version */
                uint8_t  clock_seq_hi_and_reserved; /* bits  8-13 of clock sequence field plus 2 bit variant */
                uint8_t  clock_seq_low;             /* bits  0-7  of clock sequence field */
        } uuid;
};



/*
 * Return the system time as 100ns ticks since UUID epoch.
 */
static uint64_t get_system_time(void)
{
        struct timeval tv;

        gettimeofday(&tv, NULL);

        return ((uint64_t) tv.tv_sec  * 10000000) +
               ((uint64_t) tv.tv_usec * 10) +
               UUID_TIMEOFFSET;
}


static uint64_t get_current_time(prelude_ident_t *ident)
{
        uint64_t now;

        do {
                now = get_system_time();

                /*
                 * if clock value changed since the last UUID generated
                 */
                if ( ident->last != now ) {
                        ident->last = now;
                        ident->tick = 0;
                        break;
                }

                if ( ident->tick < UUIDS_PER_TICK ) {
                        ident->tick++;
                        break;
                }

                /*
                 * We're generating ident faster than our clock resolution
                 * can afford: spin.
                 */
        } while ( 1 );

        /*
         * add the count of uuids to low order bits of the clock reading
         */
        return now + ident->tick;
}



static void uuidgen(prelude_ident_t *ident)
{
        uint64_t timestamp;

        timestamp = get_current_time(ident);
        if ( timestamp < ident->last )
                ident->clockseq++;

        ident->uuid.time_low = (uint32_t) (timestamp & 0xffffffff);
        ident->uuid.time_mid = (uint16_t) ((timestamp >> 32) & 0xffff);
        ident->uuid.time_hi_and_version = (uint16_t) ((timestamp >> 48) & 0x0fff);
        ident->uuid.time_hi_and_version |= (1 << 12);

        ident->uuid.clock_seq_low = ident->clockseq & 0xff;
        ident->uuid.clock_seq_hi_and_reserved = (ident->clockseq & 0x3f00) >> 8;
        ident->uuid.clock_seq_hi_and_reserved |= 0x80;
}



/**
 * prelude_ident_new:
 * @ret: Pointer where to store the created object.
 *
 * Create a new #prelude_ident_t object with an unique value.
 *
 * Returns: 0 on success, or a negative value if an error occured.
 */
int prelude_ident_new(prelude_ident_t **ret)
{
        prelude_ident_t *new;

        *ret = new = malloc(sizeof(*new));
        if ( ! new )
                return prelude_error_from_errno(errno);

        new->last = 0;
        new->tick = 0;
        gcry_randomize(&new->clockseq, sizeof(new->clockseq), GCRY_STRONG_RANDOM);

        return 0;
}




/**
 * prelude_ident_generate:
 * @ident: Pointer to a #prelude_ident_t object.
 * @out: #prelude_string_t where the ident will be generated.
 *
 * Generate an UUID and store it in @out.
 *
 * Returns: A negative value if an error occur.
 */
int prelude_ident_generate(prelude_ident_t *ident, prelude_string_t *out)
{
        uuidgen(ident);

        return prelude_string_sprintf(out, "%8.8x-%4.4x-%4.4x-%2.2x%2.2x",
                                      ident->uuid.time_low, ident->uuid.time_mid,
                                      ident->uuid.time_hi_and_version,
                                      ident->uuid.clock_seq_hi_and_reserved,
                                      ident->uuid.clock_seq_low);
}


/**
 * prelude_ident_inc:
 * @ident: Pointer to a #prelude_ident_t object.
 *
 * Deprecated.
 *
 * Returns: A new ident.
 */
uint64_t prelude_ident_inc(prelude_ident_t *ident)
{
        return get_system_time();
}


/**
 * prelude_ident_destroy:
 * @ident: Pointer to a #prelude_ident_t object.
 *
 * Destroy a #prelude_ident_t object.
 */
void prelude_ident_destroy(prelude_ident_t *ident)
{
        free(ident);
}
