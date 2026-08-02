#ifndef _WCHAR_H_
#define _WCHAR_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stddef.h>
#include <sys/_types/_mbstate_t.h>

typedef int wint_t;
#define WEOF ((wint_t)-1)

size_t wcslen(const wchar_t *s);
int wcscmp(const wchar_t *a, const wchar_t *b);
int wcsncmp(const wchar_t *a, const wchar_t *b, size_t n);
int wcscoll(const wchar_t *a, const wchar_t *b);
size_t wcsxfrm(wchar_t *dst, const wchar_t *src, size_t n);
wchar_t *wcscpy(wchar_t *dst, const wchar_t *src);
wchar_t *wcsncpy(wchar_t *dst, const wchar_t *src, size_t n);
wchar_t *wcscat(wchar_t *dst, const wchar_t *src);
wchar_t *wcsncat(wchar_t *dst, const wchar_t *src, size_t n);
wchar_t *wcschr(const wchar_t *s, wchar_t c);
wchar_t *wcsrchr(const wchar_t *s, wchar_t c);
wchar_t *wcspbrk(const wchar_t *s1, const wchar_t *s2);
wchar_t *wcsstr(const wchar_t *s1, const wchar_t *s2);
size_t wcsspn(const wchar_t *s, const wchar_t *accept);
size_t wcscspn(const wchar_t *s, const wchar_t *reject);
wchar_t *wcstok(wchar_t *s, const wchar_t *delim, wchar_t **saveptr);

wchar_t *wmemchr(const wchar_t *s, wchar_t c, size_t n);
int wmemcmp(const wchar_t *a, const wchar_t *b, size_t n);
wchar_t *wmemcpy(wchar_t *dst, const wchar_t *src, size_t n);
wchar_t *wmemmove(wchar_t *dst, const wchar_t *src, size_t n);
wchar_t *wmemset(wchar_t *dst, wchar_t c, size_t n);

double wcstod(const wchar_t *s, wchar_t **endptr);
float wcstof(const wchar_t *s, wchar_t **endptr);
long double wcstold(const wchar_t *s, wchar_t **endptr);
long wcstol(const wchar_t *s, wchar_t **endptr, int base);
long long wcstoll(const wchar_t *s, wchar_t **endptr, int base);
unsigned long wcstoul(const wchar_t *s, wchar_t **endptr, int base);
unsigned long long wcstoull(const wchar_t *s, wchar_t **endptr, int base);

struct tm;
size_t wcsftime(wchar_t *s, size_t maxsize, const wchar_t *format, const struct tm *timeptr);

#include <stdarg.h>
int swprintf(wchar_t *s, size_t n, const wchar_t *format, ...);
int vswprintf(wchar_t *s, size_t n, const wchar_t *format, va_list ap);

wint_t fputwc(wchar_t c, FILE *stream);
wint_t fgetwc(FILE *stream);
wint_t getwc(FILE *stream);
wint_t putwc(wchar_t c, FILE *stream);
wint_t ungetwc(wint_t c, FILE *stream);

size_t wcsrtombs(char *dst, const wchar_t **src, size_t n, mbstate_t *ps);
size_t wcsnrtombs(char *dst, const wchar_t **src, size_t nwc, size_t n, mbstate_t *ps);

wint_t btowc(int c);
int wctob(wint_t wc);
int mbtowc(wchar_t *pwc, const char *s, size_t n);
size_t mbrlen(const char *s, size_t n, mbstate_t *ps);
size_t mbrtowc(wchar_t *pwc, const char *s, size_t n, mbstate_t *ps);
size_t wcrtomb(char *s, wchar_t wc, mbstate_t *ps);
size_t mbsrtowcs(wchar_t *dst, const char **src, size_t n, mbstate_t *ps);
size_t mbsnrtowcs(wchar_t *dst, const char **src, size_t nms, size_t n, mbstate_t *ps);

#ifdef __cplusplus
}
#endif

#endif /* _WCHAR_H_ */
