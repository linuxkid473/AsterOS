/* Plain setjmp/longjmp -- see setjmp.h for why this isn't Apple's real
 * ABI. jmp_buf layout: [0]=rbx [1]=rbp [2]=rsp [3]=r12 [4]=r13 [5]=r14
 * [6]=r15 [7]=rip. sigsetjmp/siglongjmp are aliases (no signal mask
 * save/restore -- we don't need it). */
	.text

	.globl _setjmp
_setjmp:
	movq	%rbx, 0(%rdi)
	movq	%rbp, 8(%rdi)
	leaq	8(%rsp), %rax   /* rsp as it will be right after our `ret` */
	movq	%rax, 16(%rdi)
	movq	%r12, 24(%rdi)
	movq	%r13, 32(%rdi)
	movq	%r14, 40(%rdi)
	movq	%r15, 48(%rdi)
	movq	(%rsp), %rax    /* return address */
	movq	%rax, 56(%rdi)
	xorl	%eax, %eax
	ret

	.globl _longjmp
_longjmp:
	movl	%esi, %eax
	testl	%eax, %eax
	jne	1f
	movl	$1, %eax
1:
	movq	0(%rdi), %rbx
	movq	8(%rdi), %rbp
	movq	24(%rdi), %r12
	movq	32(%rdi), %r13
	movq	40(%rdi), %r14
	movq	48(%rdi), %r15
	movq	16(%rdi), %rsp
	jmpq	*56(%rdi)

	.globl _sigsetjmp
_sigsetjmp:
	jmp	_setjmp

	.globl _siglongjmp
_siglongjmp:
	jmp	_longjmp
