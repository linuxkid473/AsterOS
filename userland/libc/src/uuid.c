/* Real Darwin's gethostuuid() returns a stable hardware UUID persisted at
 * first boot; we have no NVRAM-backed identity store, so this generates a
 * fresh random one from real kernel entropy (getentropy) every call --
 * a valid UUID, just not a stable *host* identifier across calls. Fine
 * for LockFileManager's actual use (building a probably-unique lock file
 * name), wrong if anything ever needs cross-call/cross-reboot stability.
 * TODO: back with a real persisted identifier if that ever matters. */
#include <uuid/uuid.h>
#include <sys/random.h>
#include <stddef.h>
#include <stdio.h>
#include <time.h>

void
uuid_generate_random(uuid_t out)
{
	getentropy(out, sizeof(uuid_t));
	/* RFC 4122 version/variant bits, so it at least looks like a real UUID */
	out[6] = (out[6] & 0x0F) | 0x40;
	out[8] = (out[8] & 0x3F) | 0x80;
}

void uuid_generate(uuid_t out) { uuid_generate_random(out); }

void
uuid_unparse_lower(const uuid_t uu, uuid_string_t out)
{
	snprintf(out, sizeof(uuid_string_t),
	    "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
	    uu[0], uu[1], uu[2], uu[3], uu[4], uu[5], uu[6], uu[7],
	    uu[8], uu[9], uu[10], uu[11], uu[12], uu[13], uu[14], uu[15]);
}

void
uuid_unparse_upper(const uuid_t uu, uuid_string_t out)
{
	uuid_unparse_lower(uu, out);
	for (int i = 0; out[i]; i++) {
		if (out[i] >= 'a' && out[i] <= 'f') {
			out[i] -= 'a' - 'A';
		}
	}
}

void uuid_unparse(const uuid_t uu, uuid_string_t out) { uuid_unparse_upper(uu, out); }

int
gethostuuid(uuid_t out, const struct timespec *wait)
{
	(void)wait;
	uuid_generate_random(out);
	return 0;
}
