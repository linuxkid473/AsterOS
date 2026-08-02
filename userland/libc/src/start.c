/* __libc_start: called from crt0.S with the raw argc/argv/envp off the
 * initial stack (see crt0.S). Sets up `environ`, calls main(), then
 * exit()s with its return value -- the standard crt0 contract. */
#include <unistd.h>
#include <stdlib.h>

char **environ; /* the one authoritative definition -- see unistd.h */

/* argv[0] as seen at process start -- see mach-o/dyld.h's
 * _NSGetExecutablePath, our best-effort stand-in for real dyld image
 * tracking. */
const char *__libc_argv0;

int main(int argc, char **argv, char **envp);

#define MAX_ATEXIT 32
static void (*g_atexit_fns[MAX_ATEXIT])(void);
static int g_atexit_count;

int
atexit(void (*func)(void))
{
	if (g_atexit_count >= MAX_ATEXIT) {
		return -1;
	}
	g_atexit_fns[g_atexit_count++] = func;
	return 0;
}

/* __cxa_atexit/__cxa_finalize: the Itanium C++ ABI's registration
 * mechanism for global/static-object destructors (what the compiler
 * actually emits calls to, not atexit() -- real Darwin's libSystem
 * provides these, not libc++abi, so we must too). __dso_handle is
 * emitted per-TU by the compiler itself; we only ever have the one
 * image, so dso_handle is never actually used to distinguish callers,
 * matching a single-image process correctly. */
#define MAX_CXA_ATEXIT 64
static struct {
	void (*destructor)(void *);
	void *arg;
	void *dso_handle;
} g_cxa_atexit_fns[MAX_CXA_ATEXIT];
static int g_cxa_atexit_count;

int
__cxa_atexit(void (*destructor)(void *), void *arg, void *dso_handle)
{
	if (g_cxa_atexit_count >= MAX_CXA_ATEXIT) {
		return -1;
	}
	g_cxa_atexit_fns[g_cxa_atexit_count].destructor = destructor;
	g_cxa_atexit_fns[g_cxa_atexit_count].arg = arg;
	g_cxa_atexit_fns[g_cxa_atexit_count].dso_handle = dso_handle;
	g_cxa_atexit_count++;
	return 0;
}

void
__cxa_finalize(void *dso_handle)
{
	for (int i = g_cxa_atexit_count - 1; i >= 0; i--) {
		if (dso_handle && g_cxa_atexit_fns[i].dso_handle != dso_handle) {
			continue;
		}
		if (g_cxa_atexit_fns[i].destructor) {
			g_cxa_atexit_fns[i].destructor(g_cxa_atexit_fns[i].arg);
			g_cxa_atexit_fns[i].destructor = (void (*)(void *))0;
		}
	}
}

void
exit(int status)
{
	__cxa_finalize((void *)0);
	while (g_atexit_count > 0) {
		g_atexit_fns[--g_atexit_count]();
	}
	_exit(status);
	__builtin_unreachable();
}

void
_Exit(int status)
{
	_exit(status); /* unlike exit(), skips atexit handlers */
	__builtin_unreachable();
}

void
abort(void)
{
	_exit(134); /* 128 + SIGABRT(6), matching POSIX shell convention */
	__builtin_unreachable();
}

void __init_default_rune_locale(void);
void __init_stack_chk_guard(void);

/* C++ global constructors (registered by the compiler into the
 * __DATA,__mod_init_func section as an array of function pointers) --
 * with no dyld, nothing has ever walked this section, so every global
 * object with a constructor (LLVM's many ManagedStatic/cl::opt
 * registries included) was silently never initialized. ld64
 * synthesizes section$start$/section$end$ symbols for any section, so
 * this needs no runtime Mach-O header parsing. */
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
