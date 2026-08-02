/*
 * Signal trampoline -- ground-truthed against
 * src/libplatform/src/setjmp/x86_64/_sigtramp.s (functional part only, we
 * drop the DWARF unwind tables since we have no debugger use case here).
 *
 * The kernel invokes this directly (never called from our own code) with:
 *   %rdi = handler function pointer (from sa_tramp's first union arg)
 *   %rsi = sigstyle (unused)
 *   %rdx = signum
 *   %rcx = siginfo_t *
 *   %r8  = ucontext_t *
 *   %r9  = token (opaque, must be echoed back to sigreturn verbatim)
 *
 * We call handler(signum, siginfo, uctx) -- a plain `void h(int)` handler
 * just ignores the extra two args per the C calling convention -- then
 * sigreturn(uctx, UC_FLAVOR, token) to resume. sigreturn does not return.
 */
	.text
	.globl ____sigtramp
____sigtramp:
	pushq	%rbp
	movq	%rsp, %rbp

	movq	%rdi, %rax   /* handler */
	movq	%r8, %rbx    /* save uctx */
	movq	%r9, %r12    /* save token */

	movl	%edx, %edi   /* signum */
	movq	%rcx, %rsi   /* siginfo */
	movq	%r8, %rdx    /* uctx */
	call	*%rax

	movq	%rbx, %rdi
	movl	$30, %esi    /* UC_FLAVOR */
	movq	%r12, %rdx
	movl	$0x2000000 | 184, %eax /* SYS_sigreturn */
	syscall
	ud2                  /* sigreturn does not return on success */
