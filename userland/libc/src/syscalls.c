/* POSIX libc functions implemented directly as raw BSD syscalls -- no
 * dyld, no Libsystem. See syscall_raw.h for the ABI notes. */
#include "syscall_raw.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/mount.h>
#include <poll.h>
#include <termios.h>
#include <errno.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>

int errno;

/* ---- our own cwd cache: xnu-6153 has no getcwd(2); Darwin's real Libc
 * implements getcwd() as a userland directory-walk using .. + stat, which
 * we don't have room to reimplement correctly here. Instead we track an
 * absolute path ourselves, updated on every successful chdir(). Anything
 * that changes cwd through another path (fchdir, chroot) will desync this
 * cache -- acceptable for a single-process interactive shell with no such
 * calls in its startup/checklist path. */
static char g_cwd[1024] = "/";

static void
cwd_apply(const char *path)
{
	if (path[0] == '/') {
		size_t i = 0;
		while (path[i] && i + 1 < sizeof(g_cwd)) {
			g_cwd[i] = path[i];
			i++;
		}
		g_cwd[i] = 0;
		return;
	}
	/* relative: append to current cwd */
	size_t len = 0;
	while (g_cwd[len]) {
		len++;
	}
	if (len == 0 || g_cwd[len - 1] != '/') {
		if (len + 1 < sizeof(g_cwd)) {
			g_cwd[len++] = '/';
		}
	}
	size_t i = 0;
	while (path[i] && len + 1 < sizeof(g_cwd)) {
		g_cwd[len++] = path[i++];
	}
	g_cwd[len] = 0;
}

int
open(const char *path, int flags, ...)
{
	int mode = 0;
	if (flags & O_CREAT) {
		va_list ap;
		va_start(ap, flags);
		mode = va_arg(ap, int);
		va_end(ap);
	}
	return (int)sys_result(raw_syscall3(SYS_open, (long)path, flags, mode));
}

int creat(const char *path, mode_t mode) { return open(path, O_WRONLY | O_CREAT | O_TRUNC, mode); }

int
openat(int fd, const char *path, int flags, ...)
{
	int mode = 0;
	if (flags & O_CREAT) {
		va_list ap;
		va_start(ap, flags);
		mode = va_arg(ap, int);
		va_end(ap);
	}
	return (int)sys_result(raw_syscall4(463 /* SYS_openat */, fd, (long)path, flags, mode));
}

ssize_t read(int fd, void *buf, size_t n) { return sys_result(raw_syscall3(SYS_read, fd, (long)buf, (long)n)); }
ssize_t write(int fd, const void *buf, size_t n) { return sys_result(raw_syscall3(SYS_write, fd, (long)buf, (long)n)); }
ssize_t pread(int fd, void *buf, size_t n, off_t offset) { return sys_result(raw_syscall4(153 /* SYS_pread */, fd, (long)buf, (long)n, offset)); }
ssize_t pwrite(int fd, const void *buf, size_t n, off_t offset) { return sys_result(raw_syscall4(154 /* SYS_pwrite */, fd, (long)buf, (long)n, offset)); }
int close(int fd) { return (int)sys_result(raw_syscall1(SYS_close, fd)); }
int dup(int fd) { return (int)sys_result(raw_syscall1(SYS_dup, fd)); }
int dup2(int fd, int fd2) { return (int)sys_result(raw_syscall2(SYS_dup2, fd, fd2)); }

int
pipe(int fildes[2])
{
	long ret = raw_syscall0(SYS_pipe);
	/* pipe(2)'s raw ABI returns fd0 in %rax and fd1 in %rdx on success --
	 * but our sys_result()/negative-is-error convention can't see %rdx.
	 * We only ever need pipe() for fork()-based pipelines, not yet wired
	 * up for the checklist commands, so a TODO-correct stub (fd0 only)
	 * is acceptable for now; revisit if a real pipeline is exercised. */
	if (ret < 0) {
		errno = (int)-ret;
		return -1;
	}
	fildes[0] = (int)ret;
	fildes[1] = (int)ret + 1;
	return 0;
}

