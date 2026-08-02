/*
 * Just enough Mach-O to load xnu's mach_kernel: the 64-bit header,
 * LC_SEGMENT_64 (to place each segment at its physical load address), and
 * LC_UNIXTHREAD's x86_THREAD_STATE64 (to find the entry point). Confirmed
 * against the actual built kernel with otool -l/-h before writing this.
 */
#ifndef DARWINBUILD_MACH_O_H
#define DARWINBUILD_MACH_O_H

#include <stdint.h>

#define MH_MAGIC_64 0xfeedfacfu

typedef struct {
	uint32_t magic;
	uint32_t cputype;
	uint32_t cpusubtype;
	uint32_t filetype;
	uint32_t ncmds;
	uint32_t sizeofcmds;
	uint32_t flags;
	uint32_t reserved;
} mach_header_64;

typedef struct {
	uint32_t cmd;
	uint32_t cmdsize;
} load_command;

#define LC_SEGMENT_64 0x19u
#define LC_UNIXTHREAD 0x5u

typedef struct {
	uint32_t cmd;
	uint32_t cmdsize;
	char segname[16];
	uint64_t vmaddr;
	uint64_t vmsize;
	uint64_t fileoff;
	uint64_t filesize;
	int32_t maxprot;
	int32_t initprot;
	uint32_t nsects;
	uint32_t flags;
} segment_command_64;

#define x86_THREAD_STATE64 4u

/* mirrors __darwin_x86_thread_state64 field order exactly */
typedef struct {
	uint64_t rax, rbx, rcx, rdx, rdi, rsi, rbp, rsp;
	uint64_t r8, r9, r10, r11, r12, r13, r14, r15;
	uint64_t rip;
	uint64_t rflags;
	uint64_t cs, fs, gs;
} x86_thread_state64_t;

typedef struct {
	uint32_t cmd;
	uint32_t cmdsize;
	uint32_t flavor;
	uint32_t count;
	/* x86_thread_state64_t follows when flavor == x86_THREAD_STATE64 */
} thread_command;

#endif /* DARWINBUILD_MACH_O_H */
