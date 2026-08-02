/* Copyright (c) 2026 Vihaan Nathan
 *
 * Hand-rolled XML plist parser, deliberately narrow: it understands
 * exactly the tags a LaunchDaemon plist uses (<dict>/<key>/<string>/
 * <array>/<true/>/<false/>/<integer>), located by scanning for the root
 * "<dict>" rather than validating the <?xml ...?> prolog or the
 * <!DOCTYPE> line -- there's exactly one producer of these files (us),
 * so no DTD/entity/CDATA generality is needed. Keys this project doesn't
 * support yet (Sockets, WatchPaths, StartInterval, dict-form KeepAlive,
 * UserName, ...) are walked and discarded by skip_value() rather than
 * causing a parse failure, so an otherwise-valid plist with one
 * unsupported key still loads -- but they are genuinely ignored, not
 * silently half-applied.
 */
#include "plist.h"
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>

static void
skip_ws(char **pp)
{
	char *p = *pp;
	for (;;) {
		while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r') {
			p++;
		}
		if (p[0] == '<' && p[1] == '!' && p[2] == '-' && p[3] == '-') {
			char *end = strstr(p, "-->");
			if (!end) {
				break;
			}
			p = end + 3;
			continue;
		}
		break;
	}
	*pp = p;
}

static int
consume(char **pp, const char *lit)
{
	skip_ws(pp);
	size_t n = strlen(lit);
	if (strncmp(*pp, lit, n) != 0) {
		return 0;
	}
	*pp += n;
	return 1;
}

/* Returns a malloc'd copy of everything up to (not including) close_tag,
 * and advances *pp past close_tag. NULL if close_tag never appears. */
static char *
text_until(char **pp, const char *close_tag)
{
	char *end = strstr(*pp, close_tag);
	if (!end) {
		return NULL;
	}
	size_t len = (size_t)(end - *pp);
	char *out = malloc(len + 1);
	if (!out) {
		return NULL;
	}
	memcpy(out, *pp, len);
	out[len] = '\0';
	*pp = end + strlen(close_tag);
	return out;
}

static int
parse_key(char **pp, char *out, size_t outsz)
{
	if (!consume(pp, "<key>")) {
		return -1;
	}
	char *text = text_until(pp, "</key>");
	if (!text) {
		return -1;
	}
	strncpy(out, text, outsz - 1);
	out[outsz - 1] = '\0';
	free(text);
	return 0;
}

static int
parse_string_value(char **pp, char **out)
{
	if (!consume(pp, "<string>")) {
		return -1;
	}
	*out = text_until(pp, "</string>");
	return *out ? 0 : -1;
}

static int
parse_bool_value(char **pp, int *out)
{
	if (consume(pp, "<true/>")) {
		*out = 1;
		return 0;
	}
	if (consume(pp, "<false/>")) {
		*out = 0;
		return 0;
	}
	return -1;
}

/* Walks and discards one well-formed value of any supported shape,
 * recursing into <array>/<dict> -- how an unsupported top-level key
 * (e.g. a dict-form KeepAlive) gets skipped without aborting the whole
 * parse. */
static int
skip_value(char **pp)
{
	if (consume(pp, "<true/>") || consume(pp, "<false/>")) {
		return 0;
	}
	if (consume(pp, "<string>")) {
		char *s = text_until(pp, "</string>");
		free(s);
		return s ? 0 : -1;
	}
	if (consume(pp, "<integer>")) {
		char *s = text_until(pp, "</integer>");
		free(s);
		return s ? 0 : -1;
	}
	if (consume(pp, "<array>")) {
		for (;;) {
			if (consume(pp, "</array>")) {
				return 0;
			}
			if (skip_value(pp) != 0) {
				return -1;
			}
		}
	}
	if (consume(pp, "<dict>")) {
		for (;;) {
			if (consume(pp, "</dict>")) {
				return 0;
			}
			char key[128];
			if (parse_key(pp, key, sizeof(key)) != 0) {
				return -1;
			}
			if (skip_value(pp) != 0) {
				return -1;
			}
		}
	}
	return -1; /* not a tag this parser recognizes at all */
}

