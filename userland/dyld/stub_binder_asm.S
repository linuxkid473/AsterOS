/* Copyright (c) 2026 Vihaan Nathan
 *
 * Lazy-binding entry point. ld64's x86_64 stub sequence (ground-truthed
 * against src/ld64/src/ld/passes/stubs/stub_x86_64.hpp -- we can't change
 * this, it's what real ld64 emits) is, per external call site:
 *
 *   stub:        jmp   *foo$lazy_ptr(%rip)      ; initially points into stub_helper
 *   stub_helper: pushq $<lazy_bind_offset>       ; one of these per symbol
 *                jmp   helperhelper
 *   helperhelper:leaq  dyld_ImageLoaderCache(%rip), %r11   ; shared, once per binary
 *                pushq %r11
 *                jmp   *_fast_lazy_bind(%rip)
 *
 * `_fast_lazy_bind` is a linker-synthesized data slot that gets eagerly
 * bound (regular, non-lazy BIND_OPCODE, symbol name "dyld_stub_binder") to
 * this symbol's address -- see bind.c's resolve_symbol() special case.
 *
 * Real dyld's `dyld_ImageLoaderCache` slot exists purely as a perf cache
 * so dyld doesn't have to re-derive "which image is this stub in" every
 * call; we don't need that optimization, so we ignore its pushed value
 * entirely and instead identify the caller's image by which loaded
 * image's segments contain the return address one frame up -- same
 * externally observable result, no need to replicate dyld's internal
 * cache-slot contract.
 */
	.text
	.globl __dyld_stub_binder_entry
__dyld_stub_binder_entry:
	/* stack on entry: [0]=imageCache(unused) [8]=lazy_offset [16]=orig retaddr */
	pushq	%rax
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%r8
	pushq	%r9
	movl	64(%rsp), %edi		/* lazy_offset (arg1) */
	movq	72(%rsp), %rsi		/* return address (arg2) */
	call	_dyld_stub_binder_resolve
	movq	%rax, %r11		/* r11 is caller-clobbered by convention (see helperhelper above too) */
	popq	%r9
	popq	%r8
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	popq	%rax
	addq	$16, %rsp		/* drop imageCache + lazy_offset, leaving just orig retaddr */
	jmpq	*%r11
