#ifndef TIME_H
#define TIME_H

#include <types.h>

typedef LONGLONG time_t;

struct tm
{
    INT tm_sec;     /* seconds after the minute — [0, 60] */
    INT tm_min;     /* minutes after the hour — [0, 59]   */
    INT tm_hour;    /* hours since midnight — [0, 23]     */
    INT tm_mday;    /* day of the month — [1, 31]         */
    INT tm_mon;     /* months since January — [0, 11]     */
    INT tm_year;    /* years since 1900                   */
    INT tm_wday;    /* days since Sunday — [0, 6]         */
    INT tm_yday;    /* days since January 1 — [0, 365]    */
    INT tm_isdst;   /* Daylight Saving Time flag          */
};

time_t time( time_t* timer );
time_t difftime( time_t time1, time_t time0 );

#endif
