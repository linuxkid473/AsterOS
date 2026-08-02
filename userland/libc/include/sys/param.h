#ifndef _SYS_PARAM_H_
#define _SYS_PARAM_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <limits.h>

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define howmany(x, y) (((x) + ((y) - 1)) / (y))

#ifdef __cplusplus
}
#endif

#endif /* _SYS_PARAM_H_ */
