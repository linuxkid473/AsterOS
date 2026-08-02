/* Minimal malloc: a chain of mmap'd arena blocks, each with a first-fit
 * free list. No munmap-back-to-OS. Originally sized as a single fixed
 * 8MB block ("good enough for a shell + a handful of coreutils
 * applets"), which is far too small for a real linker (ld64) processing
 * multiple sizeable static archives -- that exhausted the arena,
 * returned NULL, and surfaced as an uncaught std::bad_alloc. Growing by
 * mmap'ing another block on exhaustion, rather than just raising the
 * fixed size, means this no longer silently breaks again the next time
 * something bigger comes along.
 *
 * Thread safety: real pthreads now exist (pthread.c), so every public
 * entry point below takes g_malloc_lock -- a plain atomic-CAS spinlock,
 * not a pthread_mutex_t (this file must not depend on pthread.c, which
 * itself calls malloc()). Internal helpers with a _nolock suffix assume
 * the caller already holds it; callers that build on top of another
 * public entry point's logic (calloc, aligned_alloc) take the lock once
 * themselves and call the _nolock forms directly instead of recursing
 * into the public (locking) ones, since the spinlock isn't recursive. */
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <stdint.h>

static int g_malloc_lock;

static void
malloc_lock(void)
{
	while (__atomic_exchange_n(&g_malloc_lock, 1, __ATOMIC_ACQUIRE)) {
		__asm__ __volatile__("pause" ::: "memory");
	}
}

static void
malloc_unlock(void)
{
	__atomic_store_n(&g_malloc_lock, 0, __ATOMIC_RELEASE);
}

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

static void *
malloc_nolock(size_t size)
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

static void
free_nolock(void *ptr)
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
malloc(size_t size)
{
	malloc_lock();
	void *p = malloc_nolock(size);
	malloc_unlock();
	return p;
}

void
free(void *ptr)
{
	malloc_lock();
	free_nolock(ptr);
	malloc_unlock();
}

void *
calloc(size_t nmemb, size_t size)
{
	size_t total = nmemb * size;
	if (nmemb != 0 && total / nmemb != size) {
		return (void *)0; /* overflow */
	}
	malloc_lock();
	void *p = malloc_nolock(total);
	if (p) {
		memset(p, 0, total);
	}
	malloc_unlock();
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
	malloc_lock();
	struct chunk *c = (struct chunk *)ptr - 1;
	if (c->size >= size) {
		malloc_unlock();
		return ptr;
	}
	void *n = malloc_nolock(size);
	if (!n) {
		malloc_unlock();
		return (void *)0;
	}
	memcpy(n, ptr, c->size);
	free_nolock(ptr);
	malloc_unlock();
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
	malloc_lock();
	void *raw = malloc_nolock(size + alignment + sizeof(struct chunk));
	if (!raw) {
		malloc_unlock();
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
	malloc_unlock();
	return (void *)aligned;
}
