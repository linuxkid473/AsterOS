/* mbstate_t is an opaque object used to keep conversion state during
 * multibyte stream conversions. __darwin_mbstate_t itself is defined by
 * i386/_types.h (pulled in transitively via sys/_types.h) -- this header
 * just aliases mbstate_t from it, matching real Darwin's layering. This
 * used to (wrongly) redefine __darwin_mbstate_t itself, which collided
 * with i386/_types.h's own definition once that real header was added
 * to this tree (ld64 pulls it in via mach-o/x86_64/reloc.h and others). */
#ifndef _MBSTATE_T
#define _MBSTATE_T

#include <sys/_types.h>

#ifdef __cplusplus
extern "C" {
#endif
typedef __darwin_mbstate_t mbstate_t;
#ifdef __cplusplus
}
#endif

#endif /* _MBSTATE_T */
