#ifndef _SYS_TIMES_H_
#define _SYS_TIMES_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/types.h>

struct tms {
	clock_t tms_utime;
	clock_t tms_stime;
	clock_t tms_cutime;
	clock_t tms_cstime;
};

clock_t times(struct tms *buf);

#ifdef __cplusplus
}
#endif

#endif /* _SYS_TIMES_H_ */