static int
parse_string_array(char **pp, char **arr, int max, int *count)
{
	if (!consume(pp, "<array>")) {
		return -1;
	}
	*count = 0;
	for (;;) {
		if (consume(pp, "</array>")) {
			return 0;
		}
		char *s;
		if (parse_string_value(pp, &s) != 0) {
			return -1;
		}
		if (*count < max) {
			arr[(*count)++] = s;
		} else {
			free(s); /* silently dropped past DAEMON_MAX_ARGS -- documented fixed ceiling */
		}
	}
}

static int
parse_env_dict(char **pp, struct daemon_config *cfg)
{
	if (!consume(pp, "<dict>")) {
		return -1;
	}
	for (;;) {
		if (consume(pp, "</dict>")) {
			return 0;
		}
		char key[128];
		if (parse_key(pp, key, sizeof(key)) != 0) {
			return -1;
		}
		char *val;
		if (parse_string_value(pp, &val) != 0) {
			return -1;
		}
		if (cfg->nenv < DAEMON_MAX_ENV) {
			size_t len = strlen(key) + 1 + strlen(val) + 1;
			char *kv = malloc(len);
			if (kv) {
				snprintf(kv, len, "%s=%s", key, val);
				cfg->envp[cfg->nenv++] = kv;
			}
		}
		free(val);
	}
}

int
plist_parse_daemon(const char *path, struct daemon_config *cfg)
{
	int fd = open(path, O_RDONLY, 0);
	if (fd < 0) {
		return -1;
	}
	struct stat st;
	if (fstat(fd, &st) != 0 || st.st_size <= 0) {
		close(fd);
		return -1;
	}
	char *buf = malloc((size_t)st.st_size + 1);
	if (!buf) {
		close(fd);
		return -1;
	}
	ssize_t n = read(fd, buf, (size_t)st.st_size);
	close(fd);
	if (n < 0) {
		free(buf);
		return -1;
	}
	buf[n] = '\0';

	memset(cfg, 0, sizeof(*cfg));

	char *p = strstr(buf, "<dict>");
	if (!p) {
		free(buf);
		return -1;
	}
	p += 6;

	int have_label = 0, have_args = 0;
	int ok = 1;

	for (;;) {
		if (consume(&p, "</dict>")) {
			break;
		}
		char key[128];
		if (parse_key(&p, key, sizeof(key)) != 0) {
			ok = 0;
			break;
		}
		if (strcmp(key, "Label") == 0) {
			char *s;
			if (parse_string_value(&p, &s) != 0) {
				ok = 0;
				break;
			}
			strncpy(cfg->label, s, sizeof(cfg->label) - 1);
			free(s);
			have_label = 1;
		} else if (strcmp(key, "ProgramArguments") == 0) {
			if (parse_string_array(&p, cfg->argv, DAEMON_MAX_ARGS, &cfg->argc) != 0) {
				ok = 0;
				break;
			}
			cfg->argv[cfg->argc] = NULL;
			have_args = cfg->argc > 0;
		} else if (strcmp(key, "RunAtLoad") == 0) {
			if (parse_bool_value(&p, &cfg->run_at_load) != 0) {
				ok = 0;
				break;
			}
		} else if (strcmp(key, "KeepAlive") == 0) {
			if (parse_bool_value(&p, &cfg->keep_alive) != 0) {
				ok = 0;
				break;
			}
		} else if (strcmp(key, "EnvironmentVariables") == 0) {
			if (parse_env_dict(&p, cfg) != 0) {
				ok = 0;
				break;
			}
		} else if (strcmp(key, "StandardOutPath") == 0) {
			char *s;
			if (parse_string_value(&p, &s) != 0) {
				ok = 0;
				break;
			}
			strncpy(cfg->stdout_path, s, sizeof(cfg->stdout_path) - 1);
			free(s);
		} else if (strcmp(key, "StandardErrorPath") == 0) {
			char *s;
			if (parse_string_value(&p, &s) != 0) {
				ok = 0;
				break;
			}
			strncpy(cfg->stderr_path, s, sizeof(cfg->stderr_path) - 1);
			free(s);
		} else {
			if (skip_value(&p) != 0) {
				ok = 0;
				break;
			}
		}
	}

	free(buf);
	return (ok && have_label && have_args) ? 0 : -1;
}
