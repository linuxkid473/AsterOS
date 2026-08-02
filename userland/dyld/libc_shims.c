/* Copyright (c) 2026 Vihaan Nathan
 *
 * We link syscalls.o directly (not through an archive), so ld64 pulls in
 * the whole translation unit -- including execv()/execvp(), which we
 * never call but which reference `environ`/getenv() (normally provided
 * by start.c, which we deliberately don't link: it pulls in __libc_start
 * and its own undefined reference to main()). Cheaper to satisfy the two
 * stray references directly than to link start.c for symbols dyld itself
 * never uses.
 */
char **environ;

char *
getenv(const char *name)
{
	(void)name;
	return 0;
}