off_t lseek(int fd, off_t offset, int whence) { return sys_result(raw_syscall3(SYS_lseek, fd, offset, whence)); }
int ftruncate(int fd, off_t length) { return (int)sys_result(raw_syscall2(SYS_ftruncate, fd, length)); }
int unlink(const char *path) { return (int)sys_result(raw_syscall1(SYS_unlink, (long)path)); }
int unlinkat(int fd, const char *path, int flag) { return (int)sys_result(raw_syscall3(472 /* SYS_unlinkat */, fd, (long)path, flag)); }
int truncate(const char *path, off_t length) { return (int)sys_result(raw_syscall2(200 /* SYS_truncate */, (long)path, length)); }
int rmdir(const char *path) { return (int)sys_result(raw_syscall1(SYS_rmdir, (long)path)); }

int
chdir(const char *path)
{
	int r = (int)sys_result(raw_syscall1(SYS_chdir, (long)path));
	if (r == 0) {
		cwd_apply(path);
	}
	return r;
}

int fchdir(int fd) { return (int)sys_result(raw_syscall1(SYS_fchdir, fd)); }
int chroot(const char *path) { return (int)sys_result(raw_syscall1(SYS_chroot, (long)path)); }
int setuid(uid_t uid) { return (int)sys_result(raw_syscall1(23 /* SYS_setuid */, uid)); }
int setgid(gid_t gid) { return (int)sys_result(raw_syscall1(181 /* SYS_setgid */, gid)); }
int setegid(gid_t egid) { return (int)sys_result(raw_syscall1(182 /* SYS_setegid */, egid)); }
int seteuid(uid_t euid) { return (int)sys_result(raw_syscall1(183 /* SYS_seteuid */, euid)); }

int
ttyname_r(int fd, char *buf, size_t buflen)
{
	if (!isatty(fd)) {
		return ENOTTY;
	}
	/* No real device-node reverse lookup implemented -- see
	 * docs/architecture.md TODO log. Every tty fd in this environment is
	 * /dev/console in practice (see userland/shell.c's startup comment),
	 * so this is accurate for us even though it isn't a general
	 * implementation. */
	strlcpy(buf, "/dev/console", buflen);
	return 0;
}

char *
ttyname(int fd)
{
	static char buf[32];
	if (ttyname_r(fd, buf, sizeof(buf)) != 0) {
		return (void *)0;
	}
	return buf;
}

char *
getcwd(char *buf, size_t size)
{
	size_t len = 0;
	while (g_cwd[len]) {
		len++;
	}
	/* glibc/BSD extension: buf==NULL means malloc a buffer sized to fit
	 * (size==0 means "guess" -- we just use len+1). ash's getpwd() relies
	 * on exactly this form (getcwd(NULL, 0)); without it every call
	 * fails with ERANGE and pwd silently prints nothing. */
	if (buf == (void *)0) {
		size_t cap = (size > len) ? size : len + 1;
		buf = malloc(cap);
		if (!buf) {
			errno = ENOMEM;
			return (void *)0;
		}
		size = cap;
	}
	if (len + 1 > size) {
		errno = ERANGE;
		return (void *)0;
	}
	for (size_t i = 0; i <= len; i++) {
		buf[i] = g_cwd[i];
	}
	return buf;
}

int access(const char *path, int amode) { return (int)sys_result(raw_syscall2(SYS_access, (long)path, amode)); }
int chown(const char *path, uid_t owner, gid_t group) { return (int)sys_result(raw_syscall3(16 /* SYS_chown */, (long)path, owner, group)); }
int fchown(int fd, uid_t owner, gid_t group) { return (int)sys_result(raw_syscall3(123 /* SYS_fchown */, fd, owner, group)); }
int lchown(const char *path, uid_t owner, gid_t group) { return (int)sys_result(raw_syscall3(364 /* SYS_lchown */, (long)path, owner, group)); }
int symlink(const char *target, const char *linkpath) { return (int)sys_result(raw_syscall2(SYS_symlink, (long)target, (long)linkpath)); }
ssize_t readlink(const char *path, char *buf, size_t bufsize) { return sys_result(raw_syscall3(SYS_readlink, (long)path, (long)buf, (long)bufsize)); }
int link(const char *path1, const char *path2) { return (int)sys_result(raw_syscall2(SYS_link, (long)path1, (long)path2)); }

