/* Minimal malloc: a chain of mmap'd arena blocks, each with a first-fit
 * free list. No munmap-back-to-OS, no thread safety (we are
 * single-threaded -- no pthreads in this environment). Originally sized
 * as a single fixed 8MB block ("good enough for a shell + a handful of
 * coreutils applets"), which is far too small for a real linker (ld64)
 * processing multiple sizeable static archives -- that exhausted the
 * arena, returned NULL, and surfaced as an uncaught std::bad_alloc.
 * Growing by mmap'ing another block on exhaustion, rather than just
 * raising the fixed size, means this no longer silently breaks again
 * the next time something bigger comes along. */
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <stdint.h>

/* Header is padded to 32 bytes (a multiple of 16) so that a payload
 * immediately following it stays 16-byte aligned whenever the payload
 * before it was -- see align16() below for why 16, not 8. */
struct chunk {
	size_t         size;   /* payload size, not including header */
	int            free;
	struct chunk  *next;
	size_t         _pad;
};

#define ARENA_SIZE (8 * 1024 * 1024)
static struct chunk *g_head;

static struct chunk *
arena_grow(size_t min_size)
{
	size_t block_size = ARENA_SIZE;
	if (min_size + sizeof(struct chunk) > block_size) {
		block_size = min_size + sizeof(struct chunk);
	}
	unsigned char *base = mmap(0, block_size, PROT_READ | PROT_WRITE,
	    MAP_PRIVATE | MAP_ANON, -1, 0);
	if (base == (void *)-1) {
		return (void *)0;
	}
	struct chunk *c = (struct chunk *)base;
	c->size = block_size - sizeof(struct chunk);
	c->free = 1;
	c->next = (void *)0;
	return c;
}

static void
arena_init(void)
{
	if (g_head) {
		return;
	}
	g_head = arena_grow(0);
}

/* x86_64 SysV/Itanium C++ ABI requires malloc to return memory aligned
 * to alignof(max_align_t) == 16 (SSE loads/stores like `movaps` fault
 * with #GP on anything less) -- this was 8 (a real, load-bearing bug:
 * every chunk is `sizeof(struct chunk)` (now a 16-byte multiple) past
 * the previous one, so keeping allocation sizes themselves 16-aligned
 * is what keeps every payload in the arena 16-aligned, not just the
 * first one). Discovered via a SIGSEGV in a large real program
 * (clang) that happened to allocate an object whose 16-byte-aligned
 * member landed on an 8-but-not-16-aligned address; smaller programs
 * had gotten lucky by chance until then. */
static size_t
align16(size_t n)
{
	return (n + 15) & ~(size_t)15;
}

void *
malloc(size_t size)
{
	arena_init();
	if (size == 0) {
		size = 1;
	}
	size = align16(size);

	for (struct chunk *c = g_head; c; c = c->next) {
		if (!c->free || c->size < size) {
			continue;
		}
		/* split if there's enough room left for another header +
		 * a useful minimum payload */
		if (c->size >= size + sizeof(struct chunk) + 8) {
			struct chunk *rem = (struct chunk *)((unsigned char *)(c + 1) + size);
			rem->size = c->size - size - sizeof(struct chunk);
			rem->free = 1;
			rem->next = c->next;
			c->next = rem;
			c->size = size;
		}
		c->free = 0;
		return (void *)(c + 1);
	}

	/* No existing block has room -- mmap another one and link it onto
	 * the end of the chain, then retry the allocation from it. */
	struct chunk *tail = g_head;
	while (tail->next) {
		tail = tail->next;
	}
	struct chunk *fresh = arena_grow(size);
	if (!fresh) {
		return (void *)0; /* real OOM: mmap itself failed */
	}
	tail->next = fresh;

	if (fresh->size >= size + sizeof(struct chunk) + 8) {
		struct chunk *rem = (struct chunk *)((unsigned char *)(fresh + 1) + size);
		rem->size = fresh->size - size - sizeof(struct chunk);
		rem->free = 1;
		rem->next = fresh->next;
		fresh->next = rem;
		fresh->size = size;
	}
	fresh->free = 0;
	return (void *)(fresh + 1);
}

void
free(void *ptr)
{
	if (!ptr) {
		return;
	}
	struct chunk *c = (struct chunk *)ptr - 1;
	c->free = 1;
	/* coalesce adjacent free chunks (single pass, list is address-ordered
	 * since we only ever split forward) */
	for (struct chunk *p = g_head; p && p->next; p = p->next) {
		if (p->free && p->next->free &&
		    (unsigned char *)(p + 1) + p->size == (unsigned char *)p->next) {
			p->size += sizeof(struct chunk) + p->next->size;
			p->next = p->next->next;
		}
	}
}

void *
calloc(size_t nmemb, size_t size)
{
	size_t total = nmemb * size;
	if (nmemb != 0 && total / nmemb != size) {
		return (void *)0; /* overflow */
	}
	void *p = malloc(total);
	if (p) {
		memset(p, 0, total);
	}
	return p;
}

void *
realloc(void *ptr, size_t size)
{
	if (!ptr) {
		return malloc(size);
	}
	if (size == 0) {
		free(ptr);
		return (void *)0;
	}
	struct chunk *c = (struct chunk *)ptr - 1;
	if (c->size >= size) {
		return ptr;
	}
	void *n = malloc(size);
	if (!n) {
		return (void *)0;
	}
	memcpy(n, ptr, c->size);
	free(ptr);
	return n;
}

void *
reallocarray(void *ptr, size_t nmemb, size_t size)
{
	size_t total = nmemb * size;
	if (nmemb != 0 && total / nmemb != size) {
		return (void *)0;
	}
	return realloc(ptr, total);
}

/* free() finds a chunk's header at ptr - 1, so an over-aligned payload
 * needs a real chunk header placed immediately before it -- over-allocate
 * and carve the leading slack off as its own (immediately freeable) chunk,
 * the same split-on-alloc technique malloc() itself already uses. */
void *
aligned_alloc(size_t alignment, size_t size)
{
	if (alignment <= 8) {
		return malloc(size);
	}
	void *raw = malloc(size + alignment + sizeof(struct chunk));
	if (!raw) {
		return (void *)0;
	}
	uintptr_t aligned = ((uintptr_t)raw + sizeof(struct chunk) + alignment - 1) &
	    ~(uintptr_t)(alignment - 1);
	struct chunk *orig = (struct chunk *)raw - 1;
	struct chunk *newc = (struct chunk *)aligned - 1;
	size_t front_waste = (unsigned char *)newc - (unsigned char *)orig;

	newc->size = orig->size - front_waste;
	newc->free = 0;
	newc->next = orig->next;
	orig->size = front_waste - sizeof(struct chunk);
	orig->free = 1;
	orig->next = newc;
	return (void *)aligned;
}
