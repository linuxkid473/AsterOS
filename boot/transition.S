/*
 * jump_to_kernel(entry_phys, boot_args_phys) -- noreturn.
 *
 * UEFI runs us in 64-bit long mode with paging on. xnu's entry point
 * (_pstart in osfmk/x86_64/start.s) requires the mirror image: 32-bit
 * legacy protected mode, paging OFF, flat (base=0, limit=4G) segments --
 * it does its own PAE/paging/long-mode setup completely from scratch. This
 * function performs that mode transition and hands off.
 *
 * Sequence follows Intel SDM Vol 3, 9.8.5.4 "Switching Out of IA-32e Mode":
 *   1. far-transfer into a 32-bit (compatibility mode) code segment
 *   2. reload data/stack segments with a flat 32-bit data segment
 *   3. clear CR0.PG (disable paging) -- the CPU auto-clears EFER.LMA as a
 *      side effect of doing this while EFER.LME=1, which is precisely the
 *      documented mechanism for leaving IA-32e mode from a 32-bit CS
 *   4. clear EFER.LME too, for a clean, unambiguous CPU state (not strictly
 *      required -- xnu's own SWITCH_TO_64BIT_MODE macro just ORs LME back
 *      in regardless of its prior value -- but leaves nothing ambiguous)
 *   5. load %eax with the boot_args physical address (xnu's very first
 *      instruction is `mov %eax, %edi` -- see osfmk/x86_64/start.s) and
 *      jump to the kernel's physical entry point
 *
 * MS x64 ABI (this whole bootloader targets x86_64-unknown-windows/EFI):
 * first two integer args arrive in %ecx, %edx.
 *
 * entry_phys/boot_args_phys are kept in %esi/%edi across the whole
 * transition -- general-purpose register contents aren't touched by any of
 * CS reload, CR0 write, or EFER write, so no memory access (and therefore
 * no addressing-mode concerns) is needed for them after the far transfer.
 */

	.text
	.globl jump_to_kernel
	.p2align 4
jump_to_kernel:
	cli
	movl	%ecx, %esi		/* entry_phys, preserved across the switch */
	movl	%edx, %edi		/* boot_args_phys, preserved across the switch */

	lea	Lgdt_desc(%rip), %rax
	lgdt	(%rax)

	/* Far-transfer into the 32-bit code segment (selector 0x08). A far
	 * jump/return is the documented way to change CS (and thus operating
	 * mode) from 64-bit code; we use lretq (far return): push the target
	 * selector, then the target offset, then let lretq pop offset-then-
	 * selector off the stack (stack grows down, so pushing selector
	 * first puts it at the higher address, popped second, matching the
	 * far-return convention of IP-then-CS). */
	pushq	$0x08
	lea	1f(%rip), %rax
	pushq	%rax
	lretq

	.code32
1:
	movw	$0x10, %ax		/* flat 32-bit data selector */
	movw	%ax, %ds
	movw	%ax, %es
	movw	%ax, %fs
	movw	%ax, %gs
	movw	%ax, %ss

	/* Disable paging -- with EFER.LME still 1 at this exact instant, this
	 * is the documented trigger for the CPU to auto-clear EFER.LMA and
	 * leave IA-32e mode (Intel SDM 9.8.5.4). */
	movl	%cr0, %eax
	andl	$0x7FFFFFFF, %eax
	movl	%eax, %cr0

	/* Clear EFER.LME too, for an unambiguous post-transition state. */
	movl	$0xC0000080, %ecx	/* MSR_EFER */
	rdmsr
	andl	$0xFFFFFEFF, %eax	/* clear bit 8 (LME) */
	wrmsr

	movl	%edi, %eax		/* boot_args phys addr -- xnu wants this in %eax */
	movl	%esi, %ecx		/* kernel entry phys addr */
	jmp	*%ecx

	.code64

	.align	16
Lgdt:
	.quad	0x0000000000000000	/* 0x00: null */
	.quad	0x00CF9A000000FFFF	/* 0x08: 32-bit code, base=0 limit=4G, G=1 D=1 */
	.quad	0x00CF92000000FFFF	/* 0x10: 32-bit data, base=0 limit=4G, G=1 D=1 */
Lgdt_end:

	.align	16
Lgdt_desc:
	.word	Lgdt_end - Lgdt - 1
	.quad	Lgdt