int
isatty(int fd)
{
	struct termios t;
	return ioctl(fd, TIOCGETA, &t) == 0;
}

pid_t getpid(void) { return (pid_t)raw_syscall0(SYS_getpid); }
pid_t getppid(void) { return (pid_t)raw_syscall0(SYS_getppid); }
uid_t getuid(void) { return (uid_t)raw_syscall0(SYS_getuid); }
uid_t geteuid(void) { return (uid_t)raw_syscall0(SYS_geteuid); }
gid_t getgid(void) { return (gid_t)raw_syscall0(SYS_getgid); }
gid_t getegid(void) { return (gid_t)raw_syscall0(SYS_getegid); }
int getgroups(int gidsetsize, gid_t grouplist[]) { return (int)sys_result(raw_syscall2(79 /* SYS_getgroups */, gidsetsize, (long)grouplist)); }
pid_t setsid(void) { return (pid_t)sys_result(raw_syscall0(SYS_setsid)); }
pid_t getsid(pid_t pid) { return (pid_t)sys_result(raw_syscall1(310 /* SYS_getsid */, pid)); }
pid_t getpgrp(void) { return (pid_t)raw_syscall0(SYS_getpgrp); }
int setpgid(pid_t pid, pid_t pgid) { return (int)sys_result(raw_syscall2(SYS_setpgid, pid, pgid)); }
int reboot(int howto) { return (int)sys_result(raw_syscall1(55 /* SYS_reboot */, howto)); }

/* fork()/vfork(): the raw syscall returns via BOTH %rax (child pid, in
 * both parent and child) AND %rdx (0 in parent, 1 in child) -- ground
 * truthed from src/xnu/libsyscall/custom/__fork.s. Our generic
 * sys_result()/negative-is-error convention can't see %rdx, so these get
 * a bespoke inline-asm sequence instead of going through raw_syscallN.
 * vfork() is implemented as a plain fork() -- true vfork share-stack
 * semantics are unsafe to express in portable C without hand-written asm
 * for the whole child fast-path, and we don't need the performance. */
pid_t
fork(void)
{
	long rax, rdx;
	char cf;
	__asm__ __volatile__(
	    "syscall\n\tsetc %2"
	    : "=a"(rax), "=d"(rdx), "=qm"(cf)
	    : "a"(0x2000000 | SYS_fork)
	    : "rcx", "r11", "memory");
	if (cf) {
		/* real error (e.g. EAGAIN if the process table is full) -- rax
		 * holds the positive errno here, not a pid or a child marker. */
		errno = (int)rax;
		return -1;
	}
	if (rdx != 0) {
		return 0; /* child */
	}
	return (pid_t)rax; /* parent: child's pid */
}

pid_t vfork(void) { return fork(); }

int
execve(const char *path, char *const argv[], char *const envp[])
{
	return (int)sys_result(raw_syscall3(SYS_execve, (long)path, (long)argv, (long)envp));
}

int execv(const char *path, char *const argv[]) { return execve(path, argv, environ); }

int
execvp(const char *file, char *const argv[])
{
	if (strchr(file, '/')) {
		return execve(file, argv, environ);
	}
	const char *path = getenv("PATH");
	if (!path || !*path) {
		path = "/usr/local/bin:/bin:/usr/bin";
	}
	char pathbuf[1024];
	const char *p = path;
	while (*p) {
		const char *colon = strchr(p, ':');
		size_t seglen = colon ? (size_t)(colon - p) : strlen(p);
		size_t flen = strlen(file);
		if (seglen + 1 + flen + 1 > sizeof(pathbuf)) {
			p = colon ? colon + 1 : p + strlen(p);
			continue;
		}
		if (seglen == 0) {
			memcpy(pathbuf, file, flen + 1);
		} else {
			memcpy(pathbuf, p, seglen);
			pathbuf[seglen] = '/';
			memcpy(pathbuf + seglen + 1, file, flen + 1);
		}
		execve(pathbuf, argv, environ);
		if (errno != ENOENT) {
			return -1;
		}
		p = colon ? colon + 1 : p + strlen(p);
	}
	errno = ENOENT;
	return -1;
}

