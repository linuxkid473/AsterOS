/* Private raw BSD syscall layer -- class-2 syscalls (0x2000000 | number in
 * %rax), args in %rdi,%rsi,%rdx,%r10,%r8,%r9 (r10 not rcx -- `syscall`
 * clobbers rcx with the return address), carry flag set on error with
 * errno in %rax. Ground-truthed from src/xnu/bsd/kern/syscalls.master and
 * src/xnu/libsyscall/custom sources. Not installed as a public header --
 * internal to libc/src only. */
#ifndef DARWINBUILD_SYSCALL_RAW_H
#define DARWINBUILD_SYSCALL_RAW_H

extern int errno;

#define SYS_exit          1
#define SYS_fork          2
#define SYS_read          3
#define SYS_write         4
#define SYS_open          5
#define SYS_close         6
#define SYS_wait4         7
#define SYS_link          9
#define SYS_unlink        10
#define SYS_chdir         12
#define SYS_fchdir        13
#define SYS_chmod         15
#define SYS_getpid        20
#define SYS_getuid        24
#define SYS_geteuid       25
#define SYS_access        33
#define SYS_kill          37
#define SYS_getppid       39
#define SYS_dup           41
#define SYS_pipe          42
#define SYS_getegid       43
#define SYS_sigaction     46
#define SYS_getgid        47
#define SYS_sigprocmask   48
#define SYS_ioctl         54
#define SYS_symlink       57
#define SYS_readlink      58
#define SYS_execve        59
#define SYS_umask         60
#define SYS_chroot        61
#define SYS_vfork         66
#define SYS_munmap        73
#define SYS_getpgrp       81
#define SYS_setpgid       82
#define SYS_dup2          90
#define SYS_fcntl         92
#define SYS_mkdir         136
#define SYS_rmdir         137
#define SYS_mount         167
#define SYS_fchmod        124
#define SYS_rename        128
#define SYS_utimes        138
#define SYS_setsid        147
#define SYS_mmap          197
#define SYS_lseek         199
#define SYS_ftruncate     201
#define SYS_getrusage     117
#define SYS_gettimeofday  116
#define SYS_getdirentries64 344
#define SYS_getentropy    500

/* stat/fstat/lstat (syscalls 188/189/190) produce the OLD 32-bit-ino
 * struct layout (fstatat_internal()/fstat1() called with is64==0, see
 * src/xnu/bsd/vfs/vfs_syscalls.c and kern/kern_descrip.c). Our struct stat
 * (sys/stat.h) matches __DARWIN_STRUCT_STAT64, so we must call the *64
 * syscalls (is64==1) instead, ground-truthed the same way -- using
 * 188/189/190 here would silently misinterpret every field after
 * st_rdev. */
#define SYS_stat          338
#define SYS_fstat         339
#define SYS_lstat         340

/* IMPORTANT ABI NOTE (found the hard way -- see the running log in
 * TODO.md): on error, xnu leaves the POSITIVE errno in %rax and sets the
 * carry flag; it does NOT negate it. A "negative raw return means error"
 * heuristic is simply wrong -- any syscall whose errno value (1-106)
 * could also plausibly be a legitimate small positive success return
 * (fd counts, byte counts, ...) will have its errors silently
 * misread as success. This is exactly what real Apple syscall stubs
 * check the carry flag for (see src/xnu/libsyscall/custom/__fork.s:
 * "jnc L1" / "CALL_EXTERN(_cerror)"), so we do the same here: every
 * raw_syscallN captures CF via `setc` into g_syscall_cf, and sys_result()
 * consults that instead of guessing from the sign of the return value. */
static int g_syscall_cf;

static inline long
raw_syscall0(long num)
{
	long ret;
	char cf;
	__asm__ __volatile__("syscall\n\tsetc %1"
	    : "=a"(ret), "=qm"(cf)
	    : "a"(0x2000000 | num)
	    : "rcx", "r11", "memory");
	g_syscall_cf = cf;
	return ret;
}

