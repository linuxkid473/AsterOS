/* Wide-character classification -- this environment only ever deals in
 * ASCII (see wchar.c's own note), so these are the plain ctype.h
 * classifiers operating on a wider argument type. */
#ifndef _WCTYPE_H_
#define _WCTYPE_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <wchar.h>

typedef int wctype_t;

int iswalpha(wint_t wc);
int iswdigit(wint_t wc);
int iswalnum(wint_t wc);
int iswspace(wint_t wc);
int iswupper(wint_t wc);
int iswlower(wint_t wc);
int iswcntrl(wint_t wc);
int iswprint(wint_t wc);
int iswgraph(wint_t wc);
int iswpunct(wint_t wc);
int iswxdigit(wint_t wc);
int iswblank(wint_t wc);
wint_t towupper(wint_t wc);
wint_t towlower(wint_t wc);
wctype_t wctype(const char *name);
int iswctype(wint_t wc, wctype_t desc);

#ifdef __cplusplus
}
#endif

#endif /* _WCTYPE_H_ */
