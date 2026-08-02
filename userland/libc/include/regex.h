/* Stub -- only referenced by busybox's coreutils/test.c under
 * `#if BASH_TEST2`, which is compiled out (CONFIG_ASH_BASH_COMPAT=n in
 * our .config), so these declarations only need to exist for the
 * preprocessor/type-checker; the .c stub implementations always report
 * failure and are never actually called. */
#ifndef _REGEX_H_
#define _REGEX_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/types.h>

typedef struct {
	int re_dummy;
} regex_t;

typedef long regoff_t;

typedef struct {
	regoff_t rm_so;
	regoff_t rm_eo;
} regmatch_t;

#define REG_EXTENDED 1
#define REG_ICASE    2
#define REG_NEWLINE  4
#define REG_NOSUB    8
#define REG_NOMATCH  1

int regcomp(regex_t *preg, const char *pattern, int cflags);
int regexec(const regex_t *preg, const char *string, size_t nmatch, regmatch_t pmatch[], int eflags);
void regfree(regex_t *preg);
size_t regerror(int errcode, const regex_t *preg, char *errbuf, size_t errbuf_size);

#ifdef __cplusplus
}
#endif

#endif /* _REGEX_H_ */
