/* No #ifndef guard -- <assert.h> is meant to be re-includable so NDEBUG
 * toggling works across translation units, per the real header's
 * contract. */
#undef assert

#ifdef NDEBUG
#define assert(e) ((void)0)
#else
#ifdef __cplusplus
extern "C" {
#endif
void __assert_fail(const char *expr, const char *file, int line) __attribute__((noreturn));
void __assert_rtn(const char *func, const char *file, int line, const char *expr) __attribute__((noreturn));
#ifdef __cplusplus
}
#endif
#define assert(e) ((e) ? (void)0 : __assert_fail(#e, __FILE__, __LINE__))
#endif
