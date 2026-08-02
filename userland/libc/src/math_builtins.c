/* fabs/sqrt/floor: real functions (not macros -- see math.h's note),
 * each a direct, exact wrapper around the clang/LLVM builtin that
 * lowers to a single hardware instruction on x86_64. Not approximated:
 * __builtin_fabs/__builtin_sqrt/__builtin_floor are exact per IEEE 754. */
#include <math.h>

double fabs(double x) { return __builtin_fabs(x); }
double sqrt(double x) { return __builtin_sqrt(x); }
double floor(double x) { return __builtin_floor(x); }
