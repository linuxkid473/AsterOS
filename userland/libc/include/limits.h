#ifndef _LIMITS_H_
#define _LIMITS_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Integer widths come from the compiler's own <limits.h>-equivalent
 * built-ins where possible; we only need the POSIX pathname/arg limits
 * busybox actually reads, ground-truthed against
 * src/xnu/bsd/sys/syslimits.h. */
#define CHAR_BIT   8
#define MB_LEN_MAX 1 /* ASCII-only -- see stdlib.h's MB_CUR_MAX */
#define SCHAR_MIN  (-128)
#define SCHAR_MAX  127
#define UCHAR_MAX  255
#define CHAR_MIN   SCHAR_MIN
#define CHAR_MAX   SCHAR_MAX
#define SHRT_MIN   (-32768)
#define SHRT_MAX   32767
#define USHRT_MAX  65535
#define INT_MIN    (-2147483647 - 1)
#define INT_MAX    2147483647
#define UINT_MAX   4294967295U
#define LONG_MIN   (-9223372036854775807L - 1)
#define LONG_MAX   9223372036854775807L
#define ULONG_MAX  18446744073709551615UL
#define LLONG_MIN  (-9223372036854775807LL - 1)
#define LLONG_MAX  9223372036854775807LL
#define ULLONG_MAX 18446744073709551615ULL

#define PATH_MAX  1024
#define NAME_MAX  255
#define MAXPATHLEN PATH_MAX
#define MAXSYMLINKS 32
#define ARG_MAX   (256 * 1024)
#define _POSIX_ARG_MAX 4096
#define IOV_MAX   1024
#define OPEN_MAX  64
#define LINK_MAX  32767
#define PIPE_BUF  512

#ifdef __cplusplus
}
#endif

#endif /* _LIMITS_H_ */
