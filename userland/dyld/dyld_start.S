/* Copyright (c) 2026 Vihaan Nathan
 *
 * Entry point for our dyld (MH_DYLINKER). Ground-truthed against
 * src/xnu/bsd/kern/kern_exec.c's dylinker hand-off path: after
 * exec_copyout_strings() lays down the usual [argc][argv][envp][apple]
 * stack (same layout our normal crt0.S reads), exec_activate_image()
 * additionally does `thread_adjuserstack(-8)` and copyoutptr()s the MAIN
 * executable's (unslid-by-us, kernel-mapped) mach_header address into that
 * new slot -- so unlike a normal _start, %rsp here points one word below
 * where argc actually lives. No register carries this pointer; it is only
 * ever on the stack (kern_exec.c never touches rdx/r8/etc for this case).
 */
	.text
	.globl _dyld_start
_dyld_start:
	popq	%rdi			/* main executable's mach_header * (arg1) */
	movq	%rsp, %rbx		/* save argc pointer -- need it back after the call, rbx is callee-saved */
	movq	(%rsp), %rsi		/* argc (arg2) */
	leaq	8(%rsp), %rdx		/* argv (arg3) */
	movq	%rsi, %rax
	incq	%rax
	leaq	(%rdx, %rax, 8), %rcx	/* envp = argv + (argc+1)*8 (arg4) */
	andq	$-16, %rsp
	call	_dyld_bootstrap_main
	/* rax = resolved entry point of the main executable. Restore the
	 * stack exactly as the kernel handed it to us so the main binary's
	 * own crt0 sees the same layout it would with no dyld involved. */
	movq	%rbx, %rsp
	xorq	%rbp, %rbp
	jmpq	*%rax
	hlt
