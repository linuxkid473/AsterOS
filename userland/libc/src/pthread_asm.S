/*
 * Real kernel thread entry trampolines for pthread_create().
 *
 * _pthread_start is where the kernel actually starts executing every
 * thread created via bsdthread_create(2) -- the address registered via
 * bsdthread_register(2) as `threadstart`. Ground-truthed against
 * src/xnu/bsd/pthread/libpthread_kern/kern/kern_support.c's
 * _bsdthread_create() x86_64 register setup:
 *   rdi = user_pthread (the struct __pthread* we passed in)
 *   rsi = th_thport    (mach port name for the new kernel thread)
 *   rdx = user_func    (start_routine, unused here -- stashed in the
 *                        struct by pthread_create() instead)
 *   rcx = user_funcarg (arg, likewise unused here)
 *   r8  = user_stack   (top of the stack we mmap'd)
 *   r9  = flags
 *   rsp = user_stack   (the kernel also points the stack pointer here,
 *                        so we're already running on our own stack)
 * rdi/rsi already match the C calling convention for
 * __pthread_trampoline_c(struct __pthread *t, unsigned int kport), so no
 * argument shuffling is needed -- just align the stack (user_stack's
 * alignment isn't guaranteed) and call in.
 */
	.text
	.globl __pthread_start
__pthread_start:
	andq	$-16, %rsp
	call	___pthread_trampoline_c
	/* __pthread_trampoline_c is noreturn (ends in bsdthread_terminate);
	 * ud2 as a hard stop if that guarantee is ever violated. */
	ud2

/* bsdthread_register(2) requires a non-NULL wqthread pointer, but this
 * project never issues workq_open()/workq_kernreturn() (this xnu's own
 * pthread_workqueue.c runs the GCD-style workqueue thread pool entirely
 * in-kernel and has no callback path back out through pthread_functions
 * for it -- see the libpthread_kern notes in TODO.md), so the kernel
 * should never actually jump here. If it somehow does, exit loudly
 * rather than run on an unexpected stack with unexpected registers. */
	.globl __pthread_wqthread
__pthread_wqthread:
	movl	$0x2000001, %eax   /* SYS_exit */
	movl	$66, %edi           /* arbitrary distinct status */
	syscall
	hlt
