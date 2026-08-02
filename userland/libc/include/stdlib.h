#ifndef _STDLIB_H_
#define _STDLIB_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/types.h>
#include <sys/wait.h> /* matches real Darwin's stdlib.h -> sys/wait.h -> sys/resource.h chain */

#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1
#ifndef NULL
#define NULL ((void *)0)
#endif
#define RAND_MAX 0x7fffffff

/* Always 1: this environment is ASCII-only, one byte is one character,
 * in every (i.e. the one) locale -- see locale.h. */
#define MB_CUR_MAX 1
#define MB_CUR_MAX_L(loc) ((void)(loc), 1)

/* alloca() is a compiler builtin (stack allocation, no free() needed) --
 * declared here because real Darwin's stdlib.h transitively pulls in
 * <alloca.h> the same way, and busybox's libbb code calls it without
 * including alloca.h explicitly. */
#define alloca(size) __builtin_alloca(size)

void  *malloc(size_t size);
void  *calloc(size_t nmemb, size_t size);
void  *realloc(void *ptr, size_t size);
void   free(void *ptr);
void  *reallocarray(void *ptr, size_t nmemb, size_t size);
void  *aligned_alloc(size_t alignment, size_t size);

void   exit(int status) __attribute__((noreturn));
void   _Exit(int status) __attribute__((noreturn));
void   abort(void) __attribute__((noreturn));
int    atexit(void (*func)(void));

int    atoi(const char *nptr);
long   atol(const char *nptr);
long long atoll(const char *nptr);
long   strtol(const char *nptr, char **endptr, int base);
unsigned long strtoul(const char *nptr, char **endptr, int base);
long long strtoll(const char *nptr, char **endptr, int base);
unsigned long long strtoull(const char *nptr, char **endptr, int base);
double strtod(const char *nptr, char **endptr);
float strtof(const char *nptr, char **endptr);
long double strtold(const char *nptr, char **endptr);

char  *getenv(const char *name);
int    setenv(const char *name, const char *value, int overwrite);
int    unsetenv(const char *name);
int    putenv(char *string);

void   qsort(void *base, size_t nmemb, size_t size,
              int (*compar)(const void *, const void *));
void   qsort_r(void *base, size_t nmemb, size_t size, void *thunk,
              int (*compar)(void *, const void *, const void *));
void  *bsearch(const void *key, const void *base, size_t nmemb, size_t size,
              int (*compar)(const void *, const void *));
int    abs(int j);
long   labs(long j);
long long llabs(long long j);

typedef struct { int quot, rem; } div_t;
typedef struct { long quot, rem; } ldiv_t;
typedef struct { long long quot, rem; } lldiv_t;
div_t   div(int numer, int denom);
ldiv_t  ldiv(long numer, long denom);
lldiv_t lldiv(long long numer, long long denom);
int    rand(void);
void   srand(unsigned int seed);
long   random(void);
void   srandom(unsigned int seed);
unsigned int arc4random(void);

char  *realpath(const char *path, char *resolved_path);
char  *mktemp(char *tmpl);
int    mkstemp(char *tmpl);

#ifdef __cplusplus
}
#endif

#endif /* _STDLIB_H_ */
