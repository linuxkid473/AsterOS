/* No stack-unwinding-for-diagnostics support wired up yet (libunwind is
 * built and available -- see build/runtimes-install -- but nothing
 * here calls into it for this). backtrace() honestly reports "no
 * frames captured" rather than fabricating a trace. TODO: implement
 * for real via libunwind's unw_step if crash backtraces ever matter
 * here. */
#ifndef _EXECINFO_H_
#define _EXECINFO_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/types.h>

int backtrace(void **buffer, int size);
char **backtrace_symbols(void *const *buffer, int size);
void backtrace_symbols_fd(void *const *buffer, int size, int fd);

#ifdef __cplusplus
}
#endif

#endif /* _EXECINFO_H_ */
