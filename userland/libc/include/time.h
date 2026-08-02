#ifndef _TIME_H_
#define _TIME_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/types.h>

struct timespec {
	time_t tv_sec;
	long   tv_nsec;
};

struct tm {
	int tm_sec;
	int tm_min;
	int tm_hour;
	int tm_mday;
	int tm_mon;
	int tm_year;
	int tm_wday;
	int tm_yday;
	int tm_isdst;
	long tm_gmtoff;
	const char *tm_zone;
};

#define CLOCKS_PER_SEC 1000000

typedef int clockid_t;
#define CLOCK_REALTIME             0
#define CLOCK_MONOTONIC_RAW        4
#define CLOCK_MONOTONIC_RAW_APPROX 5
#define CLOCK_MONOTONIC            6
#define CLOCK_UPTIME_RAW           8
#define CLOCK_UPTIME_RAW_APPROX    9
#define CLOCK_PROCESS_CPUTIME_ID   12
#define CLOCK_THREAD_CPUTIME_ID    16

int clock_gettime(clockid_t clock_id, struct timespec *tp);
int clock_getres(clockid_t clock_id, struct timespec *res);

time_t time(time_t *tloc);
double difftime(time_t end, time_t start);
time_t mktime(struct tm *tm);
struct tm *gmtime(const time_t *timep);
struct tm *localtime(const time_t *timep);
struct tm *gmtime_r(const time_t *timep, struct tm *result);
struct tm *localtime_r(const time_t *timep, struct tm *result);
size_t strftime(char *s, size_t max, const char *format, const struct tm *tm);
char *asctime(const struct tm *tm);
char *ctime(const time_t *timep);
clock_t clock(void);
int nanosleep(const struct timespec *req, struct timespec *rem);

#ifdef __cplusplus
}
#endif

#endif /* _TIME_H_ */
