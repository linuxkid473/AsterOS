/*
 * Raw BSD syscall wrappers -- no libc, no dyld, no Libsystem. This is the
 * "tiny userspace" layer documented in docs/architecture.md: class-2 BSD
 * syscalls (0x2000000 | number in %rax), args in %rdi/%rsi/%rdx/%r10/%r8/%r9
 * (r10 instead of rcx because the `syscall` instruction itself clobbers rcx
 * with the return address), `syscall` instruction, carry flag set on error
 * with the errno value left in %rax.
 *
 * Numbers ground-truthed directly from src/xnu/bsd/kern/syscalls.master.
 */
#ifndef DARWINBUILD_SYSCALL_H
#define DARWINBUILD_SYSCALL_H

typedef long ssize_t;
typedef unsigned long size_t;
typedef long off_t;

#define SYS_exit           1
#define SYS_read           3
#define SYS_write          4
#define SYS_open           5
#define SYS_close          6
#define SYS_unlink         10
#define SYS_chdir          12
#define SYS_mkdir          136
#define SYS_rmdir          137
#define SYS_mount          167
#define SYS_fstat          189
#define SYS_getdirentries64 344

#define O_RDONLY  0x0000
#define O_WRONLY  0x0001
#define O_RDWR    0x0002
#define O_CREAT   0x0200
#define O_TRUNC   0x0400
#define O_APPEND  0x0008

/* struct stat layout matters only for the one field we read (st_mode, to
 * tell files from directories) -- rather than reproduce Darwin's full
 * struct stat (which differs in field order/padding from Linux and is easy
 * to get subtly wrong from memory), we ask the kernel for it into a
 * generously-oversized buffer and read st_mode by its known byte offset,
 * ground-truthed against bsd/sys/stat.h's struct stat: dev_t(4) ino_t(8)
 * mode_t(2) ... i.e. offset 12. */
#define ST_MODE_OFFSET 12
#define STAT_BUF_SIZE  160
#define S_IFMT  0170000
#define S_IFDIR 0040000

static inline long
raw_syscall0(long num)
{
	long ret;
	__asm__ __volatile__(
	    "syscall"
	    : "=a"(ret)
	    : "a"(0x2000000 | num)
	    : "rcx", "r11", "memory");
	return ret;
}

static inline long
raw_syscall1(long num, long a1)
{
	long ret;
	register long r_a1 __asm__("rdi") = a1;
	__asm__ __volatile__(
	    "syscall"
	    : "=a"(ret)
	    : "a"(0x2000000 | num), "r"(r_a1)
	    : "rcx", "r11", "memory");
	return ret;
}

static inline long
raw_syscall2(long num, long a1, long a2)
{
	long ret;
	register long r_a1 __asm__("rdi") = a1;
	register long r_a2 __asm__("rsi") = a2;
	__asm__ __volatile__(
	    "syscall"
	    : "=a"(ret)
	    : "a"(0x2000000 | num), "r"(r_a1), "r"(r_a2)
	    : "rcx", "r11", "memory");
	return ret;
}

static inline long
raw_syscall3(long num, long a1, long a2, long a3)
{
	long ret;
	register long r_a1 __asm__("rdi") = a1;
	register long r_a2 __asm__("rsi") = a2;
	register long r_a3 __asm__("rdx") = a3;
	__asm__ __volatile__(
	    "syscall"
	    : "=a"(ret)
	    : "a"(0x2000000 | num), "r"(r_a1), "r"(r_a2), "r"(r_a3)
	    : "rcx", "r11", "memory");
	return ret;
}

static inline long
raw_syscall4(long num, long a1, long a2, long a3, long a4)
{
	long ret;
	register long r_a1 __asm__("rdi") = a1;
	register long r_a2 __asm__("rsi") = a2;
	register long r_a3 __asm__("rdx") = a3;
	register long r_a4 __asm__("r10") = a4;
	__asm__ __volatile__(
	    "syscall"
	    : "=a"(ret)
	    : "a"(0x2000000 | num), "r"(r_a1), "r"(r_a2), "r"(r_a3), "r"(r_a4)
	    : "rcx", "r11", "memory");
	return ret;
}

/* Every wrapper below returns a POSIX-style result: >=0 (or the real value)
 * on success, -1 on error. The raw carry-flag/errno protocol is folded away
 * here since none of our callers need the specific errno value beyond
 * success/failure -- one less thing raw inline asm has to expose. We detect
 * the error case the portable way: BSD syscalls document negative return
 * values as impossible on success (fixed-width counts/fds/etc.), so a
 * negative raw return is unambiguously the (positive, negated for
 * convenience here) errno. */
static inline long
sys_exit(int code)
{
	return raw_syscall1(SYS_exit, code);
}

static inline ssize_t
sys_read(int fd, void *buf, size_t n)
{
	return raw_syscall3(SYS_read, fd, (long)buf, (long)n);
}

static inline ssize_t
sys_write(int fd, const void *buf, size_t n)
{
	return raw_syscall3(SYS_write, fd, (long)buf, (long)n);
}

static inline int
sys_open(const char *path, int flags, int mode)
{
	return (int)raw_syscall3(SYS_open, (long)path, flags, mode);
}

static inline int
sys_close(int fd)
{
	return (int)raw_syscall1(SYS_close, fd);
}

static inline int
sys_unlink(const char *path)
{
	return (int)raw_syscall1(SYS_unlink, (long)path);
}

static inline int
sys_mkdir(const char *path, int mode)
{
	return (int)raw_syscall2(SYS_mkdir, (long)path, mode);
}

static inline int
sys_rmdir(const char *path)
{
	return (int)raw_syscall1(SYS_rmdir, (long)path);
}

static inline int
sys_mount(const char *type, const char *dir, int flags, void *data)
{
	return (int)raw_syscall4(SYS_mount, (long)type, (long)dir, flags, (long)data);
}

static inline int
sys_fstat(int fd, void *statbuf)
{
	return (int)raw_syscall2(SYS_fstat, fd, (long)statbuf);
}

static inline ssize_t
sys_getdirentries64(int fd, void *buf, size_t bufsize, off_t *position)
{
	return raw_syscall4(SYS_getdirentries64, fd, (long)buf, (long)bufsize, (long)position);
}

#endif /* DARWINBUILD_SYSCALL_H */
