/* Only the "C" locale exists in this environment (see locale.h). Every
 * _l function below is exactly its plain, locale-independent
 * counterpart, ignoring the locale_t argument -- correct, not a
 * shortcut, because there is only ever one locale to be independent of
 * (same approach musl libc takes for the same reason). */
#include <xlocale.h>
#include <ctype.h>
#include <wctype.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdio.h>
#include <nl_types.h>
#include <errno.h>
#include <runetype.h>

extern int errno;

/* Populated by __init_default_rune_locale() (called from __libc_start
 * before main(), see start.c) rather than a static initializer list --
 * this is plain, eagerly-run C code, not a C++ global constructor, so
 * it doesn't depend on whether this environment walks a Mach-O
 * __mod_init_func section (it doesn't, currently). */
_RuneLocale _DefaultRuneLocale;

void
__init_default_rune_locale(void)
{
	for (int c = 0; c < _CACHED_RUNES; c++) {
		unsigned int mask = 0;
		if (isupper(c)) { mask |= _CTYPE_U; }
		if (islower(c)) { mask |= _CTYPE_L; }
		if (isalpha(c)) { mask |= _CTYPE_A; }
		if (isdigit(c)) { mask |= _CTYPE_D; }
		if (isxdigit(c)) { mask |= _CTYPE_X; }
		if (isspace(c)) { mask |= _CTYPE_S; }
		if (isblank(c)) { mask |= _CTYPE_B; }
		if (iscntrl(c)) { mask |= _CTYPE_C; }
		if (isprint(c)) { mask |= _CTYPE_R; }
		if (isgraph(c)) { mask |= _CTYPE_G; }
		if (ispunct(c)) { mask |= _CTYPE_P; }
		_DefaultRuneLocale.__runetype[c] = mask;
		_DefaultRuneLocale.__maplower[c] = tolower(c);
		_DefaultRuneLocale.__mapupper[c] = toupper(c);
	}
}

char *
setlocale(int category, const char *locale)
{
	(void)category;
	if (locale && locale[0] && strcmp(locale, "C") != 0 &&
	    strcmp(locale, "POSIX") != 0 && strcmp(locale, "") != 0) {
		return (char *)0; /* only "C"/"POSIX"/"" (env default) supported */
	}
	return "C";
}

struct lconv *
localeconv(void)
{
	static struct lconv c_lconv = {
		.decimal_point = ".",
		.thousands_sep = "",
		.grouping = "",
		.int_curr_symbol = "",
		.currency_symbol = "",
		.mon_decimal_point = "",
		.mon_thousands_sep = "",
		.mon_grouping = "",
		.positive_sign = "",
		.negative_sign = "",
		.int_frac_digits = 127 /* CHAR_MAX: "not available" per POSIX */,
		.frac_digits = 127,
		.p_cs_precedes = 127,
		.p_sep_by_space = 127,
		.n_cs_precedes = 127,
		.n_sep_by_space = 127,
		.p_sign_posn = 127,
		.n_sign_posn = 127,
		.int_p_cs_precedes = 127,
		.int_n_cs_precedes = 127,
		.int_p_sep_by_space = 127,
		.int_n_sep_by_space = 127,
		.int_p_sign_posn = 127,
		.int_n_sign_posn = 127,
	};
	return &c_lconv;
}

/* locale_t: a sentinel value, never dereferenced -- there is nothing to
 * allocate since there is only one locale. */
locale_t newlocale(int category_mask, const char *locale, locale_t base) { (void)category_mask; (void)locale; (void)base; return (locale_t)1; }
locale_t duplocale(locale_t loc) { (void)loc; return (locale_t)1; }
void freelocale(locale_t loc) { (void)loc; }
locale_t uselocale(locale_t new_loc) { (void)new_loc; return (locale_t)1; }
struct lconv *localeconv_l(locale_t loc) { (void)loc; return localeconv(); }

int isalpha_l(int c, locale_t loc) { (void)loc; return isalpha(c); }
int isdigit_l(int c, locale_t loc) { (void)loc; return isdigit(c); }
int isalnum_l(int c, locale_t loc) { (void)loc; return isalnum(c); }
int isspace_l(int c, locale_t loc) { (void)loc; return isspace(c); }
int isupper_l(int c, locale_t loc) { (void)loc; return isupper(c); }
int islower_l(int c, locale_t loc) { (void)loc; return islower(c); }
int iscntrl_l(int c, locale_t loc) { (void)loc; return iscntrl(c); }
int isprint_l(int c, locale_t loc) { (void)loc; return isprint(c); }
int isgraph_l(int c, locale_t loc) { (void)loc; return isgraph(c); }
int ispunct_l(int c, locale_t loc) { (void)loc; return ispunct(c); }
int isxdigit_l(int c, locale_t loc) { (void)loc; return isxdigit(c); }
int isblank_l(int c, locale_t loc) { (void)loc; return isblank(c); }
int toupper_l(int c, locale_t loc) { (void)loc; return toupper(c); }
int tolower_l(int c, locale_t loc) { (void)loc; return tolower(c); }

