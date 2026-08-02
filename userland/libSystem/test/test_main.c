/* End-to-end smoke test for the real libSystem.B.dylib: a normal,
 * dynamically-linked executable exercising printf/malloc/free and a
 * fork+waitpid round trip, all resolved at runtime through dyld's
 * rebase/bind/export-trie machinery against the dylib built by
 * userland/libSystem/build.sh -- not the empty placeholder Phase 11 used.
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int
main(void)
{
	printf("SYSTEST: libSystem printf works\n");

	char *buf = malloc(64);
	if (!buf) {
		printf("SYSTEST FAIL: malloc returned NULL\n");
		return 1;
	}
	for (int i = 0; i < 63; i++) {
		buf[i] = 'A' + (i % 26);
	}
	buf[63] = 0;
	printf("SYSTEST: malloc'd buffer: %s\n", buf);
	free(buf);

	pid_t pid = fork();
	if (pid < 0) {
		printf("SYSTEST FAIL: fork failed\n");
		return 1;
	}
	if (pid == 0) {
		_exit(42);
	}
	int status = 0;
	if (waitpid(pid, &status, 0) != pid) {
		printf("SYSTEST FAIL: waitpid didn't return child pid\n");
		return 1;
	}
	if (!WIFEXITED(status) || WEXITSTATUS(status) != 42) {
		printf("SYSTEST FAIL: child exit status wrong\n");
		return 1;
	}
	printf("SYSTEST: fork/waitpid round trip works\n");

	printf("SYSTEST PASS\n");
	return 0;
}
