/* Signal numbers and struct layouts ground-truthed against
 * src/xnu/bsd/sys/signal.h. The raw sigaction(2) syscall's struct
 * __sigaction (with sa_tramp) and the trampoline calling convention are
 * ground-truthed against src/libplatform/src/setjmp/x86_64/_sigtramp.s --
 * our own trampoline (libc/src/sigtramp.S) is a simplified reimplementation
 * of that (functional part only, no DWARF unwind tables). */
#ifndef _SIGNAL_H_
#define _SIGNAL_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/types.h>

#define SIGHUP    1
#define SIGINT    2
#define SIGQUIT   3
#define SIGILL    4
#define SIGTRAP   5
#define SIGABRT   6
#define SIGIOT    SIGABRT
#define SIGEMT    7
#define SIGFPE    8
#define SIGKILL   9
#define SIGBUS    10
#define SIGSEGV   11
#define SIGSYS    12
#define SIGPIPE   13
#define SIGALRM   14
#define SIGTERM   15
#define SIGURG    16
#define SIGSTOP   17
#define SIGTSTP   18
#define SIGCONT   19
#define SIGCHLD   20
#define SIGTTIN   21
#define SIGTTOU   22
#define SIGIO     23
#define SIGXCPU   24
#define SIGXFSZ   25
#define SIGVTALRM 26
#define SIGPROF   27
#define SIGWINCH  28
#define SIGINFO   29
#define SIGUSR1   30
#define SIGUSR2   31
#define NSIG      32

#define SIG_DFL ((void (*)(int))0)
#define SIG_IGN ((void (*)(int))1)
#define SIG_ERR ((void (*)(int))-1)

#define SA_ONSTACK   0x0001
#define SA_RESTART   0x0002
#define SA_RESETHAND 0x0004
#define SA_NOCLDSTOP 0x0008
#define SA_NODEFER   0x0010
#define SA_NOCLDWAIT 0x0020
#define SA_SIGINFO   0x0040

#define SIG_BLOCK   1
#define SIG_UNBLOCK 2
#define SIG_SETMASK 3

typedef struct {
	int si_dummy; /* SA_SIGINFO not supported by our trampoline -- see
	               * libc/src/signal.c; struct exists only so code that
	               * declares a siginfo_t pointer still compiles. */
} siginfo_t;

union __sigaction_u {
	void (*__sa_handler)(int);
	void (*__sa_sigaction)(int, siginfo_t *, void *);
};

/* Public POSIX struct sigaction (no sa_tramp -- that's an
 * implementation-private field added only in the raw syscall struct
 * below). */
struct sigaction {
	union __sigaction_u __sigaction_u;
	sigset_t sa_mask;
	int      sa_flags;
};
#define sa_handler   __sigaction_u.__sa_handler
#define sa_sigaction __sigaction_u.__sa_sigaction

/* Raw kernel-boundary struct for the sigaction(2) syscall -- never used
 * directly by callers, only inside libc/src/signal.c. */
struct __sigaction {
	union __sigaction_u __sigaction_u;
	void (*sa_tramp)(void *, int, int, siginfo_t *, void *);
	sigset_t sa_mask;
	int      sa_flags;
};

typedef void (*sig_t)(int);

int    sigaction(int sig, const struct sigaction *act, struct sigaction *oact);
sig_t  signal(int sig, sig_t handler);
int    kill(pid_t pid, int sig);
int    raise(int sig);
int    sigemptyset(sigset_t *set);
int    sigfillset(sigset_t *set);
int    sigaddset(sigset_t *set, int signo);
int    sigdelset(sigset_t *set, int signo);
int    sigismember(const sigset_t *set, int signo);
int    sigprocmask(int how, const sigset_t *set, sigset_t *oset);
int    sigsuspend(const sigset_t *sigmask);
int    sigpending(sigset_t *set);

#ifdef __cplusplus
}
#endif

#endif /* _SIGNAL_H_ */
