/**********************************************************************
* Copyright (c) David L. Mills 1992-2001                              *
*                                                                     *
* Permission to use, copy, modify, and distribute this software and   *
* its documentation for any purpose and without fee is hereby         *
* granted, provided that the above copyright notice appears in all    *
* copies and that both the copyright notice and this permission       *
* notice appear in supporting documentation, and that the name        *
* University of Delaware not be used in advertising or publicity      *
* pertaining to distribution of the software without specific,        *
* written prior permission. The University of Delaware makes no       *
* representations about the suitability this software for any         *
* purpose. It is provided "as is" without express or implied          *
* warranty.                                                           *
*                                                                     *
***********************************************************************/

#ifndef _LIBPRELUDE_NTP_H
#define _LIBPRELUDE_NTP_H

#define TS_MASK         0xfffff000      /* mask to usec, for time stamps */
#define TS_ROUNDBIT     0x00000800      /* round at this bit */
#define	JAN_1970	0x83aa7e80	/* 2208988800 1970 - 1900 in seconds */


typedef int32_t s_fp;


typedef struct {
        union {
                uint32_t Xl_ui;
                int32_t Xl_i;
        } Ul_i;
        union {
                uint32_t Xl_uf;
                int32_t Xl_f;
        } Ul_f;
} l_fp;

#define l_ui    Ul_i.Xl_ui              /* unsigned integral part */
#define l_i     Ul_i.Xl_i               /* signed integral part */
#define l_uf    Ul_f.Xl_uf              /* unsigned fractional part */
#define l_f     Ul_f.Xl_f               /* signed fractional part */



extern const unsigned long ustotslo[];
extern const unsigned long ustotsmid[];
extern const unsigned long ustotshi[];


#define M_NEG(v_i, v_f)         /* v = -v */ \
        do { \
                if ((v_f) == 0) \
                        (v_i) = -((s_fp)(v_i)); \
                else { \
                        (v_f) = -((s_fp)(v_f)); \
                        (v_i) = ~(v_i); \
                } \
        } while(0)



#define L_NEG(v)        M_NEG((v)->l_ui, (v)->l_uf)


#define TVUTOTSF(tvu, tsf) \
        (tsf) = ustotslo[(tvu) & 0xff] \
            + ustotsmid[((tvu) >> 8) & 0xff] \
            + ustotshi[((tvu) >> 16) & 0xf]


#define sTVTOTS(tv, ts) \
        do { \
                int isneg = 0; \
                long usec; \
                (ts)->l_ui = (tv)->tv_sec; \
                usec = (tv)->tv_usec; \
                if (((tv)->tv_sec < 0) || ((tv)->tv_usec < 0)) { \
                        usec = -usec; \
                        (ts)->l_ui = -(ts)->l_ui; \
                        isneg = 1; \
                } \
                TVUTOTSF(usec, (ts)->l_uf); \
                if (isneg) { \
                        L_NEG((ts)); \
                } \
        } while(0)

/* Taken from /usr/src/contrib/ntp/include/ntp_unixtime.h, FreeBSD 5.1-RELEASE */

/*
 * TV_SHIFT is used to turn the table result into a usec value.  To round,
 * add in TV_ROUNDBIT before shifting
 */
#define TV_SHIFT        3
#define TV_ROUNDBIT     0x4
    

/*
 * Convert a time stamp fraction to microseconds.  The time stamp
 * fraction is assumed to be unsigned.  To use this in a program, declare:
 */
extern const long tstouslo[];
extern const long tstousmid[];
extern const long tstoushi[];

#define TSFTOTVU(tsf, tvu) \
        (tvu) = (tstoushi[((tsf) >> 24) & 0xff] \
            + tstousmid[((tsf) >> 16) & 0xff] \
            + tstouslo[((tsf) >> 9) & 0x7f] \
            + TV_ROUNDBIT) >> TV_SHIFT
/*
 * Convert a time stamp to a struct timeval.  The time stamp
 * has to be positive.
 */
#define TSTOTV(ts, tv) \
        do { \
                (tv)->tv_sec = (ts)->l_ui; \
                TSFTOTVU((ts)->l_uf, (tv)->tv_usec); \
                if ((tv)->tv_usec == 1000000) { \
                        (tv)->tv_sec++; \
                        (tv)->tv_usec = 0; \
                } \
        } while (0)



#endif /* _LIBPRELUDE_NTP_H */