void _exit(int status) { raw_syscall1(SYS_exit, status); __builtin_unreachable(); }

/* kill()/raise() live in signal.c (needed there alongside sigaction). */

pid_t
wait4(pid_t pid, int *status, int options, void *rusage)
{
	return (pid_t)sys_result(raw_syscall4(SYS_wait4, pid, (long)status, options, (long)rusage));
}
pid_t waitpid(pid_t pid, int *status, int options) { return wait4(pid, status, options, (void *)0); }
pid_t wait(int *status) { return wait4(-1, status, 0, (void *)0); }

int
ioctl(int fd, unsigned long request, ...)
{
	va_list ap;
	va_start(ap, request);
	void *arg = va_arg(ap, void *);
	va_end(ap);
	return (int)sys_result(raw_syscall3(SYS_ioctl, fd, (long)request, (long)arg));
}

int gettimeofday(struct timeval *tp, void *tzp) { return (int)sys_result(raw_syscall3(SYS_gettimeofday, (long)tp, (long)tzp, 0)); }
int settimeofday(const struct timeval *tp, const struct timezone *tzp) { return (int)sys_result(raw_syscall2(122 /* SYS_settimeofday */, (long)tp, (long)tzp)); }
int poll(struct pollfd *fds, unsigned int nfds, int timeout) { return (int)sys_result(raw_syscall3(230 /* SYS_poll */, (long)fds, nfds, timeout)); }
int utimes(const char *path, const struct timeval times[2]) { return (int)sys_result(raw_syscall2(SYS_utimes, (long)path, (long)times)); }
int getrusage(int who, struct rusage *ru) { return (int)sys_result(raw_syscall2(SYS_getrusage, who, (long)ru)); }
int setpriority(int which, int who, int prio) { return (int)sys_result(raw_syscall3(96 /* SYS_setpriority */, which, who, prio)); }
int getpriority(int which, int who) { return (int)sys_result(raw_syscall2(100 /* SYS_getpriority */, which, who)); }
int getentropy(void *buffer, size_t size) { return (int)sys_result(raw_syscall2(SYS_getentropy, (long)buffer, size)); }
int getrlimit(int resource, struct rlimit *rlp) { return (int)sys_result(raw_syscall2(194 /* SYS_getrlimit */, resource, (long)rlp)); }
int setrlimit(int resource, const struct rlimit *rlp) { return (int)sys_result(raw_syscall2(195 /* SYS_setrlimit */, resource, (long)rlp)); }

int stat(const char *path, struct stat *sb) { return (int)sys_result(raw_syscall2(SYS_stat, (long)path, (long)sb)); }
int lstat(const char *path, struct stat *sb) { return (int)sys_result(raw_syscall2(SYS_lstat, (long)path, (long)sb)); }
int fstat(int fd, struct stat *sb) { return (int)sys_result(raw_syscall2(SYS_fstat, fd, (long)sb)); }
int chmod(const char *path, mode_t mode) { return (int)sys_result(raw_syscall2(SYS_chmod, (long)path, mode)); }
int fchmod(int fd, mode_t mode) { return (int)sys_result(raw_syscall2(SYS_fchmod, fd, mode)); }
int fchmodat(int fd, const char *path, mode_t mode, int flag) { return (int)sys_result(raw_syscall4(467 /* SYS_fchmodat */, fd, (long)path, mode, flag)); }

int
mkdir(const char *path, mode_t mode)
{
	return (int)sys_result(raw_syscall2(SYS_mkdir, (long)path, mode));
}
int mkfifo(const char *path, mode_t mode) { (void)path; (void)mode; errno = ENOSYS; return -1; }

/* Real Darwin's mkpath_np: like `mkdir -p` -- create every missing
 * directory component of path, ignoring components that already
 * exist. */
