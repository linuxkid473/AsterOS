/* Macros ground-truthed against src/xnu/bsd/sys/wait.h. */
#ifndef _SYS_WAIT_H_
#define _SYS_WAIT_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/types.h>
#include <sys/resource.h> /* [XSI] for struct rusage -- matches real Darwin's sys/wait.h */

#define WNOHANG   0x00000001
#define WUNTRACED 0x00000002

#define _WSTOPPED 0177
#define _W_INT(x) (x)
#define _WSTATUS(x) (_W_INT(x) & 0177)

#define WEXITSTATUS(x)  ((_W_INT(x) >> 8) & 0x000000ff)
#define WSTOPSIG(x)     (_W_INT(x) >> 8)
#define WIFCONTINUED(x) (_WSTATUS(x) == _WSTOPPED && WSTOPSIG(x) == 0x13)
#define WIFSTOPPED(x)   (_WSTATUS(x) == _WSTOPPED && WSTOPSIG(x) != 0x13)
#define WIFEXITED(x)    (_WSTATUS(x) == 0)
#define WIFSIGNALED(x)  (_WSTATUS(x) != _WSTOPPED && _WSTATUS(x) != 0)
#define WTERMSIG(x)     (_WSTATUS(x))
#define WCOREFLAG       0200
#define WCOREDUMP(x)    (_W_INT(x) & WCOREFLAG)

#define WAIT_ANY    (-1)
#define WAIT_MYPGRP 0

pid_t wait(int *status);
pid_t waitpid(pid_t pid, int *status, int options);
pid_t wait4(pid_t pid, int *status, int options, void *rusage);

#ifdef __cplusplus
}
#endif

#endif /* _SYS_WAIT_H_ */
