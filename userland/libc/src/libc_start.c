/* __libc_start: called from crt0.S with the raw argc/argv/envp off the
 * initial stack. Statically linked into every executable (never part of
 * libSystem.dylib -- see start.c) because it references that executable's
 * own `main`, which only exists once the final executable is linked.
 * `environ`/`exit`/`run_mod_init_funcs` are ordinary extern references,
 * satisfied locally for a static binary or dynamically against
 * libSystem.dylib for one built against dyld -- same mechanism either way.
 */
#include <unistd.h>
#include <stdlib.h>

extern const char *__libc_argv0;
void __init_stack_chk_guard(void);
void __init_default_rune_locale(void);

int main(int argc, char **argv, char **envp);

/* C++ global constructors (__DATA,__mod_init_func, an array of function
 * pointers the compiler emits calls into). ld64 synthesizes
 * section$start$/section$end$ symbols scoped to whichever image is being
 * linked -- since this file is always statically linked into the final
 * executable (never into libSystem.dylib itself, see start.c), these
 * correctly resolve to that executable's own section with no runtime
 * Mach-O parsing needed. */
extern void (*__mod_init_func_start[])(void) __asm("section$start$__DATA$__mod_init_func");
extern void (*__mod_init_func_end[])(void) __asm("section$end$__DATA$__mod_init_func");

static void
run_mod_init_funcs(void)
{
	size_t n = (size_t)(__mod_init_func_end - __mod_init_func_start);
	for (size_t i = 0; i < n; i++) {
		__mod_init_func_start[i]();
	}
}

void
__libc_start(int argc, char **argv, char **envp)
{
	__init_stack_chk_guard(); /* before anything stack-protector-instrumented runs */
	environ = envp;
	__libc_argv0 = argc > 0 ? argv[0] : "";
	__init_default_rune_locale();
	run_mod_init_funcs();
	int rc = main(argc, argv, envp);
	exit(rc);
}
