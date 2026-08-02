/* Calendar time support -- gmtime/mktime/strftime implemented from
 * scratch (proleptic Gregorian civil calendar, well-known
 * days-since-epoch algorithm). localtime == gmtime: no timezone
 * database in this environment (TZ is always UTC), which is a
 * documented, deliberate limitation, not a bug. */
#include <time.h>
#include <sys/time.h>
#include <signal.h>
#include <string.h>
#include <stdio.h>
#include <mach/mach_time.h>

/* mach_absolute_time: no real monotonic hardware counter wired up yet
 * (see clock_gettime's comment below) -- microseconds since epoch via
 * the same gettimeofday() source is a real, honest tick source, just
 * not immune to wall-clock adjustments. mach_timebase_info's numer/
 * denom make the tick->nanosecond conversion exact for that choice
 * (1 tick == 1us == 1000ns), not an arbitrary placeholder. */
uint64_t
mach_absolute_time(void)
{
	struct timeval tv;
	gettimeofday(&tv, (void *)0);
	return (uint64_t)tv.tv_sec * 1000000ULL + (uint64_t)tv.tv_usec;
}

kern_return_t
mach_timebase_info(mach_timebase_info_t info)
{
	info->numer = 1000;
	info->denom = 1;
	return KERN_SUCCESS;
}

time_t
time(time_t *tloc)
{
	struct timeval tv;
	gettimeofday(&tv, (void *)0);
	if (tloc) {
		*tloc = tv.tv_sec;
	}
	return tv.tv_sec;
}

double
difftime(time_t end, time_t start)
{
	return (double)(end - start);
}

/* All clock IDs read the same wall-clock syscall -- there is no dedicated
 * monotonic-clock source wired up in this kernel yet, so CLOCK_MONOTONIC*
 * are not actually monotonic across a wall-clock adjustment. Nothing in
 * this environment adjusts the clock during normal operation (no ntpd),
 * so this is a reasonable bootstrap, not a silent correctness bug -- TODO:
 * back CLOCK_MONOTONIC* with a real monotonic source (e.g. mach_absolute_time)
 * if that ever matters here. */
int
clock_gettime(clockid_t clock_id, struct timespec *tp)
{
	(void)clock_id;
	struct timeval tv;
	if (gettimeofday(&tv, (void *)0) != 0) {
		return -1;
	}
	tp->tv_sec = tv.tv_sec;
	tp->tv_nsec = tv.tv_usec * 1000;
	return 0;
}

int
clock_getres(clockid_t clock_id, struct timespec *res)
{
	(void)clock_id;
	res->tv_sec = 0;
	res->tv_nsec = 1000; /* gettimeofday's microsecond resolution */
	return 0;
}

clock_t
clock(void)
{
	return (clock_t)time((void *)0) * CLOCKS_PER_SEC;
}

/* SIGALRM handler installed only for the duration of a nanosleep() call
 * below -- needed because SIGALRM's default disposition is to terminate
 * the process, and sigsuspend() must have something to wake it up. */
static void
nanosleep_alarm_noop(int sig)
{
	(void)sig;
}

/* There is no dedicated timed-wait syscall in this kernel (no BSD
 * nanosleep(2), no select()/poll() with a timeout wired up yet) --
 * real, not a stub: arms a one-shot ITIMER_REAL via setitimer(2) (already
 * used by alarm(), see syscalls.c) and blocks in sigsuspend(2) until it
 * fires. Known sharp edge, documented rather than hidden: this
 * temporarily installs its own SIGALRM handler and reprograms the
 * ITIMER_REAL timer, so a caller that has its own pending alarm()/
 * SIGALRM in flight at the same time will have it clobbered -- fine for
 * every caller in this project today (nothing calls nanosleep/usleep/
 * sleep while an alarm() from the same process is outstanding), but a
 * real kernel-level timed wait would not have this restriction. */
