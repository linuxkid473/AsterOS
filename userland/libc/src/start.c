/* Runtime support __libc_start (crt0.S's callee) needs: environ storage,
 * atexit/__cxa_atexit machinery, exit/abort, and the __DATA,__mod_init_func
 * runner. Split out of __libc_start itself (see libc_start.c) so this half
 * can live in libSystem.dylib while __libc_start -- the one piece that
 * references the executable's own `main` symbol directly -- stays a
 * statically-linked-per-executable object; a dylib can never resolve a
 * reference to whatever `main` some future caller happens to define. */
#include <unistd.h>
#include <stdlib.h>

char **environ; /* the one authoritative definition -- see unistd.h */

/* argv[0] as seen at process start -- see mach-o/dyld.h's
 * _NSGetExecutablePath, our best-effort stand-in for real dyld image
 * tracking. */
const char *__libc_argv0;

/* Full argc/argv, added for NSProcessInfo (userland/Foundation) --
 * same reasoning as __libc_argv0 just above: real storage lives here
 * (shared, libSystem.dylib-resident) so both static and dynamically
 * linked executables can see it via extern, assigned from
 * __libc_start() in libc_start.c, which alone has the real values off
 * the initial stack. */
int __libc_argc;
char **__libc_argv;

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

/* run_mod_init_funcs() (C++ global constructor support) lives in
 * libc_start.c, not here: ld64's section$start$/section$end$ symbols are
 * scoped to the image being linked, so that function must be compiled
 * into whichever object is statically linked per-executable (like
 * __libc_start itself) to see *that executable's* mod-init section --
 * a copy living in this dylib-bound file would only ever see
 * libSystem.dylib's own (empty) section instead. */
