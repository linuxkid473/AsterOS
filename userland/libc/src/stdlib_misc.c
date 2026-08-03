#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h> /* environ (single authoritative definition in start.c) */
#include <sys/random.h>
#include <limits.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>

char *
getenv(const char *name)
{
	if (!environ) {
		return (void *)0;
	}
	size_t nlen = strlen(name);
	for (char **e = environ; *e; e++) {
		if (strncmp(*e, name, nlen) == 0 && (*e)[nlen] == '=') {
			return *e + nlen + 1;
		}
	}
	return (void *)0;
}

/* Growable-by-replacement environ: we never try to preserve the original
 * kernel-provided envp array in place (it isn't ours to resize), instead
 * we lazily copy it into a malloc'd array the first time it's mutated. */
static int g_env_owned;
static int g_env_cap;

static void
env_take_ownership(void)
{
	if (g_env_owned) {
		return;
	}
	int n = 0;
	if (environ) {
		while (environ[n]) {
			n++;
		}
	}
	int cap = n + 16;
	char **copy = malloc((cap + 1) * sizeof(char *));
	for (int i = 0; i < n; i++) {
		copy[i] = environ[i];
	}
	copy[n] = (void *)0;
	environ = copy;
	g_env_cap = cap;
	g_env_owned = 1;
}

int
setenv(const char *name, const char *value, int overwrite)
{
	env_take_ownership();
	size_t nlen = strlen(name);
	int n = 0;
	while (environ[n]) {
		if (strncmp(environ[n], name, nlen) == 0 && environ[n][nlen] == '=') {
			if (!overwrite) {
				return 0;
			}
			size_t need = nlen + 1 + strlen(value) + 1;
			char *entry = malloc(need);
			if (!entry) {
				return -1;
			}
			memcpy(entry, name, nlen);
			entry[nlen] = '=';
			strcpy(entry + nlen + 1, value);
			environ[n] = entry;
			return 0;
		}
		n++;
	}
	if (n + 1 >= g_env_cap) {
		int newcap = g_env_cap * 2 + 16;
		char **grown = malloc((newcap + 1) * sizeof(char *));
		for (int i = 0; i <= n; i++) {
			grown[i] = environ[i];
		}
		environ = grown;
		g_env_cap = newcap;
	}
	size_t need = nlen + 1 + strlen(value) + 1;
	char *entry = malloc(need);
	if (!entry) {
		return -1;
	}
	memcpy(entry, name, nlen);
	entry[nlen] = '=';
	strcpy(entry + nlen + 1, value);
	environ[n] = entry;
	environ[n + 1] = (void *)0;
	return 0;
}

int
unsetenv(const char *name)
{
	env_take_ownership();
	size_t nlen = strlen(name);
	int n = 0;
	while (environ[n]) {
		if (strncmp(environ[n], name, nlen) == 0 && environ[n][nlen] == '=') {
			int j = n;
			while (environ[j]) {
				environ[j] = environ[j + 1];
				j++;
			}
			return 0;
		}
		n++;
	}
	return 0;
}

int
putenv(char *string)
{
	char *eq = strchr(string, '=');
	if (!eq) {
		return unsetenv(string);
	}
	*eq = 0;
	int r = setenv(string, eq + 1, 1);
	*eq = '=';
	return r;
}

long
strtol(const char *nptr, char **endptr, int base)
{
	const char *p = nptr;
	while (isspace((unsigned char)*p)) {
		p++;
	}
	int neg = 0;
	if (*p == '+' || *p == '-') {
		neg = (*p == '-');
		p++;
	}
	if ((base == 0 || base == 16) && p[0] == '0' && (p[1] == 'x' || p[1] == 'X')) {
		base = 16;
		p += 2;
	} else if (base == 0 && p[0] == '0') {
		base = 8;
	} else if (base == 0) {
		base = 10;
	}
	long val = 0;
	const char *start = p;
	for (; *p; p++) {
		int d;
		if (*p >= '0' && *p <= '9') {
			d = *p - '0';
		} else if (*p >= 'a' && *p <= 'z') {
			d = *p - 'a' + 10;
		} else if (*p >= 'A' && *p <= 'Z') {
			d = *p - 'A' + 10;
		} else {
			break;
		}
		if (d >= base) {
			break;
		}
		val = val * base + d;
	}
	if (endptr) {
		*endptr = (char *)((p == start) ? nptr : p);
	}
	return neg ? -val : val;
}

