#include <bios.h>
#include <time.h>

time_t difftime( time_t time1, time_t time0 )
{
    return time1 - time0;
}

time_t time( time_t* timer )
{
    USHORT days[] = {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334 };
    struct tm t1;
    struct tm t2;
    REGS   regs;
    time_t t;

    /* Get time/date t1 */
    regs.h.ah = 4;
    int86( 0x1A, &regs, &regs );
    t1.tm_year = (regs.h.ch >> 4) * 10 + (regs.h.ch & 0xF);
    t1.tm_year = (regs.h.cl >> 4) * 10 + (regs.h.cl & 0xF) + t1.tm_year * 100;
    t1.tm_mon  = (regs.h.dh >> 4) * 10 + (regs.h.dh & 0xF) - 1;
    t1.tm_mday = (regs.h.dl >> 4) * 10 + (regs.h.dl & 0xF) - 1;
    regs.h.ah = 2;
    int86( 0x1A, &regs, &regs );
    t1.tm_hour = (regs.h.ch >> 4) * 10 + (regs.h.ch & 0xF);
    t1.tm_min  = (regs.h.cl >> 4) * 10 + (regs.h.cl & 0xF);
    t1.tm_sec  = (regs.h.dh >> 4) * 10 + (regs.h.dh & 0xF);

    /* Get time/date t2 */
    regs.h.ah = 4;
    int86( 0x1A, &regs, &regs );
    t2.tm_year = (regs.h.ch >> 4) * 10 + (regs.h.ch & 0xF);
    t2.tm_year = (regs.h.cl >> 4) * 10 + (regs.h.cl & 0xF) + t2.tm_year * 100;
    t2.tm_mon  = (regs.h.dh >> 4) * 10 + (regs.h.dh & 0xF) - 1;
    t2.tm_mday = (regs.h.dl >> 4) * 10 + (regs.h.dl & 0xF) - 1;
    regs.h.ah = 2;
    int86( 0x1A, &regs, &regs );
    t2.tm_hour = (regs.h.ch >> 4) * 10 + (regs.h.ch & 0xF);
    t2.tm_min  = (regs.h.cl >> 4) * 10 + (regs.h.cl & 0xF);
    t2.tm_sec  = (regs.h.dh >> 4) * 10 + (regs.h.dh & 0xF);

    if ((t1.tm_year != t2.tm_year) || (t1.tm_mon != t2.tm_mon) || (t1.tm_mday != t2.tm_mday))
    {
        /* Use t2 instead of t1, since t1 might be wrong (instead of just old) */
        t1 = t2;
    }

    /*
     * This is according to the Gregorian calendar which states that there are
     * 146,097 days in 400 years (avg 365.2425 days/year).
     */

    /* We start from the first multiple of 400 years before 1980 */
    if (t1.tm_year < 1601)
    {
        return (time_t)(-1);
    }
    t1.tm_year -= 1600;

    /* Assume 365 days per year */
    t = t1.tm_year * 365;

    /* Add the number of leap years before the current year since 1600 */
    t += (t1.tm_year-1)/4 - (t1.tm_year-1)/100 + (t1.tm_year-1)/400 + 1;

    /* Subtract the number of days before 1970 (since 1600) */
    t -= 135140;

    /* Add the days in this year */
    t += days[ t1.tm_mon ];

    if ((t1.tm_mon > 1) && (t1.tm_year % 4 == 0) && ((t1.tm_year % 100 != 0) || (t1.tm_year % 400 == 0)))
    {
        /* We're in a leap year, past february, so add a day */
        t++;
    }
    t += t1.tm_mday;

    /* t is now the number of days since Jan 1st, 1970 */
    t = t * 24 + t1.tm_hour;
    t = t * 60 + t1.tm_min;
    t = t * 60 + t1.tm_sec;
    /* t is now the number of seconds since Jan 1st, 1970 */

    if (timer != NULL) *timer = t;
    return t;
}

