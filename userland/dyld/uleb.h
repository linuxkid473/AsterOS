/* Copyright (c) 2026 Vihaan Nathan
 *
 * ULEB128/SLEB128 decoding for the rebase/bind/export opcode streams --
 * same variable-length integer encoding DWARF uses, standard algorithm.
 */
#ifndef DYLD_ULEB_H
#define DYLD_ULEB_H

#include <stdint.h>
#include "dyld_panic.h"

static inline uint64_t
read_uleb128(const uint8_t **p, const uint8_t *end)
{
	uint64_t result = 0;
	int bit = 0;
	uint8_t byte;
	do {
		if (*p >= end) {
			dyld_panic("uleb128 read past end");
		}
		byte = *(*p)++;
		result |= (uint64_t)(byte & 0x7f) << bit;
		bit += 7;
	} while (byte & 0x80);
	return result;
}

static inline int64_t
read_sleb128(const uint8_t **p, const uint8_t *end)
{
	int64_t result = 0;
	int bit = 0;
	uint8_t byte;
	do {
		if (*p >= end) {
			dyld_panic("sleb128 read past end");
		}
		byte = *(*p)++;
		result |= (int64_t)(byte & 0x7f) << bit;
		bit += 7;
	} while (byte & 0x80);
	if (bit < 64 && (byte & 0x40)) {
		result |= -((int64_t)1 << bit);
	}
	return result;
}

#endif
