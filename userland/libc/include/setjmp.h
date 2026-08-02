/* Our own setjmp/longjmp -- register-save layout inspired by (not
 * bit-for-bit compatible with) Apple's real implementation
 * (src/libplatform/src/setjmp/x86_64/{setjmp,_setjmp}.s): we skip the
 * pointer-authentication "munge" step (needs a thread-local secret we
 * have no TSD/pthread infrastructure to provide) and the sigprocmask
 * save/restore __setjmp/__longjmp do (we don't need signal-mask
 * save/restore for ash's control-flow use of setjmp/longjmp). Since our
 * own setjmp.S is the only definer AND the only caller context, ABI
 * compatibility with real Darwin isn't needed -- only internal
 * consistency, which a plain callee-saved-register save/restore gives. */
#ifndef _SETJMP_H_
#define _SETJMP_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Tagged (not anonymous) so jmp_buf has linkage -- required for setjmp()/
 * longjmp() to be usable from C++ translation units (an anonymous
 * struct type has no linkage, which C++ forbids in an extern function's
 * signature unless it's defined in the same TU); matches real Darwin's
 * own setjmp.h using __jmp_buf_tag for the same reason. */
struct __jmp_buf_tag {
	long regs[8]; /* rbx, rbp, rsp, r12, r13, r14, r15, rip */
};
typedef struct __jmp_buf_tag jmp_buf[1];

typedef jmp_buf sigjmp_buf;

int setjmp(jmp_buf env);
void longjmp(jmp_buf env, int val) __attribute__((noreturn));
int sigsetjmp(sigjmp_buf env, int savemask);
void siglongjmp(sigjmp_buf env, int val) __attribute__((noreturn));

#ifdef __cplusplus
}
#endif

#endif /* _SETJMP_H_ */
