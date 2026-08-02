/*
 * Process entry point for our static, no-dyld Mach-O executable. The
 * kernel hands control here with argc/argv/envp/executable-path laid out
 * on the initial stack per the standard xnu convention (see
 * bsd/kern/kern_exec.c's exec_copyout_strings): [argc][argv[0..argc-1]]
 * [NULL][envp[0..]][NULL][executable_path][NULL padding].
 *
 * We don't actually need argc/argv (this binary never forks/execs another
 * program -- every command is a builtin, see shell.c), but %rsp must be
 * 16-byte aligned before the `call` per the SysV x86_64 ABI, and the stack
 * as handed to us already satisfies that (it's set up the same way for
 * every process, dyld-based or not) -- so just call straight into main().
 */
	.text
	.globl _start
_start:
	xorq	%rbp, %rbp      /* mark the bottom of the frame chain */
	andq	$-16, %rsp      /* enforce 16-byte alignment defensively */
	call	_main
	/* main() never returns (it loops in sys_exit or an infinite shell
	 * loop), but if it somehow did, exit cleanly rather than run off
	 * into whatever garbage follows. */
	movl	$0x2000001, %eax  /* SYS_exit, class 2 */
	xorl	%edi, %edi
	syscall
	hlt
