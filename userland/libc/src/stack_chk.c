/* Stack-protector runtime support (referenced by any TU compiled with
 * -fstack-protector, which clang's own build uses by default in some
 * configurations) -- a real canary seeded from kernel entropy, not a
 * fixed/predictable value, and a real abort on mismatch.
 *
 * Seeded from __libc_start() (see start.c), not a Mach-O
 * __mod_init_func constructor -- this environment's crt0.S jumps
 * straight to __libc_start with no mod_init_func walking, so a
 * constructor-attribute function here would silently never run. */
#include <sys/random.h>
#include <stdlib.h>
#include <stdint.h>

uintptr_t __stack_chk_guard = 0;

void
__init_stack_chk_guard(void)
{
	getentropy(&__stack_chk_guard, sizeof(__stack_chk_guard));
	if (__stack_chk_guard == 0) {
		__stack_chk_guard = 1;
	}
}

void
__stack_chk_fail(void)
{
	abort();
}
