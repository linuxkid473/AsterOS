#ifndef _SYS_SYSMACROS_H_
#define _SYS_SYSMACROS_H_

#ifdef __cplusplus
extern "C" {
#endif

#define major(x) (((unsigned)(x) >> 24) & 0xff)
#define minor(x) ((unsigned)(x) & 0xffffff)
#define makedev(maj, min) ((((unsigned)(maj) & 0xff) << 24) | ((unsigned)(min) & 0xffffff))

#ifdef __cplusplus
}
#endif

#endif /* _SYS_SYSMACROS_H_ */
