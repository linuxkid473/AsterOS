/* No getattrlist()/extended-attribute-list API in this environment
 * (fat16lite has no such concept). This header exists only so code
 * that unconditionally #includes it on Darwin still compiles; add
 * real declarations here only once something actually calls into
 * them (nothing does yet: grep before assuming). */
#ifndef _SYS_ATTR_H_
#define _SYS_ATTR_H_

#ifdef __cplusplus
extern "C" {
#endif
#ifdef __cplusplus
}
#endif

#endif /* _SYS_ATTR_H_ */
