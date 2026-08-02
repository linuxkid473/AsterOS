/* See regex.h -- unreachable in our config (BASH_TEST2 compiled out),
 * these exist only to satisfy the linker if that ever changes. */
#include <regex.h>
#include <string.h>

int regcomp(regex_t *preg, const char *pattern, int cflags) { (void)preg; (void)pattern; (void)cflags; return 1; }
int regexec(const regex_t *preg, const char *string, size_t nmatch, regmatch_t pmatch[], int eflags)
{
	(void)preg; (void)string; (void)nmatch; (void)pmatch; (void)eflags;
	return REG_NOMATCH;
}
void regfree(regex_t *preg) { (void)preg; }
size_t regerror(int errcode, const regex_t *preg, char *errbuf, size_t errbuf_size)
{
	(void)errcode; (void)preg;
	if (errbuf_size) {
		strlcpy(errbuf, "regex not supported", errbuf_size);
	}
	return 0;
}
