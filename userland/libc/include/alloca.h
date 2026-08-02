#ifndef _ALLOCA_H_
#define _ALLOCA_H_

#ifdef __cplusplus
extern "C" {
#endif

#define alloca(size) __builtin_alloca(size)

#ifdef __cplusplus
}
#endif

#endif /* _ALLOCA_H_ */
