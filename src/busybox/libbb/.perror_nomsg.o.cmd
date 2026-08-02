cmd_libbb/perror_nomsg.o := /Users/vihaannathan/Desktop/DarwinBuildCuzImBore/build/tools/bin/cc-nogroup -Wp,-MD,libbb/.perror_nomsg.o.d   -std=gnu99 -Iinclude -Ilibbb  -include include/autoconf.h -D_GNU_SOURCE -DNDEBUG  -DBB_VER='"1.36.1"'  -Wall -Wshadow -Wwrite-strings -Wundef -Wstrict-prototypes -Wunused -Wunused-parameter -Wunused-function -Wunused-value -Wmissing-prototypes -Wmissing-declarations -Wno-format-security -Wdeclaration-after-statement -Wold-style-definition -finline-limit=0 -fno-builtin-strlen -fomit-frame-pointer -ffunction-sections -fdata-sections  -funsigned-char -falign-functions=1 -falign-jumps=1 -falign-labels=1 -falign-loops=1 -fno-unwind-tables -fno-asynchronous-unwind-tables -fno-builtin-printf -Oz     -DKBUILD_BASENAME='"perror_nomsg"'  -DKBUILD_MODNAME='"perror_nomsg"' -c -o libbb/perror_nomsg.o libbb/perror_nomsg.c

deps_libbb/perror_nomsg.o := \
  libbb/perror_nomsg.c \
  include/platform.h \
    $(wildcard include/config/werror.h) \
    $(wildcard include/config/big/endian.h) \
    $(wildcard include/config/little/endian.h) \
    $(wildcard include/config/nommu.h) \
  /Users/vihaannathan/Desktop/DarwinBuildCuzImBore/userland/libc/include/limits.h \
  /Users/vihaannathan/Desktop/DarwinBuildCuzImBore/userland/libc/include/sys/resource.h \
  /Users/vihaannathan/Desktop/DarwinBuildCuzImBore/userland/libc/include/sys/time.h \
  /Users/vihaannathan/Desktop/DarwinBuildCuzImBore/userland/libc/include/sys/types.h \
  /Applications/Xcode-beta.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/lib/clang/21/include/stddef.h \
  /Applications/Xcode-beta.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/lib/clang/21/include/__stddef_header_macro.h \
  /Applications/Xcode-beta.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/lib/clang/21/include/__stddef_ptrdiff_t.h \
  /Applications/Xcode-beta.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/lib/clang/21/include/__stddef_size_t.h \
  /Applications/Xcode-beta.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/lib/clang/21/include/__stddef_wchar_t.h \
  /Applications/Xcode-beta.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/lib/clang/21/include/__stddef_null.h \
  /Applications/Xcode-beta.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/lib/clang/21/include/__stddef_offsetof.h \
  /Users/vihaannathan/Desktop/DarwinBuildCuzImBore/userland/libc/include/machine/endian.h \
  /Users/vihaannathan/Desktop/DarwinBuildCuzImBore/userland/libc/include/sys/_endian.h \
  /Users/vihaannathan/Desktop/DarwinBuildCuzImBore/userland/libc/include/sys/cdefs.h \
  /Users/vihaannathan/Desktop/DarwinBuildCuzImBore/userland/libc/include/sys/_symbol_aliasing.h \
  /Users/vihaannathan/Desktop/DarwinBuildCuzImBore/userland/libc/include/sys/_posix_availability.h \
  /Users/vihaannathan/Desktop/DarwinBuildCuzImBore/userland/libc/include/sys/_types.h \
  /Users/vihaannathan/Desktop/DarwinBuildCuzImBore/userland/libc/include/machine/_types.h \
  /Users/vihaannathan/Desktop/DarwinBuildCuzImBore/userland/libc/include/i386/_types.h \
  /Users/vihaannathan/Desktop/DarwinBuildCuzImBore/userland/libc/include/sys/_pthread/_pthread_types.h \
  /Applications/Xcode-beta.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/lib/clang/21/include/stdint.h \
  /Applications/Xcode-beta.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/lib/clang/21/include/stdbool.h \
  /Users/vihaannathan/Desktop/DarwinBuildCuzImBore/userland/libc/include/unistd.h \
  /Users/vihaannathan/Desktop/DarwinBuildCuzImBore/userland/libc/include/sys/_types/_uuid_t.h \
  /Users/vihaannathan/Desktop/DarwinBuildCuzImBore/userland/libc/include/stdio.h \
  /Applications/Xcode-beta.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/lib/clang/21/include/stdarg.h \
  /Applications/Xcode-beta.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/lib/clang/21/include/__stdarg_header_macro.h \
  /Applications/Xcode-beta.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/lib/clang/21/include/__stdarg___gnuc_va_list.h \
  /Applications/Xcode-beta.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/lib/clang/21/include/__stdarg_va_list.h \
  /Applications/Xcode-beta.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/lib/clang/21/include/__stdarg_va_arg.h \
  /Applications/Xcode-beta.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/lib/clang/21/include/__stdarg___va_copy.h \
  /Applications/Xcode-beta.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/lib/clang/21/include/__stdarg_va_copy.h \

libbb/perror_nomsg.o: $(deps_libbb/perror_nomsg.o)

$(deps_libbb/perror_nomsg.o):