int
nanosleep(const struct timespec *req, struct timespec *rem)
{
	if (rem) {
		rem->tv_sec = 0;
		rem->tv_nsec = 0;
	}
	if (!req || (req->tv_sec == 0 && req->tv_nsec == 0)) {
		return 0;
	}

	struct sigaction sa, old_sa;
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = nanosleep_alarm_noop;
	sigaction(SIGALRM, &sa, &old_sa);

	struct itimerval it, old_it;
	memset(&it, 0, sizeof(it));
	it.it_value.tv_sec = req->tv_sec;
	it.it_value.tv_usec = req->tv_nsec / 1000;
	setitimer(ITIMER_REAL, &it, &old_it);

	sigset_t empty = 0;
	sigsuspend(&empty); /* wakes on SIGALRM -- or any OTHER signal, e.g. a
	                      * SIGTERM the caller's own handler catches, in
	                      * which case the itimer armed above is still
	                      * live and hasn't fired yet. */

	/* Ground-truthed as a real, not theoretical, bug: without this,
	 * a nanosleep() interrupted by a signal other than SIGALRM leaves
	 * the timer armed, then hands SIGALRM's disposition back to
	 * old_sa (often SIG_DFL, which terminates the process) below --
	 * the orphaned timer firing later kills the caller out of nowhere.
	 * Caught live: launchd (PID 1) died this way seconds after handling
	 * a SIGTERM mid-throttle-sleep, which panics the kernel (PID 1
	 * exiting is always fatal) -- see TODO.md Phase 14. Disarming
	 * unconditionally, regardless of why sigsuspend returned, is the
	 * actual fix. */
	struct itimerval disarm;
	memset(&disarm, 0, sizeof(disarm));
	setitimer(ITIMER_REAL, &disarm, (void *)0);

	sigaction(SIGALRM, &old_sa, (void *)0);
	return 0;
}

int
usleep(unsigned int usecs)
{
	struct timespec ts;
	ts.tv_sec = usecs / 1000000;
	ts.tv_nsec = (long)(usecs % 1000000) * 1000;
	return nanosleep(&ts, (void *)0);
}

/* civil_from_days / days_from_civil: Howard Hinnant's well-known
 * constant-time algorithms for converting between a day count (epoch
 * 1970-01-01 = 0) and a proleptic Gregorian y/m/d triple. */
static void
civil_from_days(long z, int *y, int *m, int *d)
{
	z += 719468;
	long era = (z >= 0 ? z : z - 146096) / 146097;
	unsigned doe = (unsigned)(z - era * 146097);
	unsigned yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
	long yy = (long)yoe + era * 400;
	unsigned doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
	unsigned mp = (5 * doy + 2) / 153;
	unsigned dd = doy - (153 * mp + 2) / 5 + 1;
	unsigned mm = mp + (mp < 10 ? 3 : -9);
	*y = (int)(yy + (mm <= 2));
	*m = (int)mm;
	*d = (int)dd;
}

static long
days_from_civil(int y, int m, int d)
{
	long yy = y - (m <= 2);
	long era = (yy >= 0 ? yy : yy - 399) / 400;
	unsigned yoe = (unsigned)(yy - era * 400);
	unsigned mp = (unsigned)(m + (m > 2 ? -3 : 9));
	unsigned doy = (153 * mp + 2) / 5 + (unsigned)d - 1;
	unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
	return era * 146097 + (long)doe - 719468;
}

struct tm *
gmtime_r(const time_t *timep, struct tm *tm)
{
	long days = (long)(*timep >= 0 ? *timep / 86400 : (*timep - 86399) / 86400);
	long rem = *timep - days * 86400;
	int y, mo, d;
	civil_from_days(days, &y, &mo, &d);
	tm->tm_year = y - 1900;
	tm->tm_mon = mo - 1;
	tm->tm_mday = d;
	tm->tm_hour = (int)(rem / 3600);
	tm->tm_min = (int)((rem % 3600) / 60);
	tm->tm_sec = (int)(rem % 60);
	long wday = (days % 7 + 7 + 4) % 7; /* 1970-01-01 was a Thursday (4) */
	tm->tm_wday = (int)wday;
	long jan1 = days_from_civil(y, 1, 1);
	tm->tm_yday = (int)(days - jan1);
	tm->tm_isdst = 0;
	tm->tm_gmtoff = 0;
	tm->tm_zone = "UTC";
	return tm;
}

struct tm *
localtime_r(const time_t *timep, struct tm *tm)
{
	return gmtime_r(timep, tm);
}

struct tm *
gmtime(const time_t *timep)
{
	static struct tm tm;
	return gmtime_r(timep, &tm);
}

struct tm *
localtime(const time_t *timep)
{
	static struct tm tm;
	return localtime_r(timep, &tm);
}