unsigned long
strtoul(const char *nptr, char **endptr, int base)
{
	const char *p = nptr;
	while (isspace((unsigned char)*p)) {
		p++;
	}
	int neg = 0;
	if (*p == '+' || *p == '-') {
		neg = (*p == '-');
		p++;
	}
	if ((base == 0 || base == 16) && p[0] == '0' && (p[1] == 'x' || p[1] == 'X')) {
		base = 16;
		p += 2;
	} else if (base == 0 && p[0] == '0') {
		base = 8;
	} else if (base == 0) {
		base = 10;
	}
	unsigned long val = 0;
	const char *start = p;
	for (; *p; p++) {
		int d;
		if (*p >= '0' && *p <= '9') {
			d = *p - '0';
		} else if (*p >= 'a' && *p <= 'z') {
			d = *p - 'a' + 10;
		} else if (*p >= 'A' && *p <= 'Z') {
			d = *p - 'A' + 10;
		} else {
			break;
		}
		if (d >= base) {
			break;
		}
		val = val * base + (unsigned long)d;
	}
	if (endptr) {
		*endptr = (char *)((p == start) ? nptr : p);
	}
	return neg ? -val : val;
}

long long strtoll(const char *nptr, char **endptr, int base) { return strtol(nptr, endptr, base); }
unsigned long long strtoull(const char *nptr, char **endptr, int base) { return strtoul(nptr, endptr, base); }

/* Was a hard 0.0 stub until Foundation's JSON/plist real-number parsing
 * (userland/Foundation/NSJSONSerialization.m, NSPropertyListSerialization.m)
 * and NSString -doubleValue both turned out to depend on it -- caught
 * live in QEMU (FOUNDATIONTEST FAIL: doubleValue), not a hypothetical.
 * Unlike userland/CoreFoundation/CFString.c's documented vsnprintf %f
 * gap (a va_list-mechanics problem with no simple fix), this is a
 * plain, self-contained parser worth actually implementing rather than
 * routing around: sign, integer digits, fractional digits, optional
 * e/E exponent. Not bit-exact/correctly-rounded like a reference libm
 * (each digit is folded in via repeated *10.0 accumulation, not exact
 * decimal-to-binary conversion) -- fine for every real caller in this
 * tree, none of which need IEEE754 last-bit precision. */
double
strtod(const char *nptr, char **endptr)
{
	const char *p = nptr;
	while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r' || *p == '\f' || *p == '\v') {
		p++;
	}
	int neg = 0;
	if (*p == '+' || *p == '-') {
		neg = (*p == '-');
		p++;
	}
	double mantissa = 0.0;
	int anyDigits = 0;
	while (*p >= '0' && *p <= '9') {
		mantissa = mantissa * 10.0 + (double)(*p - '0');
		p++;
		anyDigits = 1;
	}
	if (*p == '.') {
		p++;
		double frac = 0.1;
		while (*p >= '0' && *p <= '9') {
			mantissa += (double)(*p - '0') * frac;
			frac *= 0.1;
			p++;
			anyDigits = 1;
		}
	}
	if (!anyDigits) {
		if (endptr) {
			*endptr = (char *)nptr;
		}
		return 0.0;
	}
	if (*p == 'e' || *p == 'E') {
		const char *q = p + 1;
		int expNeg = 0;
		if (*q == '+' || *q == '-') {
			expNeg = (*q == '-');
			q++;
		}
		if (*q >= '0' && *q <= '9') {
			int exp = 0;
			while (*q >= '0' && *q <= '9') {
				exp = exp * 10 + (*q - '0');
				q++;
			}
			double scale = 1.0;
			for (int i = 0; i < exp; i++) {
				scale *= 10.0;
			}
			mantissa = expNeg ? mantissa / scale : mantissa * scale;
			p = q;
		}
		/* else: "e"/"E" not followed by digits -- not a valid exponent,
		 * leave p where it was (don't consume the 'e'). */
	}
	if (endptr) {
		*endptr = (char *)p;
	}
	return neg ? -mantissa : mantissa;
}

