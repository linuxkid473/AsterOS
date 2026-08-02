/* Copyright (c) 2026 Vihaan Nathan */
#include "image.h"
#include "dyld_panic.h"

uint64_t
dyld_stub_binder_resolve(uint32_t lazy_offset, uint64_t return_addr)
{
	struct image *im = image_containing_address(return_addr);
	if (!im) {
		dyld_panic("lazy bind: caller image not found");
	}
	return dyld_bind_lazy_symbol(im, lazy_offset);
}
