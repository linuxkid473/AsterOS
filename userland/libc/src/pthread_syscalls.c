/* Real bsdthread_create(2)/bsdthread_register(2)/bsdthread_terminate(2)
 * raw syscall wrappers -- the genuine kernel entry points real pthreads
 * are built on (see pthread.c and docs on the AsterOS libpthread_kern
 * additions to xnu). Ground-truthed against src/xnu/bsd/kern/
 * syscalls.master and the vendored src/libpthread/src/pthread.c's own
 * __bsdthread_create()/__bsdthread_register() declarations. */
#include "pthread_internal.h"
#include "syscall_raw.h"

int
__bsdthread_register(void *threadstart, void *wqthread, uint32_t flags,
    void *stack_addr_hint, void *targetconc_ptr, uint32_t dispatchqueue_offset,
    uint32_t tsd_offset)
{
	long r = raw_syscall7(SYS_bsdthread_register, (long)threadstart,
	    (long)wqthread, (long)flags, (long)stack_addr_hint,
	    (long)targetconc_ptr, (long)dispatchqueue_offset, (long)tsd_offset);
	return (int)sys_result(r);
}

long
__bsdthread_create(void *(*func)(void *), void *func_arg, void *stack,
    void *pthread, unsigned int flags)
{
	long r = raw_syscall5(SYS_bsdthread_create, (long)func, (long)func_arg,
	    (long)stack, (long)pthread, (long)flags);
	return sys_result(r);
}

int
__bsdthread_terminate(void *stackaddr, size_t freesize, unsigned int port,
    unsigned int sem)
{
	long r = raw_syscall4(SYS_bsdthread_terminate, (long)stackaddr,
	    (long)freesize, (long)port, (long)sem);
	return (int)sys_result(r);
}
