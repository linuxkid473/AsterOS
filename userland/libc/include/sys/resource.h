#ifndef _SYS_RESOURCE_H_
#define _SYS_RESOURCE_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/time.h>

struct rusage {
	struct timeval ru_utime;
	struct timeval ru_stime;
	long ru_maxrss;
	long ru_ixrss;
	long ru_idrss;
	long ru_isrss;
	long ru_minflt;
	long ru_majflt;
	long ru_nswap;
	long ru_inblock;
	long ru_oublock;
	long ru_msgsnd;
	long ru_msgrcv;
	long ru_nsignals;
	long ru_nvcsw;
	long ru_nivcsw;
};

#define RUSAGE_SELF     0
#define RUSAGE_CHILDREN (-1)

#define PRIO_PROCESS 0
#define PRIO_PGRP    1
#define PRIO_USER    2
#define PRIO_DARWIN_THREAD  3
#define PRIO_DARWIN_PROCESS 4
#define PRIO_DARWIN_BG      0x1000
#define PRIO_DARWIN_NONUI   0x1001

typedef unsigned long long rlim_t;

struct rlimit {
	rlim_t rlim_cur;
	rlim_t rlim_max;
};

#define RLIM_INFINITY  (((unsigned long long)1 << 63) - 1)
#define RLIM_SAVED_MAX RLIM_INFINITY
#define RLIM_SAVED_CUR RLIM_INFINITY

#define RLIMIT_CPU     0
#define RLIMIT_FSIZE   1
#define RLIMIT_DATA    2
#define RLIMIT_STACK   3
#define RLIMIT_CORE    4
#define RLIMIT_AS      5
#define RLIMIT_RSS     RLIMIT_AS
#define RLIMIT_MEMLOCK 6
#define RLIMIT_NPROC   7
#define RLIMIT_NOFILE  8
#define RLIM_NLIMITS   9

int getrlimit(int resource, struct rlimit *rlp);
int setrlimit(int resource, const struct rlimit *rlp);
int getrusage(int who, struct rusage *ru);
int getpriority(int which, int who);
int setpriority(int which, int who, int prio);

#ifdef __cplusplus
}
#endif

#endif /* _SYS_RESOURCE_H_ */
