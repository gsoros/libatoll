#ifndef __atoll_time_h
#define __atoll_time_h

#include <sys/time.h>
#include "time.h"

#include "atoll_log.h"

// workarounds for intellisense identifier “setenv” is undefined and identifier “tzset” is undefined
_VOID _EXFUN(tzset, (_VOID));
int _EXFUN(setenv, (const char *__string, const char *__value, int __overwrite));

namespace Atoll {

// TODO move these into a static class

inline time_t now() {
    time_t t;
    time(&t);
    return t;
}

inline tm gmTm() {
    time_t t = now();
    struct tm tm;
    gmtime_r(&t, &tm);
    return tm;
}

inline tm localTm() {
    time_t t = now();
    struct tm tm;
    localtime_r(&t, &tm);
    return tm;
}

inline void localTmStr(char *buf, size_t len) {
    tm time = localTm();
    strftime(buf, len, "%c (%z %Z)", &time);
}

inline void setTimezone(const char *tz) {
    // log_i("new timezone: %s", tz);
    setenv("TZ", tz, 1);
    tzset();
}

inline ulong systemTimeLastSet(ulong set = 0) {
    static ulong lastSet = 0;
    if (set)
        lastSet = set;
    return lastSet;
}

inline void setSytemTimeFromUtc(
    uint16_t year,
    uint8_t month,
    uint8_t day,
    uint8_t hour,
    uint8_t minute,
    uint8_t second,
    uint8_t centisecond) {
    // struct tm:
    // Member	Type	Meaning	                    Range
    // tm_sec	int	    seconds after the minute	0-60*
    // tm_min	int 	minutes after the hour	    0-59
    // tm_hour	int	    hours since midnight	    0-23
    // tm_mday	int 	day of the month	        1-31
    // tm_mon	int	    months since January	    0-11
    // tm_year	int 	years since 1900
    // tm_wday	int	    days since Sunday	        0-6
    // tm_yday	int 	days since January 1	    0-365
    // tm_isdst	int 	Daylight Saving Time flag
    struct tm tm;

    tm.tm_year = year - 1900;
    tm.tm_mon = month - 1;
    tm.tm_mday = day;
    tm.tm_hour = hour;
    tm.tm_min = minute;
    tm.tm_sec = second;
    tm.tm_isdst = 0;
    char tz[32] = "";
    strncpy(tz, getenv("TZ"), sizeof(tz));
    setTimezone("UTC0");
    timeval tv = {mktime(&tm), centisecond * 10000};
    timezone utc = {0, 0};
    settimeofday(&tv, &utc);
    setTimezone(tz);

    char buf[64];
    localTmStr(buf, sizeof(buf));
    log_i("system time set: %s", buf);

    systemTimeLastSet(millis());
}

}  // namespace Atoll

#endif