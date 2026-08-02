/* Darwin has no times(2) syscall -- real Libc emulates it via
 * getrusage(). We do the same, using the same tick rate our sysconf()
 * reports for _SC_CLK_TCK (100). */
#include <sys/times.h>
#include <sys/resource.h>

static clock_t
tv_to_ticks(struct timeval *tv)
{
	return (clock_t)(tv->tv_sec * 100 + tv->tv_usec / 10000);
}

clock_t
times(struct tms *buf)
{
	struct rusage self, children;
	getrusage(RUSAGE_SELF, &self);
	getrusage(RUSAGE_CHILDREN, &children);
	buf->tms_utime = tv_to_ticks(&self.ru_utime);
	buf->tms_stime = tv_to_ticks(&self.ru_stime);
	buf->tms_cutime = tv_to_ticks(&children.ru_utime);
	buf->tms_cstime = tv_to_ticks(&children.ru_stime);
	return buf->tms_utime + buf->tms_stime;
}