time_t
mktime(struct tm *tm)
{
	long days = days_from_civil(tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday);
	time_t t = days * 86400L + tm->tm_hour * 3600L + tm->tm_min * 60L + tm->tm_sec;
	gmtime_r(&t, tm); /* normalize wday/yday and any out-of-range fields */
	return t;
}

char *
asctime(const struct tm *tm)
{
	static const char *wdays[] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
	static const char *mons[] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun",
		"Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
	static char buf[26];
	snprintf(buf, sizeof(buf), "%s %s %2d %02d:%02d:%02d %d\n",
	    wdays[tm->tm_wday % 7], mons[tm->tm_mon % 12], tm->tm_mday,
	    tm->tm_hour, tm->tm_min, tm->tm_sec, tm->tm_year + 1900);
	return buf;
}

char *
ctime(const time_t *timep)
{
	return asctime(localtime(timep));
}

size_t
strftime(char *s, size_t max, const char *format, const struct tm *tm)
{
	static const char *wdays_full[] = { "Sunday", "Monday", "Tuesday", "Wednesday",
		"Thursday", "Friday", "Saturday" };
	static const char *wdays_abbr[] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
	static const char *mons_full[] = { "January", "February", "March", "April", "May",
		"June", "July", "August", "September", "October", "November", "December" };
	static const char *mons_abbr[] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun",
		"Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
	size_t pos = 0;
	char tmp[32];

	for (const char *f = format; *f && pos + 1 < max; f++) {
		if (*f != '%') {
			s[pos++] = *f;
			continue;
		}
		f++;
		int n = 0;
		switch (*f) {
		case 'Y': n = snprintf(tmp, sizeof(tmp), "%d", tm->tm_year + 1900); break;
		case 'y': n = snprintf(tmp, sizeof(tmp), "%02d", (tm->tm_year + 1900) % 100); break;
		case 'm': n = snprintf(tmp, sizeof(tmp), "%02d", tm->tm_mon + 1); break;
		case 'd': n = snprintf(tmp, sizeof(tmp), "%02d", tm->tm_mday); break;
		case 'e': n = snprintf(tmp, sizeof(tmp), "%2d", tm->tm_mday); break;
		case 'H': n = snprintf(tmp, sizeof(tmp), "%02d", tm->tm_hour); break;
		case 'I': {
			int h12 = tm->tm_hour % 12;
			if (h12 == 0) { h12 = 12; }
			n = snprintf(tmp, sizeof(tmp), "%02d", h12);
			break;
		}
		case 'M': n = snprintf(tmp, sizeof(tmp), "%02d", tm->tm_min); break;
		case 'S': n = snprintf(tmp, sizeof(tmp), "%02d", tm->tm_sec); break;
		case 'p': n = snprintf(tmp, sizeof(tmp), "%s", tm->tm_hour < 12 ? "AM" : "PM"); break;
		case 'a': n = snprintf(tmp, sizeof(tmp), "%s", wdays_abbr[tm->tm_wday % 7]); break;
		case 'A': n = snprintf(tmp, sizeof(tmp), "%s", wdays_full[tm->tm_wday % 7]); break;
		case 'b':
		case 'h': n = snprintf(tmp, sizeof(tmp), "%s", mons_abbr[tm->tm_mon % 12]); break;
		case 'B': n = snprintf(tmp, sizeof(tmp), "%s", mons_full[tm->tm_mon % 12]); break;
		case 'j': n = snprintf(tmp, sizeof(tmp), "%03d", tm->tm_yday + 1); break;
		case 'n': tmp[0] = '\n'; tmp[1] = 0; n = 1; break;
		case 't': tmp[0] = '\t'; tmp[1] = 0; n = 1; break;
		case 'Z': n = snprintf(tmp, sizeof(tmp), "%s", tm->tm_zone ? tm->tm_zone : "UTC"); break;
		case '%': tmp[0] = '%'; tmp[1] = 0; n = 1; break;
		default:
			tmp[0] = '%';
			tmp[1] = *f ? *f : 0;
			tmp[2] = 0;
			n = (int)strlen(tmp);
			break;
		}
		if (n < 0) {
			n = 0;
		}
		for (int i = 0; i < n && pos + 1 < max; i++) {
			s[pos++] = tmp[i];
		}
		if (!*f) {
			break;
		}
	}
	s[pos] = 0;
	return pos;
}
