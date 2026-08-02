/* The BSD "extended locale" API (the _l-suffixed functions taking an
 * explicit locale_t, used internally by libc++ regardless of what the
 * process's global locale is set to). This environment only ever has
 * the "C" locale (see locale.h) so locale_t is a trivial sentinel and
 * every _l function is exactly its plain, locale-independent
 * counterpart -- correct, not a shortcut, precisely because there is
 * only ever one locale to be "independent" of. Same approach musl libc
 * takes for the same reason. */
#ifndef _XLOCALE_H_
#define _XLOCALE_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <locale.h>
#include <wchar.h>
#include <wctype.h>
#include <stdarg.h>

typedef struct __locale_struct *locale_t;

#define LC_COLLATE_MASK  (1 << LC_COLLATE)
#define LC_CTYPE_MASK    (1 << LC_CTYPE)
#define LC_MESSAGES_MASK (1 << LC_MESSAGES)
#define LC_MONETARY_MASK (1 << LC_MONETARY)
#define LC_NUMERIC_MASK  (1 << LC_NUMERIC)
#define LC_TIME_MASK     (1 << LC_TIME)
#define LC_ALL_MASK      (LC_COLLATE_MASK | LC_CTYPE_MASK | LC_MESSAGES_MASK | \
                           LC_MONETARY_MASK | LC_NUMERIC_MASK | LC_TIME_MASK)

#define LC_GLOBAL_LOCALE ((locale_t)-1)

locale_t newlocale(int category_mask, const char *locale, locale_t base);
locale_t duplocale(locale_t loc);
void freelocale(locale_t loc);
locale_t uselocale(locale_t new_loc);
struct lconv *localeconv_l(locale_t loc);

int isalpha_l(int c, locale_t loc);
int isdigit_l(int c, locale_t loc);
int isalnum_l(int c, locale_t loc);
int isspace_l(int c, locale_t loc);
int isupper_l(int c, locale_t loc);
int islower_l(int c, locale_t loc);
int iscntrl_l(int c, locale_t loc);
int isprint_l(int c, locale_t loc);
int isgraph_l(int c, locale_t loc);
int ispunct_l(int c, locale_t loc);
int isxdigit_l(int c, locale_t loc);
int isblank_l(int c, locale_t loc);
int toupper_l(int c, locale_t loc);
int tolower_l(int c, locale_t loc);

int iswalpha_l(wint_t wc, locale_t loc);
int iswdigit_l(wint_t wc, locale_t loc);
int iswalnum_l(wint_t wc, locale_t loc);
int iswspace_l(wint_t wc, locale_t loc);
int iswupper_l(wint_t wc, locale_t loc);
int iswlower_l(wint_t wc, locale_t loc);
int iswcntrl_l(wint_t wc, locale_t loc);
int iswprint_l(wint_t wc, locale_t loc);
int iswpunct_l(wint_t wc, locale_t loc);
int iswxdigit_l(wint_t wc, locale_t loc);
int iswblank_l(wint_t wc, locale_t loc);
wint_t towupper_l(wint_t wc, locale_t loc);
wint_t towlower_l(wint_t wc, locale_t loc);
int iswctype_l(wint_t wc, wctype_t desc, locale_t loc);

double strtod_l(const char *nptr, char **endptr, locale_t loc);
float strtof_l(const char *nptr, char **endptr, locale_t loc);
long double strtold_l(const char *nptr, char **endptr, locale_t loc);
long long strtoll_l(const char *nptr, char **endptr, int base, locale_t loc);
unsigned long long strtoull_l(const char *nptr, char **endptr, int base, locale_t loc);

int strcoll_l(const char *s1, const char *s2, locale_t loc);
size_t strxfrm_l(char *dst, const char *src, size_t n, locale_t loc);
int wcscoll_l(const wchar_t *s1, const wchar_t *s2, locale_t loc);
size_t wcsxfrm_l(wchar_t *dst, const wchar_t *src, size_t n, locale_t loc);

struct tm;
size_t strftime_l(char *s, size_t max, const char *format, const struct tm *tm, locale_t loc);

int snprintf_l(char *s, size_t n, locale_t loc, const char *format, ...);
int asprintf_l(char **s, locale_t loc, const char *format, ...);
int sscanf_l(const char *s, locale_t loc, const char *format, ...);

wint_t btowc_l(int c, locale_t loc);
int wctob_l(wint_t wc, locale_t loc);
int mbtowc_l(wchar_t *pwc, const char *s, size_t n, locale_t loc);
size_t mbrlen_l(const char *s, size_t n, mbstate_t *ps, locale_t loc);
size_t mbrtowc_l(wchar_t *pwc, const char *s, size_t n, mbstate_t *ps, locale_t loc);
size_t wcrtomb_l(char *s, wchar_t wc, mbstate_t *ps, locale_t loc);
size_t mbsrtowcs_l(wchar_t *dst, const char **src, size_t n, mbstate_t *ps, locale_t loc);
size_t mbsnrtowcs_l(wchar_t *dst, const char **src, size_t nms, size_t n, mbstate_t *ps, locale_t loc);
size_t wcsnrtombs_l(char *dst, const wchar_t **src, size_t nwc, size_t n, mbstate_t *ps, locale_t loc);

#ifdef __cplusplus
}
#endif

#endif /* _XLOCALE_H_ */
