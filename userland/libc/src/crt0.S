/*
 * Process entry point for our static, no-dyld Mach-O executables. The
 * kernel hands control here with the stack laid out per xnu's
 * exec_copyout_strings (bsd/kern/kern_exec.c):
 *   [argc][argv[0..argc-1]][NULL][envp[0..]][NULL][executable_path][pad]
 * (already ground-truthed for this exact kernel by userland/start.S).
 *
 * We now actually need argc/argv/envp (BusyBox is a real argv[0]-dispatched
 * multicall binary), so unlike the old start.S this crt0 reads them off the
 * stack and calls __libc_start(argc, argv, envp) which sets up `environ`
 * before calling main().
 */
	.text
	.globl _start
_start:
	xorq	%rbp, %rbp
	movq	(%rsp), %rdi          /* argc */
	leaq	8(%rsp), %rsi         /* argv */
	movq	%rdi, %rdx
	incq	%rdx
	leaq	(%rsi, %rdx, 8), %rdx /* envp = argv + (argc+1)*8 */
	andq	$-16, %rsp
	call	___libc_start
	/* __libc_start calls exit(); should never return, but be safe. */
	movl	$0x2000001, %eax
	xorl	%edi, %edi
	syscall
	hlt
