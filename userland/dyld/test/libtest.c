/* Copyright (c) 2026 Vihaan Nathan
 *
 * Minimal exported symbol + a __DATA global, to exercise the bind and
 * rebase paths respectively when dyntest links against this dylib.
 */
int dyntest_add(int a, int b);
extern const char *dyntest_greeting;

const char *dyntest_greeting = "hello from libtest.dylib";

int
dyntest_add(int a, int b)
{
	return a + b;
}
