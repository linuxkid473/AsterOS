/* Darwin's internal per-locale character classification/case-mapping
 * table -- libc++'s <locale> (locale.cpp) reaches into
 * _DefaultRuneLocale directly for std::ctype<char>'s is()/toupper()/
 * tolower() rather than calling through <ctype.h>. Struct shape
 * ground-truthed against the real runetype.h; only the first 256
 * (ASCII) entries of each table are ever populated or consulted here
 * (see __init_default_rune_locale in locale.c) since this environment
 * is ASCII-only -- see wchar.c's own note. */
#ifndef _RUNETYPE_H_
#define _RUNETYPE_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/types.h>

#define _CACHED_RUNES 256

typedef struct {
	int __min;
	int __max;
	int __map;
	unsigned int *__types;
} _RuneEntry;

typedef struct {
	int __nranges;
	_RuneEntry *__ranges;
} _RuneRange;

typedef struct {
	char __magic[8];
	char __encoding[32];

	int (*__sgetrune)(const char *, size_t, const char **);
	int (*__sputrune)(int, char *, size_t, char **);
	int __invalid_rune;

	unsigned int __runetype[_CACHED_RUNES];
	int __maplower[_CACHED_RUNES];
	int __mapupper[_CACHED_RUNES];

	_RuneRange __runetype_ext;
	_RuneRange __maplower_ext;
	_RuneRange __mapupper_ext;

	void *__variable;
	int __variable_len;
} _RuneLocale;

extern _RuneLocale _DefaultRuneLocale;

#ifdef __cplusplus
}
#endif

#endif /* _RUNETYPE_H_ */
