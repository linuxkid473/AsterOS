/* Copyright (c) 2026 Vihaan Nathan
 *
 * Interprets the REBASE_OPCODE_* stream (LC_DYLD_INFO_ONLY rebase_off):
 * a compressed table of <segment, offset, type> tuples telling us which
 * pointer-sized slots were written on disk assuming the image loads at
 * its preferred address, and therefore need `slide` added at runtime.
 */
#include "image.h"
#include "dyld_panic.h"
#include "uleb.h"

static void
do_rebase(struct image *im, uint32_t seg_index, uint64_t seg_offset, uint32_t type)
{
	if (seg_index >= (uint32_t)im->nsegs) {
		dyld_panic("rebase: bad segment index");
	}
	if (type != REBASE_TYPE_POINTER) {
		dyld_panic("rebase: unsupported type");
	}
	uintptr_t *slot = (uintptr_t *)image_addr(im, im->segs[seg_index].vmaddr + seg_offset);
	*slot += im->slide;
}

void
image_rebase(struct image *im)
{
	if (!im->rebase_start) {
		return;
	}
	const uint8_t *p = im->rebase_start;
	const uint8_t *end = im->rebase_end;
	uint32_t type = 0;
	uint32_t seg_index = 0;
	uint64_t seg_offset = 0;

	while (p < end) {
		uint8_t byte = *p++;
		uint8_t opcode = byte & REBASE_OPCODE_MASK;
		uint8_t imm = byte & 0x0f;

		switch (opcode) {
		case REBASE_OPCODE_DONE:
			return;
		case REBASE_OPCODE_SET_TYPE_IMM:
			type = imm;
			break;
		case REBASE_OPCODE_SET_SEGMENT_AND_OFFSET_ULEB:
			seg_index = imm;
			seg_offset = read_uleb128(&p, end);
			break;
		case REBASE_OPCODE_ADD_ADDR_ULEB:
			seg_offset += read_uleb128(&p, end);
			break;
		case REBASE_OPCODE_ADD_ADDR_IMM_SCALED:
			seg_offset += (uint64_t)imm * sizeof(uintptr_t);
			break;
		case REBASE_OPCODE_DO_REBASE_IMM_TIMES:
			for (int i = 0; i < imm; i++) {
				do_rebase(im, seg_index, seg_offset, type);
				seg_offset += sizeof(uintptr_t);
			}
			break;
		case REBASE_OPCODE_DO_REBASE_ULEB_TIMES: {
			uint64_t count = read_uleb128(&p, end);
			for (uint64_t i = 0; i < count; i++) {
				do_rebase(im, seg_index, seg_offset, type);
				seg_offset += sizeof(uintptr_t);
			}
			break;
		}
		case REBASE_OPCODE_DO_REBASE_ADD_ADDR_ULEB:
			do_rebase(im, seg_index, seg_offset, type);
			seg_offset += read_uleb128(&p, end) + sizeof(uintptr_t);
			break;
		case REBASE_OPCODE_DO_REBASE_ULEB_TIMES_SKIPPING_ULEB: {
			uint64_t count = read_uleb128(&p, end);
			uint64_t skip = read_uleb128(&p, end);
			for (uint64_t i = 0; i < count; i++) {
				do_rebase(im, seg_index, seg_offset, type);
				seg_offset += skip + sizeof(uintptr_t);
			}
			break;
		}
		default:
			dyld_panic("rebase: unknown opcode");
		}
	}
}