float strtof(const char *nptr, char **endptr) { return (float)strtod(nptr, endptr); }
long double strtold(const char *nptr, char **endptr) { return (long double)strtod(nptr, endptr); }

int atoi(const char *s) { return (int)strtol(s, (void *)0, 10); }
long atol(const char *s) { return strtol(s, (void *)0, 10); }
long long atoll(const char *s) { return strtoll(s, (void *)0, 10); }

int abs(int j) { return j < 0 ? -j : j; }
long labs(long j) { return j < 0 ? -j : j; }
long long llabs(long long j) { return j < 0 ? -j : j; }

div_t div(int numer, int denom) { div_t r; r.quot = numer / denom; r.rem = numer % denom; return r; }
ldiv_t ldiv(long numer, long denom) { ldiv_t r; r.quot = numer / denom; r.rem = numer % denom; return r; }
lldiv_t lldiv(long long numer, long long denom) { lldiv_t r; r.quot = numer / denom; r.rem = numer % denom; return r; }

static unsigned long g_rand_state = 1;

void srand(unsigned int seed) { g_rand_state = seed ? seed : 1; }

int
rand(void)
{
	g_rand_state = g_rand_state * 1103515245UL + 12345UL;
	return (int)((g_rand_state >> 16) & RAND_MAX);
}

void srandom(unsigned int seed) { srand(seed); }
long random(void) { return rand(); }

unsigned int
arc4random(void)
{
	unsigned int v;
	getentropy(&v, sizeof(v)); /* real kernel entropy, not a PRNG stand-in */
	return v;
}

void
qsort(void *base, size_t nmemb, size_t size, int (*compar)(const void *, const void *))
{
	unsigned char *b = base;
	/* insertion sort -- fine for the small argument/name lists busybox's
	 * chosen applet set actually sorts (e.g. `ls`); avoids writing a
	 * recursive quicksort with a manual stack in freestanding code. */
	for (size_t i = 1; i < nmemb; i++) {
		size_t j = i;
		while (j > 0 && compar(b + (j - 1) * size, b + j * size) > 0) {
			for (size_t k = 0; k < size; k++) {
				unsigned char tmp = b[(j - 1) * size + k];
				b[(j - 1) * size + k] = b[j * size + k];
				b[j * size + k] = tmp;
			}
			j--;
		}
	}
}

void
qsort_r(void *base, size_t nmemb, size_t size, void *thunk,
    int (*compar)(void *, const void *, const void *))
{
	unsigned char *b = base;
	/* same insertion sort as qsort() above -- see its comment. */
	for (size_t i = 1; i < nmemb; i++) {
		size_t j = i;
		while (j > 0 && compar(thunk, b + (j - 1) * size, b + j * size) > 0) {
			for (size_t k = 0; k < size; k++) {
				unsigned char tmp = b[(j - 1) * size + k];
				b[(j - 1) * size + k] = b[j * size + k];
				b[j * size + k] = tmp;
			}
			j--;
		}
	}
}

void *
bsearch(const void *key, const void *base, size_t nmemb, size_t size,
    int (*compar)(const void *, const void *))
{
	const unsigned char *b = base;
	size_t lo = 0, hi = nmemb;
	while (lo < hi) {
		size_t mid = lo + (hi - lo) / 2;
		int c = compar(key, b + mid * size);
		if (c == 0) {
			return (void *)(b + mid * size);
		}
		if (c < 0) {
			hi = mid;
		} else {
			lo = mid + 1;
		}
	}
	return (void *)0;
}

/* Real path canonicalization: make absolute (via getcwd() if relative),
 * then resolve "." and ".." components lexically. No symlink
 * resolution -- fat16lite's symlink() exists but nothing here has ever
 * created one on a path clang/LLVM would need resolved, and readlink()
 * per component would be straightforward to add here if that changes.
 * Was a stub (always returned NULL) before this; a real implementation
 * was needed once real callers (LLVM's getMainExecutable) started
 * depending on it actually working. */
