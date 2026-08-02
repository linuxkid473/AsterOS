/* Darwin thread-local variable (TLV) support.
 *
 * Clang's Darwin codegen for `thread_local` emits a `tlv_descriptor`
 * triplet per variable into __DATA,__thread_vars, initialized with
 * `thunk = &__tlv_bootstrap`. On real Darwin, dyld allocates a
 * per-thread copy of the image's __thread_data/__thread_bss template
 * (contiguously, __thread_data followed by __thread_bss) on first
 * access and rewrites/redirects through that; `__tlv_bootstrap` is the
 * ABI entry point every access thunks through.
 *
 * We have no dyld and (per this project's pthread_stub.c) exactly one
 * thread ever, so there is no "per-thread" to distinguish -- a single,
 * lazily-allocated block that mirrors the __thread_data/__thread_bss
 * template is correct and permanent for the life of the process.
 */
#include <stdlib.h>
#include <string.h>

struct tlv_descriptor {
	void *(*thunk)(struct tlv_descriptor *);
	unsigned long key;
	unsigned long offset;
};

extern char __thread_data_start__[] __asm("section$start$__DATA$__thread_data");
extern char __thread_data_end__[] __asm("section$end$__DATA$__thread_data");
extern char __thread_bss_start__[] __asm("section$start$__DATA$__thread_bss");
extern char __thread_bss_end__[] __asm("section$end$__DATA$__thread_bss");

static char *g_tlv_block;

/* Real Darwin's symbol is `_tlv_bootstrap` at the C source level (one
 * leading underscore); Darwin's automatic C-symbol mangling adds a
 * second, producing the linked symbol `__tlv_bootstrap` that compiler
 * generated tlv_descriptor thunks actually reference. */
void *
_tlv_bootstrap(struct tlv_descriptor *desc)
{
	if (!g_tlv_block) {
		size_t data_size = (size_t)(__thread_data_end__ - __thread_data_start__);
		size_t bss_size = (size_t)(__thread_bss_end__ - __thread_bss_start__);
		g_tlv_block = calloc(1, data_size + bss_size);
		memcpy(g_tlv_block, __thread_data_start__, data_size);
	}
	return g_tlv_block + desc->offset;
}
