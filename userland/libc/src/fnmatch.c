/* Minimal shell-glob matcher: *, ?, [...] (with negation via ! or ^, and
 * simple ranges). Enough for busybox's internal uses -- not a full POSIX
 * fnmatch (no [[:class:]] collating support). */
#include <fnmatch.h>

static int
lower(int c)
{
	return (c >= 'A' && c <= 'Z') ? c - 'A' + 'a' : c;
}

static int
match_class(const char **pp, char c, int fold)
{
	const char *p = *pp + 1; /* skip '[' */
	int neg = 0;
	if (*p == '!' || *p == '^') {
		neg = 1;
		p++;
	}
	int matched = 0;
	int first = 1;
	while (*p && (*p != ']' || first)) {
		first = 0;
		int lo = (unsigned char)*p++;
		int hi = lo;
		if (*p == '-' && p[1] && p[1] != ']') {
			p++;
			hi = (unsigned char)*p++;
		}
		int cc = fold ? lower((unsigned char)c) : (unsigned char)c;
		int l = fold ? lower(lo) : lo;
		int h = fold ? lower(hi) : hi;
		if (cc >= l && cc <= h) {
			matched = 1;
		}
	}
	if (*p == ']') {
		p++;
	}
	*pp = p;
	return neg ? !matched : matched;
}

static int
do_match(const char *pat, const char *s, int flags)
{
	int fold = (flags & FNM_CASEFOLD) != 0;
	while (*pat) {
		if (*pat == '*') {
			while (*pat == '*') {
				pat++;
			}
			if (!*pat) {
				return 1;
			}
			for (; *s; s++) {
				if (do_match(pat, s, flags)) {
					return 1;
				}
			}
			return 0;
		}
		if (!*s) {
			return 0;
		}
		if (*pat == '?') {
			pat++;
			s++;
			continue;
		}
		if (*pat == '[') {
			const char *save = pat;
			if (!match_class(&pat, *s, fold)) {
				return 0;
			}
			(void)save;
			s++;
			continue;
		}
		if ((flags & FNM_NOESCAPE) == 0 && *pat == '\\' && pat[1]) {
			pat++;
		}
		int pc = fold ? lower((unsigned char)*pat) : (unsigned char)*pat;
		int sc = fold ? lower((unsigned char)*s) : (unsigned char)*s;
		if (pc != sc) {
			return 0;
		}
		pat++;
		s++;
	}
	return *s == 0;
}

int
fnmatch(const char *pattern, const char *string, int flags)
{
	return do_match(pattern, string, flags) ? 0 : FNM_NOMATCH;
}
