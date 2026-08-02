/* Copyright (c) 2026 Vihaan Nathan
 *
 * objc_msgSend / objc_msgSendSuper: the SysV x86_64 argument registers
 * (rdi,rsi,rdx,rcx,r8,r9, xmm0-7) are saved verbatim before calling out
 * to objc_class_lookup_and_cache (a plain C function, so it clobbers all
 * of those per the ABI), then restored and we `jmp` (not `call`) to the
 * resolved IMP -- a tail call, so any stack-passed arguments beyond the
 * register set are untouched and land exactly where the real method
 * expects them, and the real method's own `ret` returns straight to our
 * original caller.
 *
 * The lookup+cache logic itself lives in dispatch.c, not here: real
 * objc_msgSend inlines its cache probe in asm for speed, but this
 * project's stated priority is Apple-compatible *behavior*, not
 * performance (see TODO.md Phase 13) -- every send pays a real function
 * call, which is slower but far less likely to hide a subtle asm bug.
 */
	.text

	.globl _objc_msgSend
_objc_msgSend:
	testq	%rdi, %rdi
	je	Lmsg_nil

	pushq	%rbp
	movq	%rsp, %rbp
	andq	$-16, %rsp
	subq	$128, %rsp

	movq	%rdi, 0(%rsp)
	movq	%rsi, 8(%rsp)
	movq	%rdx, 16(%rsp)
	movq	%rcx, 24(%rsp)
	movq	%r8, 32(%rsp)
	movq	%r9, 40(%rsp)
	movq	%xmm0, 48(%rsp)
	movq	%xmm1, 56(%rsp)
	movq	%xmm2, 64(%rsp)
	movq	%xmm3, 72(%rsp)
	movq	%xmm4, 80(%rsp)
	movq	%xmm5, 88(%rsp)
	movq	%xmm6, 96(%rsp)
	movq	%xmm7, 104(%rsp)

	movq	0(%rsp), %rdi
	movq	(%rdi), %rdi		/* self->isa */
	movq	8(%rsp), %rsi		/* _cmd */
	call	_objc_class_lookup_and_cache
	movq	%rax, %r10

	movq	0(%rsp), %rdi
	movq	8(%rsp), %rsi
	movq	16(%rsp), %rdx
	movq	24(%rsp), %rcx
	movq	32(%rsp), %r8
	movq	40(%rsp), %r9
	movq	48(%rsp), %xmm0
	movq	56(%rsp), %xmm1
	movq	64(%rsp), %xmm2
	movq	72(%rsp), %xmm3
	movq	80(%rsp), %xmm4
	movq	88(%rsp), %xmm5
	movq	96(%rsp), %xmm6
	movq	104(%rsp), %xmm7

	movq	%rbp, %rsp
	popq	%rbp
	jmpq	*%r10

Lmsg_nil:
	xorq	%rax, %rax
	retq

	/* struct objc_super { id receiver; Class super_class; }: looks the
	 * method up starting at super_class (skipping the receiver's own
	 * dynamic class -- what `[super foo]` needs), but the resolved IMP
	 * still gets called with the *receiver* as self, per Darwin's real
	 * super-call semantics. */
	.globl _objc_msgSendSuper
_objc_msgSendSuper:
	movq	(%rdi), %r11		/* super->receiver */
	testq	%r11, %r11
	je	Lmsgsuper_nil

	pushq	%rbp
	movq	%rsp, %rbp
	andq	$-16, %rsp
	subq	$128, %rsp

	movq	8(%rdi), %r10		/* super->super_class */

	movq	%r11, 0(%rsp)		/* real self, for the eventual call */
	movq	%rsi, 8(%rsp)		/* _cmd */
	movq	%rdx, 16(%rsp)
	movq	%rcx, 24(%rsp)
	movq	%r8, 32(%rsp)
	movq	%r9, 40(%rsp)
	movq	%xmm0, 48(%rsp)
	movq	%xmm1, 56(%rsp)
	movq	%xmm2, 64(%rsp)
	movq	%xmm3, 72(%rsp)
	movq	%xmm4, 80(%rsp)
	movq	%xmm5, 88(%rsp)
	movq	%xmm6, 96(%rsp)
	movq	%xmm7, 104(%rsp)

	movq	%r10, %rdi		/* lookup starts at super_class, not receiver->isa */
	movq	8(%rsp), %rsi
	call	_objc_class_lookup_and_cache
	movq	%rax, %r10

	movq	0(%rsp), %rdi
	movq	8(%rsp), %rsi
	movq	16(%rsp), %rdx
	movq	24(%rsp), %rcx
	movq	32(%rsp), %r8
	movq	40(%rsp), %r9
	movq	48(%rsp), %xmm0
	movq	56(%rsp), %xmm1
	movq	64(%rsp), %xmm2
	movq	72(%rsp), %xmm3
	movq	80(%rsp), %xmm4
	movq	88(%rsp), %xmm5
	movq	96(%rsp), %xmm6
	movq	104(%rsp), %xmm7

	movq	%rbp, %rsp
	popq	%rbp
	jmpq	*%r10

Lmsgsuper_nil:
	xorq	%rax, %rax
	retq

	/* objc_msgSendSuper2: what modern clang actually emits for
	 * [super foo] (ground-truthed -- see TODO.md Phase 13's probe
	 * disassembly). Same struct layout as objc_super, but the second
	 * field ("current_class") is the class whose method body is
	 * currently executing, NOT the class to start searching at -- the
	 * compiler can emit a direct reference to that class's own symbol
	 * this way instead of needing a separate "superclass of X" symbol.
	 * We dereference current_class->superclass (class_t.superclass is
	 * at +8) ourselves before doing the lookup. */
	.globl _objc_msgSendSuper2
_objc_msgSendSuper2:
	movq	(%rdi), %r11		/* super->receiver */
	testq	%r11, %r11
	je	Lmsgsuper2_nil

	pushq	%rbp
	movq	%rsp, %rbp
	andq	$-16, %rsp
	subq	$128, %rsp

	movq	8(%rdi), %r10		/* super->current_class */
	movq	8(%r10), %r10		/* -> current_class->superclass */

	movq	%r11, 0(%rsp)
	movq	%rsi, 8(%rsp)
	movq	%rdx, 16(%rsp)
	movq	%rcx, 24(%rsp)
	movq	%r8, 32(%rsp)
	movq	%r9, 40(%rsp)
	movq	%xmm0, 48(%rsp)
	movq	%xmm1, 56(%rsp)
	movq	%xmm2, 64(%rsp)
	movq	%xmm3, 72(%rsp)
	movq	%xmm4, 80(%rsp)
	movq	%xmm5, 88(%rsp)
	movq	%xmm6, 96(%rsp)
	movq	%xmm7, 104(%rsp)

	movq	%r10, %rdi
	movq	8(%rsp), %rsi
	call	_objc_class_lookup_and_cache
	movq	%rax, %r10

	movq	0(%rsp), %rdi
	movq	8(%rsp), %rsi
	movq	16(%rsp), %rdx
	movq	24(%rsp), %rcx
	movq	32(%rsp), %r8
	movq	40(%rsp), %r9
	movq	48(%rsp), %xmm0
	movq	56(%rsp), %xmm1
	movq	64(%rsp), %xmm2
	movq	72(%rsp), %xmm3
	movq	80(%rsp), %xmm4
	movq	88(%rsp), %xmm5
	movq	96(%rsp), %xmm6
	movq	104(%rsp), %xmm7

	movq	%rbp, %rsp
	popq	%rbp
	jmpq	*%r10

Lmsgsuper2_nil:
	xorq	%rax, %rax
	retq