char *
realpath(const char *path, char *resolved_path)
{
	if (!path || !*path) {
		return (void *)0;
	}

	char buf[PATH_MAX];
	size_t len;
	if (path[0] == '/') {
		len = 0;
	} else {
		if (!getcwd(buf, sizeof(buf))) {
			return (void *)0;
		}
		len = strlen(buf);
		if (len == 0 || buf[len - 1] != '/') {
			buf[len++] = '/';
		}
	}
	size_t pathlen = strlen(path);
	if (len + pathlen + 1 > sizeof(buf)) {
		return (void *)0;
	}
	memcpy(buf + len, path, pathlen + 1);

	/* Lexically resolve "." and ".." components in place. */
	char out[PATH_MAX];
	out[0] = '/';
	size_t outlen = 1;
	const char *p = buf;
	while (*p) {
		while (*p == '/') {
			p++;
		}
		const char *start = p;
		while (*p && *p != '/') {
			p++;
		}
		size_t complen = (size_t)(p - start);
		if (complen == 0 || (complen == 1 && start[0] == '.')) {
			continue;
		}
		if (complen == 2 && start[0] == '.' && start[1] == '.') {
			if (outlen > 1) {
				outlen--;
				while (outlen > 1 && out[outlen - 1] != '/') {
					outlen--;
				}
			}
			continue;
		}
		if (outlen > 1) {
			out[outlen++] = '/';
		}
		if (outlen + complen >= sizeof(out)) {
			return (void *)0;
		}
		memcpy(out + outlen, start, complen);
		outlen += complen;
	}
	out[outlen] = 0;

	if (resolved_path) {
		memcpy(resolved_path, out, outlen + 1);
		return resolved_path;
	}
	char *dup = malloc(outlen + 1);
	if (dup) {
		memcpy(dup, out, outlen + 1);
	}
	return dup;
}
/* Real mktemp(3)/mkstemp(3): replace a trailing run of 'X' characters
 * with random alphanumerics, retrying on collision (O_EXCL) like every
 * real implementation -- these were permanent stubs (mkstemp always
 * failing) until clang itself needed them for real intermediate files. */
static int
fill_template_x(char *tmpl, size_t *out_xstart, size_t *out_xlen)
{
	size_t len = strlen(tmpl);
	size_t xend = len;
	while (xend > 0 && tmpl[xend - 1] == 'X') {
		xend--;
	}
	size_t xlen = len - xend;
	if (xlen < 6) {
		return -1;
	}
	*out_xstart = xend;
	*out_xlen = xlen;
	return 0;
}

static void
randomize_x(char *tmpl, size_t xstart, size_t xlen)
{
	static const char alphabet[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
	unsigned char rnd[16];
	size_t n = xlen < sizeof(rnd) ? xlen : sizeof(rnd);
	getentropy(rnd, n);
	for (size_t i = 0; i < xlen; i++) {
		tmpl[xstart + i] = alphabet[rnd[i % n] % (sizeof(alphabet) - 1)];
		rnd[i % n] = (unsigned char)(rnd[i % n] * 31 + 7); /* stretch entropy across a longer run */
	}
}

char *
mktemp(char *tmpl)
{
	size_t xstart, xlen;
	if (fill_template_x(tmpl, &xstart, &xlen) != 0) {
		tmpl[0] = 0;
		return tmpl;
	}
	for (int attempt = 0; attempt < 100; attempt++) {
		randomize_x(tmpl, xstart, xlen);
		struct stat sb;
		if (stat(tmpl, &sb) != 0) {
			return tmpl; /* nothing there -- name is free */
		}
	}
	tmpl[0] = 0;
	return tmpl;
}

int
mkstemp(char *tmpl)
{
	size_t xstart, xlen;
	if (fill_template_x(tmpl, &xstart, &xlen) != 0) {
		errno = EINVAL;
		return -1;
	}
	for (int attempt = 0; attempt < 100; attempt++) {
		randomize_x(tmpl, xstart, xlen);
		int fd = open(tmpl, O_RDWR | O_CREAT | O_EXCL, 0600);
		if (fd >= 0) {
			return fd;
		}
		if (errno != EEXIST) {
			return -1;
		}
	}
	errno = EEXIST;
	return -1;
}
