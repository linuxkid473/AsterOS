/* Implements the two unconditionally-called static methods declared in
 * our tapi/tapi.h stub -- see that header for why the rest of the real
 * libtapi API is never needed on AsterOS (no shared libraries exist).
 */
#include <tapi/tapi.h>

namespace tapi {

bool
LinkerInterfaceFile::shouldPreferTextBasedStubFile(const std::string &path)
{
	(void)path;
	return false;
}

bool
LinkerInterfaceFile::areEquivalent(const std::string &tbdPath, const std::string &dylibPath)
{
	(void)tbdPath;
	(void)dylibPath;
	return false;
}

bool
APIVersion::isAtLeast(int major, int minor)
{
	(void)major;
	(void)minor;
	return false;
}

std::string
Version::getAsString()
{
	return "0.0.0";
}

std::string
Version::getFullVersionAsString()
{
	return "libtapi not available on AsterOS";
}

} // namespace tapi
