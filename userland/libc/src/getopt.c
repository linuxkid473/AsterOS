/* Standard POSIX getopt() -- classic single-pass permuting-free
 * implementation (options must precede operands, no GNU permutation).
 * Good enough for busybox applets, which mostly use their own
 * bb_getopt_ulflags anyway; this covers the stragglers that call the
 * plain libc getopt(). */
#include <unistd.h>
#include <string.h>
#include <stdio.h>

char *optarg;
int optind = 1;
int opterr = 1;
int optopt;

static int sp = 1;

int
getopt(int argc, char *const argv[], const char *optstring)
{
	/* glibc convention busybox relies on unconditionally (see its
	 * GETOPT_RESET() macro, include/libbb.h): optind==0 on entry means
	 * "full reset, start scanning from argv[1] again" -- without this,
	 * optind==0 makes us scan argv[0] (the program/applet name itself)
	 * as if it were the first operand, find no leading '-', and return
	 * -1 immediately without ever advancing optind past argv[0]. Every
	 * caller that then does `argv += optind` (e.g. ls_main) is left
	 * with argv[0] still pointing at the applet name, misinterpreting
	 * it as a positional argument (e.g. `ls` tries to stat a file
	 * named "ls"). */
	if (optind == 0) {
		optind = 1;
		sp = 1;
	}
	if (sp == 1) {
		if (optind >= argc || argv[optind][0] != '-' || argv[optind][1] == 0) {
			return -1;
		}
		if (strcmp(argv[optind], "--") == 0) {
			optind++;
			return -1;
		}
	}
	optopt = argv[optind][sp];
	const char *cp = strchr(optstring, optopt);
	if (optopt == ':' || !cp) {
		if (opterr) {
			fprintf(stderr, "%s: illegal option -- %c\n", argv[0], optopt);
		}
		if (argv[optind][++sp] == 0) {
			optind++;
			sp = 1;
		}
		return '?';
	}
	if (cp[1] == ':') {
		if (argv[optind][sp + 1] != 0) {
			optarg = &argv[optind][sp + 1];
			optind++;
		} else if (++optind < argc) {
			optarg = argv[optind];
			optind++;
		} else {
			if (opterr) {
				fprintf(stderr, "%s: option requires an argument -- %c\n", argv[0], optopt);
			}
			sp = 1;
			return '?';
		}
		sp = 1;
	} else {
		if (argv[optind][++sp] == 0) {
			sp = 1;
			optind++;
		}
		optarg = (void *)0;
	}
	return optopt;
}
