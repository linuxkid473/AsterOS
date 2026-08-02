/* Copyright (c) 2026 Vihaan Nathan
 *
 * End-to-end validation for dyld: dyntest_add exercises a lazily-bound
 * external function call (the __stub_helper/dyld_stub_binder path),
 * dyntest_greeting exercises binding + rebasing an external data symbol.
 */
#include <unistd.h>
#include <string.h>

extern int dyntest_add(int a, int b);
extern const char *dyntest_greeting;

int
main(void)
{
	int sum = dyntest_add(40, 2);
	const char *msg = (sum == 42 && dyntest_greeting)
	    ? "DYNTEST PASS\n"
	    : "DYNTEST FAIL\n";
	write(1, msg, strlen(msg));
	if (dyntest_greeting) {
		write(1, dyntest_greeting, strlen(dyntest_greeting));
		write(1, "\n", 1);
	}
	return sum == 42 ? 0 : 1;
}
