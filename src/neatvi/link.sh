#!/bin/bash
set -euo pipefail
cd "$(dirname "${BASH_SOURCE[0]}")"
ROOT="$(cd ../.. && pwd)"

CLANG=/Applications/Xcode-beta.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/clang
OUT="$ROOT/build/neatvi_obj"

OBJS="vi ex lbuf mot sbuf ren dir syn reg led uc term rset rstr regex cmd tag conf"
NEATVI_OBJS=()
for b in $OBJS; do
	NEATVI_OBJS+=("$OUT/$b.o")
done

LIBC_OBJS=()
for f in "$ROOT"/build/libc_obj/*.o; do
	# neatvi's own regex.c provides real regcomp/regexec/regfree/regerror
	# (its own POSIX-shaped regex engine, not the system one) -- skip
	# libc's stub-only regex_stub.o for this link to avoid duplicate
	# symbols. libc itself is untouched; other binaries still get the stub.
	case "$(basename "$f")" in
	regex_stub.o) continue ;;
	esac
	LIBC_OBJS+=("$f")
done

"$CLANG" -target x86_64-apple-macos10.15 -nostdlib -static -e _start \
	"${NEATVI_OBJS[@]}" "${LIBC_OBJS[@]}" \
	-o "$OUT/neatvi"

echo "linked: $OUT/neatvi"
file "$OUT/neatvi"
