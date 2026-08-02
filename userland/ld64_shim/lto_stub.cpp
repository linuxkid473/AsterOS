/* Stand-in for ld64's real src/ld/parsers/lto_file.cpp, which requires
 * libLTO.dylib (LLVM's dynamically-loaded LTO plugin). AsterOS's clang
 * never emits LLVM bitcode objects (no -flto support in our port), so
 * ld64 never actually needs real LTO/bitcode handling here -- this
 * mirrors real ld64's own graceful fallback for when libLTO.dylib can't
 * be found: version() returns NULL and every input file is reported as
 * "not an object file" (correct, since none of our inputs ever are).
 */
#include "lto_file.h"

namespace lto {

const char *
version()
{
	return NULL;
}

unsigned int
runtime_api_version()
{
	return 0;
}

unsigned int
static_api_version()
{
	return 0;
}

void
set_library(const char *dylib)
{
	(void)dylib;
}

bool
libLTOisLoaded()
{
	return false;
}

const char *
archName(const uint8_t *fileContent, uint64_t fileLength)
{
	(void)fileContent;
	(void)fileLength;
	return NULL;
}

bool
isObjectFile(const uint8_t *fileContent, uint64_t fileLength, cpu_type_t architecture, cpu_subtype_t subarch)
{
	(void)fileContent;
	(void)fileLength;
	(void)architecture;
	(void)subarch;
	return false;
}

bool
hasObjCCategory(const uint8_t *fileContent, uint64_t fileLength)
{
	(void)fileContent;
	(void)fileLength;
	return false;
}

ld::relocatable::File *
parse(const uint8_t *fileContent, uint64_t fileLength, const char *path, time_t modTime,
    ld::File::Ordinal ordinal, cpu_type_t architecture, cpu_subtype_t subarch, bool logAllFiles,
    bool verboseOptimizationHints)
{
	(void)fileContent;
	(void)fileLength;
	(void)path;
	(void)modTime;
	(void)ordinal;
	(void)architecture;
	(void)subarch;
	(void)logAllFiles;
	(void)verboseOptimizationHints;
	return NULL;
}

bool
optimize(const std::vector<const ld::Atom *> &allAtoms, ld::Internal &state, const OptimizeOptions &options,
    ld::File::AtomHandler &handler, std::vector<const ld::Atom *> &newAtoms,
    std::vector<const char *> &additionalUndefines)
{
	(void)allAtoms;
	(void)state;
	(void)options;
	(void)handler;
	(void)newAtoms;
	(void)additionalUndefines;
	return false;
}

} // namespace lto
