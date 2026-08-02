#include <signal.h>
#include <string.h>
#include "syscall_raw.h"

#define SYS_sigaction   46
#define SYS_kill        37
#define SYS_sigprocmask 48
#define SYS_sigsuspend  111

/* libc/src/sigtramp.S; only its address is ever taken (never called from
 * C), so the mismatched declared signature vs. real calling convention is
 * intentional -- cast at the call site below. */
void ___sigtramp(void *, int, int, siginfo_t *, void *);

int
sigaction(int sig, const struct sigaction *act, struct sigaction *oact)
{
	struct __sigaction ksa;
	struct sigaction kosa;
	long r;

	if (act) {
		memset(&ksa, 0, sizeof(ksa));
		ksa.__sigaction_u = act->__sigaction_u;
		ksa.sa_mask = act->sa_mask;
		ksa.sa_flags = act->sa_flags;
		/* SIG_DFL(0)/SIG_IGN(1) are kernel-recognized sentinels -- no
		 * trampoline is ever invoked for them. Only a real handler
		 * function needs sa_tramp. */
		ksa.sa_tramp = ___sigtramp;
	}
	r = sys_result(raw_syscall3(SYS_sigaction, sig, act ? (long)&ksa : 0, oact ? (long)&kosa : 0));
	if (r < 0) {
		return -1;
	}
	if (oact) {
		*oact = kosa;
	}
	return 0;
}

sig_t
signal(int sig, sig_t handler)
{
	struct sigaction sa, old;
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = handler;
	sa.sa_flags = 0;
	if (sigaction(sig, &sa, &old) < 0) {
		return SIG_ERR;
	}
	return old.sa_handler;
}

int
kill(pid_t pid, int sig)
{
	return (int)sys_result(raw_syscall3(SYS_kill, pid, sig, 0));
}

int
raise(int sig)
{
	extern pid_t getpid(void);
	return kill(getpid(), sig);
}

int
sigprocmask(int how, const sigset_t *set, sigset_t *oset)
{
	long r = sys_result(raw_syscall3(SYS_sigprocmask, how, set ? (long)*set : 0, 0));
	if (r < 0) {
		return -1;
	}
	if (oset) {
		*oset = 0; /* TODO: kernel doesn't hand back old mask via this
		            * path in our simplified call -- fix if a checklist
		            * command is found to depend on it. */
	}
	return 0;
}

/* Real syscall (ground-truthed against src/xnu/bsd/kern/syscalls.master
 * #111: `int sigsuspend(sigset_t mask)`) -- takes the mask by value, not
 * by pointer, unlike the POSIX wrapper signature. Blocks until a signal
 * outside `*sigmask` is delivered; the kernel always resumes with EINTR
 * (sigsuspend never "succeeds" in the normal sense), which time.c's
 * nanosleep() relies on to know its SIGALRM arrived. */
int
sigsuspend(const sigset_t *sigmask)
{
	if (!sigmask) {
		errno = 14; /* EFAULT */
		return -1;
	}
	return (int)sys_result(raw_syscall1(SYS_sigsuspend, (long)*sigmask));
}
int sigpending(sigset_t *set) { if (set) { *set = 0; } return 0; }

int
sigemptyset(sigset_t *set)
{
	*set = 0;
	return 0;
}
int
sigfillset(sigset_t *set)
{
	*set = ~(sigset_t)0;
	return 0;
}
int
sigaddset(sigset_t *set, int signo)
{
	*set |= (1u << (signo - 1));
	return 0;
}
int
sigdelset(sigset_t *set, int signo)
{
	*set &= ~(1u << (signo - 1));
	return 0;
}
int
sigismember(const sigset_t *set, int signo)
{
	return (*set >> (signo - 1)) & 1;
}