static inline long
raw_syscall1(long num, long a1)
{
	long ret;
	char cf;
	register long r_a1 __asm__("rdi") = a1;
	__asm__ __volatile__("syscall\n\tsetc %1"
	    : "=a"(ret), "=qm"(cf)
	    : "a"(0x2000000 | num), "r"(r_a1)
	    : "rcx", "r11", "memory");
	g_syscall_cf = cf;
	return ret;
}

static inline long
raw_syscall2(long num, long a1, long a2)
{
	long ret;
	char cf;
	register long r_a1 __asm__("rdi") = a1;
	register long r_a2 __asm__("rsi") = a2;
	__asm__ __volatile__("syscall\n\tsetc %1"
	    : "=a"(ret), "=qm"(cf)
	    : "a"(0x2000000 | num), "r"(r_a1), "r"(r_a2)
	    : "rcx", "r11", "memory");
	g_syscall_cf = cf;
	return ret;
}

static inline long
raw_syscall3(long num, long a1, long a2, long a3)
{
	long ret;
	char cf;
	register long r_a1 __asm__("rdi") = a1;
	register long r_a2 __asm__("rsi") = a2;
	register long r_a3 __asm__("rdx") = a3;
	__asm__ __volatile__("syscall\n\tsetc %1"
	    : "=a"(ret), "=qm"(cf)
	    : "a"(0x2000000 | num), "r"(r_a1), "r"(r_a2), "r"(r_a3)
	    : "rcx", "r11", "memory");
	g_syscall_cf = cf;
	return ret;
}

static inline long
raw_syscall4(long num, long a1, long a2, long a3, long a4)
{
	long ret;
	char cf;
	register long r_a1 __asm__("rdi") = a1;
	register long r_a2 __asm__("rsi") = a2;
	register long r_a3 __asm__("rdx") = a3;
	register long r_a4 __asm__("r10") = a4;
	__asm__ __volatile__("syscall\n\tsetc %1"
	    : "=a"(ret), "=qm"(cf)
	    : "a"(0x2000000 | num), "r"(r_a1), "r"(r_a2), "r"(r_a3), "r"(r_a4)
	    : "rcx", "r11", "memory");
	g_syscall_cf = cf;
	return ret;
}

static inline long
raw_syscall5(long num, long a1, long a2, long a3, long a4, long a5)
{
	long ret;
	char cf;
	register long r_a1 __asm__("rdi") = a1;
	register long r_a2 __asm__("rsi") = a2;
	register long r_a3 __asm__("rdx") = a3;
	register long r_a4 __asm__("r10") = a4;
	register long r_a5 __asm__("r8") = a5;
	__asm__ __volatile__("syscall\n\tsetc %1"
	    : "=a"(ret), "=qm"(cf)
	    : "a"(0x2000000 | num), "r"(r_a1), "r"(r_a2), "r"(r_a3), "r"(r_a4), "r"(r_a5)
	    : "rcx", "r11", "memory");
	g_syscall_cf = cf;
	return ret;
}

static inline long
raw_syscall6(long num, long a1, long a2, long a3, long a4, long a5, long a6)
{
	long ret;
	char cf;
	register long r_a1 __asm__("rdi") = a1;
	register long r_a2 __asm__("rsi") = a2;
	register long r_a3 __asm__("rdx") = a3;
	register long r_a4 __asm__("r10") = a4;
	register long r_a5 __asm__("r8") = a5;
	register long r_a6 __asm__("r9") = a6;
	__asm__ __volatile__("syscall\n\tsetc %1"
	    : "=a"(ret), "=qm"(cf)
	    : "a"(0x2000000 | num), "r"(r_a1), "r"(r_a2), "r"(r_a3), "r"(r_a4), "r"(r_a5), "r"(r_a6)
	    : "rcx", "r11", "memory");
	g_syscall_cf = cf;
	return ret;
}

/* Every wrapper below returns a POSIX-style result and sets `errno` on
 * failure, using the real carry flag captured by the raw_syscallN call
 * immediately preceding it (must be called in the same expression/
 * statement -- g_syscall_cf is a plain scratch variable, not thread-safe
 * or reentrant, but we have no threads here). */
static inline long
sys_result(long raw)
{
	if (g_syscall_cf) {
		errno = (int)raw;
		return -1;
	}
	return raw;
}

#endif /* DARWINBUILD_SYSCALL_RAW_H */
