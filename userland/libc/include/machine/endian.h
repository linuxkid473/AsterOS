/* x86_64 is little-endian. */
#ifndef _MACHINE_ENDIAN_H_
#define _MACHINE_ENDIAN_H_

#ifdef __cplusplus
extern "C" {
#endif

#define __LITTLE_ENDIAN 1234
#define __BIG_ENDIAN    4321
#define __BYTE_ORDER    __LITTLE_ENDIAN

#define _LITTLE_ENDIAN __LITTLE_ENDIAN
#define _BIG_ENDIAN    __BIG_ENDIAN
#define _BYTE_ORDER    __BYTE_ORDER

#define LITTLE_ENDIAN __LITTLE_ENDIAN
#define BIG_ENDIAN    __BIG_ENDIAN
#define BYTE_ORDER    __BYTE_ORDER

static inline unsigned short __bswap16(unsigned short x) { return __builtin_bswap16(x); }
static inline unsigned int   __bswap32(unsigned int x)   { return __builtin_bswap32(x); }
static inline unsigned long long __bswap64(unsigned long long x) { return __builtin_bswap64(x); }

#ifdef __cplusplus
}
#endif

/* Real Darwin's machine/endian.h pulls in ntohl/htonl/ntohs/htons this
 * way too -- code that only includes <machine/endian.h> (e.g. ld64's
 * code-sign-blobs/endian.h) expects them to already be visible. */
#include <sys/_endian.h>

#endif /* _MACHINE_ENDIAN_H_ */
