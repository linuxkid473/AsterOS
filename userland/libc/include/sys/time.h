#ifndef _SYS_TIME_H_
#define _SYS_TIME_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/types.h>

struct timeval {
	time_t      tv_sec;
	suseconds_t tv_usec;
};

struct timezone {
	int tz_minuteswest;
	int tz_dsttime;
};

struct itimerval {
	struct timeval it_interval;
	struct timeval it_value;
};

#define ITIMER_REAL    0
#define ITIMER_VIRTUAL 1
#define ITIMER_PROF    2

int gettimeofday(struct timeval *tp, void *tzp);
int settimeofday(const struct timeval *tp, const struct timezone *tzp);
int utimes(const char *path, const struct timeval times[2]);
int getitimer(int which, struct itimerval *value);
int setitimer(int which, const struct itimerval *value, struct itimerval *ovalue);

#ifdef __cplusplus
}
#endif

#endif /* _SYS_TIME_H_ */
