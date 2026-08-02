/* Adapted from musl libc's src/internal/libm.h (MIT licensed) for this
 * project: x86_64-only (always little-endian, FLT_EVAL_METHOD==0), and
 * trimmed to what the vendored double-precision functions in this
 * directory actually need -- no long double support (nothing here
 * calls the long-double variants), no endian.h/fp_arch.h/features.h
 * (musl internals this freestanding environment doesn't have). See
 * musl_math/README for which upstream files these are and why they're
 * vendored instead of hand-written: LLVM's constant-folder calls the
 * real libm functions during the compiler's own execution, so they
 * must be numerically correct, not approximated. */
#ifndef _LIBM_H
#define _LIBM_H

#include <stdint.h>
#include <math.h>
#include <float.h>

typedef double double_t;
typedef float float_t;

#ifndef hidden
#define hidden __attribute__((visibility("hidden")))
#endif

#define WANT_ROUNDING 1
#define issignalingf_inline(x) 0
#define issignaling_inline(x) 0

#ifdef __GNUC__
#define predict_true(x) __builtin_expect(!!(x), 1)
#define predict_false(x) __builtin_expect(x, 0)
#else
#define predict_true(x) (x)
#define predict_false(x) (x)
#endif

static inline float eval_as_float(float x) { float y = x; return y; }
static inline double eval_as_double(double x) { double y = x; return y; }

static inline float fp_barrierf(float x) { volatile float y = x; return y; }
static inline double fp_barrier(double x) { volatile double y = x; return y; }

static inline void fp_force_evalf(float x) { volatile float y; y = x; }
static inline void fp_force_eval(double x) { volatile double y; y = x; }

#define FORCE_EVAL(x) do {                        \
	if (sizeof(x) == sizeof(float)) {         \
		fp_force_evalf(x);                \
	} else {                                  \
		fp_force_eval(x);                 \
	}                                         \
} while(0)

#define asuint(f) ((union{float _f; uint32_t _i;}){f})._i
#define asfloat(i) ((union{uint32_t _i; float _f;}){i})._f
#define asuint64(f) ((union{double _f; uint64_t _i;}){f})._i
#define asdouble(i) ((union{uint64_t _i; double _f;}){i})._f

#define EXTRACT_WORDS(hi,lo,d)                    \
do {                                              \
  uint64_t __u = asuint64(d);                     \
  (hi) = __u >> 32;                               \
  (lo) = (uint32_t)__u;                           \
} while (0)

#define GET_HIGH_WORD(hi,d)                       \
do {                                              \
  (hi) = asuint64(d) >> 32;                       \
} while (0)

#define GET_LOW_WORD(lo,d)                        \
do {                                              \
  (lo) = (uint32_t)asuint64(d);                   \
} while (0)

#define INSERT_WORDS(d,hi,lo)                     \
do {                                              \
  (d) = asdouble(((uint64_t)(hi)<<32) | (uint32_t)(lo)); \
} while (0)

#define SET_HIGH_WORD(d,hi)                       \
  INSERT_WORDS(d, hi, (uint32_t)asuint64(d))

#define SET_LOW_WORD(d,lo)                        \
  INSERT_WORDS(d, asuint64(d)>>32, lo)

#define GET_FLOAT_WORD(w,d)                       \
do {                                              \
  (w) = asuint(d);                                \
} while (0)

#define SET_FLOAT_WORD(d,w)                       \
do {                                              \
  (d) = asfloat(w);                               \
} while (0)

hidden int    __rem_pio2_large(double*,double*,int,int,int);
hidden int    __rem_pio2(double,double*);
hidden double __sin(double,double,int);
hidden double __cos(double,double);
hidden double __tan(double,double,int);
hidden double __expo2(double,double);

hidden float __math_xflowf(uint32_t, float);
hidden float __math_uflowf(uint32_t);
hidden float __math_oflowf(uint32_t);
hidden float __math_divzerof(uint32_t);
hidden float __math_invalidf(float);
hidden double __math_xflow(uint32_t, double);
hidden double __math_uflow(uint32_t);
hidden double __math_oflow(uint32_t);
hidden double __math_divzero(uint32_t);
hidden double __math_invalid(double);

#endif
