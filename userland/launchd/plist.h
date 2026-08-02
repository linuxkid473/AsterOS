/* Copyright (c) 2026 Vihaan Nathan
 *
 * A minimal parser for the one shape of XML property list launchd
 * actually needs: a top-level <dict> of LaunchDaemon keys. Not a general
 * plist library -- see plist.c's file comment for exactly what's
 * supported and what's deliberately out of scope for v1.
 */
#ifndef LAUNCHD_PLIST_H
#define LAUNCHD_PLIST_H

#define DAEMON_MAX_ARGS 16
#define DAEMON_MAX_ENV  16

struct daemon_config {
	char label[128];
	char *argv[DAEMON_MAX_ARGS + 1]; /* NULL-terminated, ready for execve */
	int argc;
	int run_at_load;
	int keep_alive;
	char *envp[DAEMON_MAX_ENV + 1]; /* "KEY=VALUE" strings, NULL-terminated */
	int nenv;
	char stdout_path[128];
	char stderr_path[128];
};

/* Parses one LaunchDaemon plist at `path` into `cfg`. Returns 0 on
 * success, -1 if the file can't be read, is malformed, or is missing
 * either required key (Label, ProgramArguments). All strings pointed to
 * by `cfg` are individually malloc'd and live for the process's
 * lifetime -- launchd never unloads a daemon config, so nothing ever
 * frees them. */
int plist_parse_daemon(const char *path, struct daemon_config *cfg);

#endif /* LAUNCHD_PLIST_H */
