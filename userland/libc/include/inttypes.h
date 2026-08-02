/* Clang ships its own inttypes.h in its resource dir that just re-exports
 * the platform's real one via #include_next -- we have no platform one
 * (nostdlibinc), so we provide our own minimal version instead of relying
 * on that chain. */
#ifndef _INTTYPES_H_
#define _INTTYPES_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#define PRId8  "d"
#define PRId16 "d"
#define PRId32 "d"
#define PRId64 "lld"
#define PRIu8  "u"
#define PRIu16 "u"
#define PRIu32 "u"
#define PRIu64 "llu"
#define PRIx8  "x"
#define PRIx16 "x"
#define PRIx32 "x"
#define PRIx64 "llx"
#define PRIX8  "X"
#define PRIX16 "X"
#define PRIX32 "X"
#define PRIX64 "llX"
#define PRIdPTR "ld"
#define PRIuPTR "lu"
#define PRIxPTR "lx"
#define PRIXPTR "lX"

typedef struct {
	intmax_t quot;
	intmax_t rem;
} imaxdiv_t;

intmax_t strtoimax(const char *nptr, char **endptr, int base);
uintmax_t strtoumax(const char *nptr, char **endptr, int base);
intmax_t imaxabs(intmax_t j);

#ifdef __cplusplus
}
#endif

#endif /* _INTTYPES_H_ */
