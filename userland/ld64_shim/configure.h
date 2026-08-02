/* Hand-written stand-in for ld64's src/create_configure-generated
 * configure.h -- that script shells out to Xcode build-setting
 * variables (RC_SUPPORTED_ARCHS, RC_ProjectSourceVersion, etc.) we
 * don't have; AsterOS only ever needs the x86_64 slice of it.
 */
#ifndef _LD64_CONFIGURE_H_
#define _LD64_CONFIGURE_H_

#define DEFAULT_MACOSX_MIN_VERSION "10.15"
#define SUPPORT_ARCH_x86_64 1
#define ALL_SUPPORTED_ARCHS "x86_64"
#define BITCODE_XAR_VERSION "1.0"
#define LD64_VERSION_NUM 530
#define LD_PAGE_SIZE 0x1000

#endif /* _LD64_CONFIGURE_H_ */