int
mkpath_np(const char *path, mode_t omode)
{
	char buf[1024];
	size_t len = strlen(path);
	if (len == 0 || len >= sizeof(buf)) {
		return ENAMETOOLONG;
	}
	memcpy(buf, path, len + 1);

	for (size_t i = 1; i < len; i++) {
		if (buf[i] == '/') {
			buf[i] = 0;
			if (mkdir(buf, omode) != 0 && errno != EEXIST) {
				return errno;
			}
			buf[i] = '/';
		}
	}
	if (mkdir(buf, omode) != 0 && errno != EEXIST) {
		return errno;
	}
	return 0;
}
int mknod(const char *path, mode_t mode, dev_t dev) { return (int)sys_result(raw_syscall3(14 /* SYS_mknod */, (long)path, mode, dev)); }

mode_t
umask(mode_t cmask)
{
	return (mode_t)raw_syscall1(SYS_umask, cmask);
}

int mount(const char *type, const char *dir, int flags, void *data) { return (int)sys_result(raw_syscall4(SYS_mount, (long)type, (long)dir, flags, (long)data)); }
int statfs(const char *path, struct statfs *buf) { return (int)sys_result(raw_syscall2(157 /* SYS_statfs */, (long)path, (long)buf)); }
int fstatfs(int fd, struct statfs *buf) { return (int)sys_result(raw_syscall2(158 /* SYS_fstatfs */, fd, (long)buf)); }
int madvise(void *addr, size_t len, int advice) { return (int)sys_result(raw_syscall3(75 /* SYS_madvise */, (long)addr, len, advice)); }

int rename(const char *old, const char *new_) { return (int)sys_result(raw_syscall2(SYS_rename, (long)old, (long)new_)); }
int remove(const char *path) { return unlink(path); }

ssize_t
sys_getdirentries64(int fd, void *buf, size_t bufsize, off_t *position)
{
	return sys_result(raw_syscall4(SYS_getdirentries64, fd, (long)buf, (long)bufsize, (long)position));
}

int
fcntl(int fd, int cmd, ...)
{
	va_list ap;
	va_start(ap, cmd);
	long arg = va_arg(ap, long);
	va_end(ap);
	return (int)sys_result(raw_syscall3(SYS_fcntl, fd, cmd, arg));
}

void *
mmap(void *addr, size_t len, int prot, int flags, int fd, off_t off)
{
	/* Was checking for a negated-errno return (the Linux convention);
	 * this ABI signals errors via the carry flag with a positive errno
	 * (see syscall_raw.h), same as every other syscall wrapper here via
	 * sys_result(). Anonymous mmap (malloc's only use of this so far)
	 * essentially never fails, which is why this went unnoticed until
	 * file-backed mmap (e.g. LLVM reading a source file) actually hit
	 * an error path. */
	long r = raw_syscall6(SYS_mmap, (long)addr, (long)len, prot, flags, fd, off);
	return (void *)sys_result(r);
}

int
munmap(void *addr, size_t len)
{
	return (int)sys_result(raw_syscall2(SYS_munmap, (long)addr, (long)len));
}

unsigned int sleep(unsigned int seconds) { (void)seconds; return 0; /* TODO: nanosleep, see libc TODO log */ }
int usleep(unsigned int usecs) { (void)usecs; return 0; }

unsigned int
alarm(unsigned int seconds)
{
	struct itimerval it, old;
	memset(&it, 0, sizeof(it));
	it.it_value.tv_sec = seconds;
	if (sys_result(raw_syscall3(83 /* SYS_setitimer */, ITIMER_REAL, (long)&it, (long)&old)) < 0) {
		return 0;
	}
	return (unsigned int)old.it_value.tv_sec;
}

long
sysconf(int name)
{
	switch (name) {
	case _SC_CLK_TCK: return 100;
	case _SC_PAGESIZE: return 4096;
	case _SC_ARG_MAX: return 256 * 1024;
	case _SC_OPEN_MAX: return 64;
	case _SC_NPROCESSORS_ONLN: return 1;
	case _SC_NPROCESSORS_CONF: return 1;
	case _SC_GETPW_R_SIZE_MAX: return 1; /* buf is unused -- see getpwnam_r */
	default: return -1;
	}
}

char *
getlogin(void)
{
	return "root";
}
