/* Copyright (c) 2026 Vihaan Nathan
 *
 * Interprets the BIND_OPCODE_* stream: resolves imported symbols against
 * other loaded images and writes the resolved address into a pointer slot.
 * Shared between eager binding (bind_off/weak_bind_off, run once at load
 * time) and lazy binding (lazy_bind_off, run on-demand from
 * dyld_stub_binder -- see stub_binder.c) since both use the same opcode
 * encoding, just addressed differently.
 */
#include "image.h"
#include "dyld_panic.h"
#include "uleb.h"
#include <string.h>

struct bind_state {
	uint32_t seg_index;
	uint64_t seg_offset;
	uint32_t type;
	int64_t addend;
	int dylib_ordinal;
	const char *symbol;
};

static struct image *
ordinal_to_image(struct image *im, int ordinal)
{
	if (ordinal == BIND_SPECIAL_DYLIB_SELF) {
		return im;
	}
	if (ordinal == BIND_SPECIAL_DYLIB_MAIN_EXECUTABLE) {
		return g_main_image;
	}
	if (ordinal >= 1 && ordinal <= im->ndeps) {
		return im->deps[ordinal - 1];
	}
	return 0;	/* flat/weak lookup ordinals fall through to the flat search below */
}

/* Real dyld special-cases this exact name: ld64 always emits it as an
 * undefined symbol "normally found in libSystem.dylib" (see ld64's
 * Resolver.cpp/stubs.cpp) for the shared `_fast_lazy_bind` pointer every
 * lazily-bound binary needs -- we have no libSystem, so we resolve it
 * directly to our own trampoline instead of searching any loaded image. */
extern void _dyld_stub_binder_entry(void) DYLD_HIDDEN;

static uint64_t
resolve_symbol(struct image *im, int ordinal, const char *symbol)
{
	if (symbol && strcmp(symbol, "dyld_stub_binder") == 0) {
		return (uint64_t)(uintptr_t)&_dyld_stub_binder_entry;
	}
	struct image *target = ordinal_to_image(im, ordinal);
	if (target) {
		uint64_t addr = image_resolve_export(target, symbol);
		if (addr) {
			return addr;
		}
	}
	/* BIND_SPECIAL_DYLIB_FLAT_LOOKUP/WEAK_LOOKUP, or a normal ordinal
	 * whose export wasn't found (weak symbols can legitimately live in
	 * a different image than the one that declared them): fall back to
	 * searching every loaded image, same flat-namespace behavior real
	 * dyld uses for these ordinals. */
	for (int i = 0; i < g_nimages; i++) {
		uint64_t addr = image_resolve_export(&g_images[i], symbol);
		if (addr) {
			return addr;
		}
	}
	return 0;
}

/* Runs one bind opcode stream to completion, applying every fixup found.
 * `single_record` stops after the first DO_BIND* (used by lazy binding,
 * which addresses one self-contained record per stub call and returns
 * the resolved value instead of just writing it). */
static uint64_t
run_bind_stream(struct image *im, const uint8_t *p, const uint8_t *end, int single_record)
{
	struct bind_state st = {0};
	uint64_t result = 0;

	while (p < end) {
		uint8_t byte = *p++;
		uint8_t opcode = byte & BIND_OPCODE_MASK;
		uint8_t imm = byte & 0x0f;

		switch (opcode) {
		case BIND_OPCODE_DONE:
			st = (struct bind_state){0};
			break;
		case BIND_OPCODE_SET_DYLIB_ORDINAL_IMM:
			st.dylib_ordinal = imm;
			break;
		case BIND_OPCODE_SET_DYLIB_ORDINAL_ULEB:
			st.dylib_ordinal = (int)read_uleb128(&p, end);
			break;
		case BIND_OPCODE_SET_DYLIB_SPECIAL_IMM:
			/* imm is a 4-bit field; the special ordinals are small
			 * negative numbers, sign-extend from that width. */
			st.dylib_ordinal = imm == 0 ? 0 : (int)(int8_t)(0xf0 | imm);
			break;
		case BIND_OPCODE_SET_SYMBOL_TRAILING_FLAGS_IMM:
			st.symbol = (const char *)p;
			while (p < end && *p) {
				p++;
			}
			p++;
			break;
		case BIND_OPCODE_SET_TYPE_IMM:
			st.type = imm;
			break;
		case BIND_OPCODE_SET_ADDEND_SLEB:
			st.addend = read_sleb128(&p, end);
			break;
		case BIND_OPCODE_SET_SEGMENT_AND_OFFSET_ULEB:
			st.seg_index = imm;
			st.seg_offset = read_uleb128(&p, end);
			break;
		case BIND_OPCODE_ADD_ADDR_ULEB:
			st.seg_offset += read_uleb128(&p, end);
			break;
		case BIND_OPCODE_DO_BIND:
		case BIND_OPCODE_DO_BIND_ADD_ADDR_ULEB:
		case BIND_OPCODE_DO_BIND_ADD_ADDR_IMM_SCALED:
		case BIND_OPCODE_DO_BIND_ULEB_TIMES_SKIPPING_ULEB: {
			uint64_t count = 1, skip = 0;
			if (opcode == BIND_OPCODE_DO_BIND_ULEB_TIMES_SKIPPING_ULEB) {
				count = read_uleb128(&p, end);
				skip = read_uleb128(&p, end);
			}
			for (uint64_t i = 0; i < count; i++) {
				if (st.type != BIND_TYPE_POINTER) {
					dyld_panic("bind: unsupported type");
				}
				if (st.seg_index >= (uint32_t)im->nsegs) {
					dyld_panic("bind: bad segment index");
				}
				uint64_t resolved = resolve_symbol(im, st.dylib_ordinal, st.symbol);
				if (!resolved) {
					dyld_panic(st.symbol ? st.symbol : "(unknown symbol)");
				}
				uint64_t value = resolved + st.addend;
				*(uintptr_t *)image_addr(im, im->segs[st.seg_index].vmaddr + st.seg_offset) = value;
				result = value;
				if (opcode == BIND_OPCODE_DO_BIND_ADD_ADDR_ULEB) {
					st.seg_offset += read_uleb128(&p, end);
				} else if (opcode == BIND_OPCODE_DO_BIND_ADD_ADDR_IMM_SCALED) {
					st.seg_offset += (uint64_t)imm * sizeof(uintptr_t);
				} else {
					st.seg_offset += skip;
				}
				st.seg_offset += sizeof(uintptr_t);
			}
			if (single_record) {
				return result;
			}
			break;
		}
		default:
			dyld_panic("bind: unknown opcode");
		}
	}
	return result;
}

void
image_bind(struct image *im)
{
	if (im->bind_start) {
		run_bind_stream(im, im->bind_start, im->bind_end, 0);
	}
	if (im->weak_bind_start) {
		run_bind_stream(im, im->weak_bind_start, im->weak_bind_end, 0);
	}
}

uint64_t
dyld_bind_lazy_symbol(struct image *im, uint32_t lazy_offset)
{
	if (!im->lazy_bind_start || lazy_offset >= (uint32_t)(im->lazy_bind_end - im->lazy_bind_start)) {
		dyld_panic("lazy bind: offset out of range");
	}
	return run_bind_stream(im, im->lazy_bind_start + lazy_offset, im->lazy_bind_end, 1);
}
