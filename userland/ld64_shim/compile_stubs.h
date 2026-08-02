/* Genuinely missing from the ld64-530 source export (not present
 * anywhere in the upstream tag) -- appears to be a placeholder some
 * internal Apple build step normally generates: a script embedded as a
 * string, written into a link crash-snapshot directory so the failing
 * link can be reproduced standalone. Not needed for the core compile
 * path (only for crash-snapshot debugging), so an empty script is
 * correct rather than a hack. */
static const char compile_stubs[] = "";
