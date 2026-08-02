/* Copyright (c) 2026 Vihaan Nathan
 *
 * Walks the export trie (LC_DYLD_INFO_ONLY export_off) to resolve a symbol
 * name to an address within a given image. The trie factors out shared
 * name prefixes into a tree of edge-labeled nodes; each node that is
 * itself an exported symbol starts with a non-zero "terminal size"
 * followed by flags + an address offset from the image's mach_header.
 */
#include "image.h"
#include "uleb.h"
#include <string.h>

/* EXPORT_SYMBOL_FLAGS_* aren't in our vendored loader.h (they live in a
 * separate part of dyld's own headers we didn't pull in) -- only the ones
 * we need to detect and refuse. */
#define EXPORT_SYMBOL_FLAGS_REEXPORT 0x08
#define EXPORT_SYMBOL_FLAGS_STUB_AND_RESOLVER 0x10

uint64_t
image_resolve_export(struct image *im, const char *symbol)
{
	if (!im->export_start) {
		return 0;
	}
	const uint8_t *p = im->export_start;
	const uint8_t *end = im->export_end;
	const char *s = symbol;

	for (;;) {
		uint64_t terminal_size = read_uleb128(&p, end);
		const uint8_t *children = p + terminal_size;

		if (*s == '\0') {
			if (terminal_size == 0) {
				return 0;
			}
			uint64_t flags = read_uleb128(&p, end);
			if (flags & (EXPORT_SYMBOL_FLAGS_REEXPORT | EXPORT_SYMBOL_FLAGS_STUB_AND_RESOLVER)) {
				return 0;	/* re-exports/resolvers: not supported in v1 */
			}
			uint64_t offset = read_uleb128(&p, end);
			return (uint64_t)(uintptr_t)im->mh + offset;
		}

		p = children;
		uint8_t child_count = *p++;
		const uint8_t *next = 0;
		for (uint8_t i = 0; i < child_count; i++) {
			const char *label = (const char *)p;
			size_t label_len = strlen(label);
			p += label_len + 1;
			uint64_t child_off = read_uleb128(&p, end);
			if (strncmp(s, label, label_len) == 0) {
				next = im->export_start + child_off;
				s += label_len;
				break;
			}
		}
		if (!next) {
			return 0;
		}
		p = next;
	}
}
