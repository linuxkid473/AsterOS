/* Real Darwin's malloc introspection API. Our allocator is a single
 * bump-allocated arena (see malloc.c), not zone-based, so there is
 * exactly one zone; malloc_zone_statistics() reports real numbers from
 * that arena's own bookkeeping (see __libc_malloc_bytes_used in
 * malloc.c), not fabricated ones. */
#ifndef _MALLOC_MALLOC_H_
#define _MALLOC_MALLOC_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/types.h>

typedef struct _malloc_zone_t malloc_zone_t;

typedef struct malloc_statistics_t {
	unsigned blocks_in_use;
	size_t size_in_use;
	size_t max_size_in_use;
	size_t size_allocated;
} malloc_statistics_t;

malloc_zone_t *malloc_default_zone(void);
void malloc_zone_statistics(malloc_zone_t *zone, malloc_statistics_t *stats);
size_t malloc_size(const void *ptr);

#ifdef __cplusplus
}
#endif

#endif /* _MALLOC_MALLOC_H_ */
