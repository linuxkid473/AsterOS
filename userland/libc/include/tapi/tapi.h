/* Minimal stand-in for Apple's internal libtapi (text-based stub / .tbd
 * file support). AsterOS has no shared libraries at all -- everything is
 * statically linked -- so ld64 never needs to actually parse a .tbd
 * file. Deliberately NOT defining TAPI_API_VERSION_MAJOR/MINOR makes
 * every "#if TAPI_API_VERSION..." guarded call site in Options.cpp take
 * its "no TAPI support" branch at compile time (the macros evaluate to 0
 * in an #if), so only the two unconditionally-called static methods
 * below need real bodies (see src/ld/Options.cpp's findFile()).
 */
#ifndef _TAPI_TAPI_H_
#define _TAPI_TAPI_H_

#include <string>

namespace tapi {

class LinkerInterfaceFile {
public:
	static bool shouldPreferTextBasedStubFile(const std::string &path);
	static bool areEquivalent(const std::string &tbdPath, const std::string &dylibPath);
	/* Only reachable via Options::findTAPIFile(), which always returns
	 * nullptr (see ld64_shim/tapi_stub.cpp) -- info.isInlined is never
	 * true, so InputFiles.cpp's call site is dead code that still needs
	 * to type-check. */
	std::string getInstallName() const { return std::string(); }
};

class APIVersion {
public:
	static bool isAtLeast(int major, int minor);
};

class Version {
public:
	static std::string getAsString();
	static std::string getFullVersionAsString();
};

} // namespace tapi

#endif /* _TAPI_TAPI_H_ */