int iswalpha_l(wint_t wc, locale_t loc) { (void)loc; return iswalpha(wc); }
int iswdigit_l(wint_t wc, locale_t loc) { (void)loc; return iswdigit(wc); }
int iswalnum_l(wint_t wc, locale_t loc) { (void)loc; return iswalnum(wc); }
int iswspace_l(wint_t wc, locale_t loc) { (void)loc; return iswspace(wc); }
int iswupper_l(wint_t wc, locale_t loc) { (void)loc; return iswupper(wc); }
int iswlower_l(wint_t wc, locale_t loc) { (void)loc; return iswlower(wc); }
int iswcntrl_l(wint_t wc, locale_t loc) { (void)loc; return iswcntrl(wc); }
int iswprint_l(wint_t wc, locale_t loc) { (void)loc; return iswprint(wc); }
int iswpunct_l(wint_t wc, locale_t loc) { (void)loc; return iswpunct(wc); }
int iswxdigit_l(wint_t wc, locale_t loc) { (void)loc; return iswxdigit(wc); }
int iswblank_l(wint_t wc, locale_t loc) { (void)loc; return iswblank(wc); }
wint_t towupper_l(wint_t wc, locale_t loc) { (void)loc; return towupper(wc); }
wint_t towlower_l(wint_t wc, locale_t loc) { (void)loc; return towlower(wc); }
int iswctype_l(wint_t wc, wctype_t desc, locale_t loc) { (void)loc; return iswctype(wc, desc); }

double strtod_l(const char *nptr, char **endptr, locale_t loc) { (void)loc; return strtod(nptr, endptr); }
float strtof_l(const char *nptr, char **endptr, locale_t loc) { (void)loc; return strtof(nptr, endptr); }
long double strtold_l(const char *nptr, char **endptr, locale_t loc) { (void)loc; return strtold(nptr, endptr); }
long long strtoll_l(const char *nptr, char **endptr, int base, locale_t loc) { (void)loc; return strtoll(nptr, endptr, base); }
unsigned long long strtoull_l(const char *nptr, char **endptr, int base, locale_t loc) { (void)loc; return strtoull(nptr, endptr, base); }

int strcoll_l(const char *s1, const char *s2, locale_t loc) { (void)loc; return strcmp(s1, s2); }
size_t strxfrm_l(char *dst, const char *src, size_t n, locale_t loc) { (void)loc; return strxfrm(dst, src, n); }
int wcscoll_l(const wchar_t *s1, const wchar_t *s2, locale_t loc) { (void)loc; return wcscmp(s1, s2); }
size_t wcsxfrm_l(wchar_t *dst, const wchar_t *src, size_t n, locale_t loc) { (void)loc; return wcsxfrm(dst, src, n); }

size_t strftime_l(char *s, size_t max, const char *format, const struct tm *tm, locale_t loc) { (void)loc; return strftime(s, max, format, tm); }

int snprintf_l(char *s, size_t n, locale_t loc, const char *format, ...) { (void)loc; va_list ap; va_start(ap, format); int r = vsnprintf(s, n, format, ap); va_end(ap); return r; }
int asprintf_l(char **s, locale_t loc, const char *format, ...) { (void)loc; va_list ap; va_start(ap, format); int r = vasprintf(s, format, ap); va_end(ap); return r; }
int sscanf_l(const char *s, locale_t loc, const char *format, ...) { (void)loc; va_list ap; va_start(ap, format); int r = vsscanf(s, format, ap); va_end(ap); return r; }

wint_t btowc_l(int c, locale_t loc) { (void)loc; return btowc(c); }
int wctob_l(wint_t wc, locale_t loc) { (void)loc; return wctob(wc); }
int mbtowc_l(wchar_t *pwc, const char *s, size_t n, locale_t loc) { (void)loc; return mbtowc(pwc, s, n); }
size_t mbrlen_l(const char *s, size_t n, mbstate_t *ps, locale_t loc) { (void)loc; return mbrlen(s, n, ps); }
size_t mbrtowc_l(wchar_t *pwc, const char *s, size_t n, mbstate_t *ps, locale_t loc) { (void)loc; return mbrtowc(pwc, s, n, ps); }
size_t wcrtomb_l(char *s, wchar_t wc, mbstate_t *ps, locale_t loc) { (void)loc; return wcrtomb(s, wc, ps); }
size_t mbsrtowcs_l(wchar_t *dst, const char **src, size_t n, mbstate_t *ps, locale_t loc) { (void)loc; return mbsrtowcs(dst, src, n, ps); }
size_t mbsnrtowcs_l(wchar_t *dst, const char **src, size_t nms, size_t n, mbstate_t *ps, locale_t loc) { (void)loc; return mbsnrtowcs(dst, src, nms, n, ps); }
size_t wcsnrtombs_l(char *dst, const wchar_t **src, size_t nwc, size_t n, mbstate_t *ps, locale_t loc) { (void)loc; return wcsnrtombs(dst, src, nwc, n, ps); }

nl_catd catopen(const char *name, int oflag) { (void)name; (void)oflag; errno = ENOENT; return (nl_catd)-1; }
char *catgets(nl_catd catd, int set_id, int msg_id, const char *s) { (void)catd; (void)set_id; (void)msg_id; return (char *)s; }
int catclose(nl_catd catd) { (void)catd; return 0; }
