/*
 * boot_args / flattened-device-tree structures, copied to match exactly
 * what xnu expects -- ground-truthed against the real source:
 *   src/xnu/pexpert/pexpert/i386/boot.h   (struct boot_args)
 *   src/xnu/pexpert/pexpert/device_tree.h (flattened device tree format)
 * Redefined here rather than pulled in from the xnu tree so this
 * bootloader has no dependency on xnu's (enormous, kernel-oriented)
 * header set -- it only needs this one ABI contract.
 */
#ifndef DARWINBUILD_DARWIN_BOOT_H
#define DARWINBUILD_DARWIN_BOOT_H

#include <stdint.h>

#define kBootArgsRevision 0
#define kBootArgsVersion2 2
#define kBootArgsEfiMode64 64
#define BOOT_LINE_LENGTH 1024

typedef struct {
	uint32_t v_baseAddr;
	uint32_t v_display;
	uint32_t v_rowBytes;
	uint32_t v_width;
	uint32_t v_height;
	uint32_t v_depth;
} Boot_VideoV1;

typedef struct {
	uint32_t v_display;
	uint32_t v_rowBytes;
	uint32_t v_width;
	uint32_t v_height;
	uint32_t v_depth;
	uint8_t v_rotate;
	uint8_t v_resv_byte[3];
	uint32_t v_resv[6];
	uint64_t v_baseAddr;
} Boot_Video;

typedef struct {
	uint16_t Revision;
	uint16_t Version;

	uint8_t efiMode;
	uint8_t debugMode;
	uint16_t flags;

	char CommandLine[BOOT_LINE_LENGTH];

	uint32_t MemoryMap;
	uint32_t MemoryMapSize;
	uint32_t MemoryMapDescriptorSize;
	uint32_t MemoryMapDescriptorVersion;

	Boot_VideoV1 VideoV1;

	uint32_t deviceTreeP;
	uint32_t deviceTreeLength;

	uint32_t kaddr;
	uint32_t ksize;

	uint32_t efiRuntimeServicesPageStart;
	uint32_t efiRuntimeServicesPageCount;
	uint64_t efiRuntimeServicesVirtualPageStart;

	uint32_t efiSystemTable;
	uint32_t kslide;

	uint32_t performanceDataStart;
	uint32_t performanceDataSize;

	uint32_t keyStoreDataStart;
	uint32_t keyStoreDataSize;
	uint64_t bootMemStart;
	uint64_t bootMemSize;
	uint64_t PhysicalMemorySize;
	uint64_t FSBFrequency;
	uint64_t pciConfigSpaceBaseAddress;
	uint32_t pciConfigSpaceStartBusNumber;
	uint32_t pciConfigSpaceEndBusNumber;
	uint32_t csrActiveConfig;
	uint32_t csrCapabilities;
	uint32_t boot_SMC_plimit;
	uint16_t bootProgressMeterStart;
	uint16_t bootProgressMeterEnd;
	Boot_Video Video;

	uint32_t apfsDataStart;
	uint32_t apfsDataSize;

	uint32_t __reserved4[710];
} boot_args;

_Static_assert(sizeof(boot_args) == 4096, "boot_args must be 4096 bytes");

/* EFI memory map descriptor, as boot_args->MemoryMap expects (matches
 * EFI_MEMORY_DESCRIPTOR exactly -- xnu re-declares its own copy in
 * pexpert/i386/boot.h under the name EfiMemoryRange). */
typedef struct {
	uint32_t Type;
	uint32_t Pad;
	uint64_t PhysicalStart;
	uint64_t VirtualStart;
	uint64_t NumberOfPages;
	uint64_t Attribute;
} EfiMemoryRange;

/* ---- Flattened device tree (pexpert/pexpert/device_tree.h) ---- */
#define kPropNameLength 32

typedef struct {
	char name[kPropNameLength];
	uint32_t length;
	/* value bytes follow, padded to a 4-byte boundary */
} DeviceTreeNodeProperty;

typedef struct {
	uint32_t nProperties;
	uint32_t nChildren;
} DeviceTreeNode;

#endif /* DARWINBUILD_DARWIN_BOOT_H */
