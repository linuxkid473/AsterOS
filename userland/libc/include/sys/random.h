#ifndef _SYS_RANDOM_H_
#define _SYS_RANDOM_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/types.h>

int getentropy(void *buffer, size_t size);

#ifdef __cplusplus
}
#endif

#endif /* _SYS_RANDOM_H_ */
