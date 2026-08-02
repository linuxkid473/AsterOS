/* Minimal stand-in for Apple's CrashReporterClient.h.
 * AsterOS has no CrashReporter service to report to; ld64's Options.cpp
 * only needs the section-annotation struct/macros to exist so it can
 * define its (never-read) gCRAnnotations global.
 */
#ifndef _CRASH_REPORTER_CLIENT_H
#define _CRASH_REPORTER_CLIENT_H

#include <stdint.h>

#define CRASHREPORTER_ANNOTATIONS_VERSION 5
#define CRASHREPORTER_ANNOTATIONS_SECTION "__crash_info"

struct crashreporter_annotations_t {
	uint64_t version;
	uint64_t message;
	uint64_t signature_string;
	uint64_t backtrace;
	uint64_t message2;
	uint64_t thread;
	uint64_t dialog_mode;
};

/* Real Darwin sets gCRAnnotations.message here so a crash reporter can
 * show the last abort message; AsterOS has no crash reporter to consume
 * it, so this is a no-op rather than wiring up a shared gCRAnnotations
 * global across every translation unit that includes this header. */
#define CRSetCrashLogMessage(msg) ((void)(msg))
#define CRSetCrashLogMessage2(msg) ((void)(msg))

#endif /* _CRASH_REPORTER_CLIENT_H */
