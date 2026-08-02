/* Stand-ins for two real ld64 files excluded from this build (see
 * userland/ld64_shim/build.sh's comment on CXX_SOURCES): passes/
 * bitcode_bundle.cpp (needs libxar) and parsers/textstub_dylib_file.cpp
 * (needs real libtapi, see tapi/tapi.h). AsterOS's clang never emits
 * LLVM bitcode and no input is ever a .tbd text-based stub (this is a
 * fully static system -- no dylibs at all), so both real
 * implementations' behavior collapses to "nothing to do" / "never a
 * match" for every input we actually feed ld64.
 */
#include "passes/bitcode_bundle.h"
#include "parsers/textstub_dylib_file.hpp"

namespace ld {
namespace passes {
namespace bitcode_bundle {

void
doPass(const Options &opts, ld::Internal &internal)
{
	/* Real BitcodeBundle::doPass() no-ops whenever
	 * state.filesWithBitcode/state.ltoBitcodePath are both empty --
	 * always true here, since nothing in this toolchain ever produces
	 * bitcode. */
	(void)opts;
	(void)internal;
}

} // namespace bitcode_bundle
} // namespace passes
} // namespace ld

namespace textstub {
namespace dylib {

bool
isTextStubFile(const uint8_t *fileContent, uint64_t fileLength, const char *path)
{
	(void)fileContent;
	(void)fileLength;
	(void)path;
	return false; /* no .tbd inputs ever exist on AsterOS -- no dylibs at all */
}

ld::dylib::File *
parse(const uint8_t *fileContent, uint64_t fileLength, const char *path, time_t modTime,
    const Options &opts, ld::File::Ordinal ordinal, bool bundleLoader, bool indirectDylib)
{
	(void)fileContent; (void)fileLength; (void)path; (void)modTime;
	(void)opts; (void)ordinal; (void)bundleLoader; (void)indirectDylib;
	return NULL; /* unreachable: isTextStubFile() above always gates this */
}

ld::dylib::File *
parse(const char *path, tapi::LinkerInterfaceFile *file, time_t modTime, ld::File::Ordinal ordinal,
    const Options &opts, bool indirectDylib)
{
	(void)path; (void)file; (void)modTime; (void)ordinal; (void)opts; (void)indirectDylib;
	return NULL; /* unreachable: only called via info.isInlined, always false */
}

} // namespace dylib
} // namespace textstub
