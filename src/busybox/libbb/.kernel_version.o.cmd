cmd_libbb/kernel_version.o := /Users/vihaannathan/Desktop/DarwinBuildCuzImBore/build/tools/bin/cc-nogroup -Wp,-MD,libbb/.kernel_version.o.d   -std=gnu99 -Iinclude -Ilibbb  -include include/autoconf.h -D_GNU_SOURCE -DNDEBUG  -DBB_VER='"1.36.1"'  -Wall -Wshadow -Wwrite-strings -Wundef -Wstrict-prototypes -Wunused -Wunused-parameter -Wunused-function -Wunused-value -Wmissing-prototypes -Wmissing-declarations -Wno-format-security -Wdeclaration-after-statement -Wold-style-definition -finline-limit=0 -fno-builtin-strlen -fomit-frame-pointer -ffunction-sections -fdata-sections  -funsigned-char -falign-functions=1 -falign-jumps=1 -falign-labels=1 -falign-loops=1 -fno-unwind-tables -fno-asynchronous-unwind-tables -fno-builtin-printf -Oz     -DKBUILD_BASENAME='"kernel_version"'  -DKBUILD_MODNAME='"kernel_version"' -c -o libbb/kernel_version.o libbb/kernel_version.c

deps_libbb/kernel_version.o := \
  libbb/kernel_version.c \
  include/libbb.h \
    $(wildcard include/config/feature/shadowpasswds.h) \
    $(wildcard include/config/use/bb/shadow.h) \
    $(wildcard include/config/selinux.h) \
    $(wildcard include/config/feature/utmp.h) \
    $(wildcard include/config/locale/support.h) \
    $(wildcard include/config/use/bb/pwd/grp.h) \
    $(wildcard include/config/lfs.h) \
    $(wildcard include/config/feature/buffers/go/on/stack.h) \
    $(wildcard include/config/feature/buffers/go/in/bss.h) \
    $(wildcard include/config/extra/cflags.h) \
    $(wildcard include/config/variable/arch/pagesize.h) \
    $(wildcard include/config/feature/verbose.h) \
    $(wildcard include/config/feature/etc/services.h) \
    $(wildcard include/config/feature/ipv6.h) \
    $(wildcard include/config/feature/seamless/xz.h) \
    $(wildcard include/config/feature/seamless/lzma.h) \
    $(wildcard include/config/feature/seamless/bz2.h) \
    $(wildcard include/config/feature/seamless/gz.h) \
    $(wildcard include/config/feature/seamless/z.h) \
    $(wildcard include/config/float/duration.h) \
    $(wildcard include/config/feature/check/names.h) \
    $(wildcard include/config/feature/prefer/applets.h) \
    $(wildcard include/config/long/opts.h) \
    $(wildcard include/config/feature/pidfile.h) \
    $(wildcard include/config/feature/syslog.h) \
    $(wildcard include/config/feature/syslog/info.h) \
    $(wildcard include/config/warn/simple/msg.h) \
    $(wildcard include/config/feature/individual.h) \
    $(wildcard include/config/shell/ash.h) \
    $(wildcard include/config/shell/hush.h) \
    $(wildcard include/config/echo.h) \
    $(wildcard include/config/sleep.h) \
    $(wildcard include/config/printf.h) \
    $(wildcard include/config/test.h) \
    $(wildcard include/config/test1.h) \
    $(wildcard include/config/test2.h) \
    $(wildcard include/config/kill.h) \
    $(wildcard include/config/killall.h) \
    $(wildcard include/config/killall5.h) \
    $(wildcard include/config/chown.h) \
    $(wildcard include/config/ls.h) \
    $(wildcard include/config/xxx.h) \
    $(wildcard include/config/route.h) \
    $(wildcard include/config/feature/hwib.h) \
    $(wildcard include/config/desktop.h) \
    $(wildcard include/config/feature/crond/d.h) \
    $(wildcard include/config/feature/setpriv/capabilities.h) \
    $(wildcard include/config/run/init.h) \
    $(wildcard include/config/feature/securetty.h) \
    $(wildcard include/config/pam.h) \
    $(wildcard include/config/use/bb/crypt.h) \
    $(wildcard include/config/feature/adduser/to/group.h) \
    $(wildcard include/config/feature/del/user/from/group.h) \
    $(wildcard include/config/ioctl/hex2str/error.h) \
    $(wildcard include/config/feature/editing.h) \
    $(wildcard include/config/feature/editing/history.h) \
    $(wildcard include/config/feature/tab/completion.h) \
    $(wildcard include/config/feature/username/completion.h) \
    $(wildcard include/config/feature/editing/fancy/prompt.h) \
    $(wildcard include/config/feature/editing/savehistory.h) \
    $(wildcard include/config/feature/editing/vi.h) \
    $(wildcard include/config/feature/editing/save/on/exit.h) \
    $(wildcard include/config/pmap.h) \
    $(wildcard include/config/feature/show/threads.h) \
    $(wildcard include/config/feature/ps/additional/columns.h) \
    $(wildcard include/config/feature/topmem.h) \
    $(wildcard include/config/feature/top/smp/process.h) \
    $(wildcard include/config/pgrep.h) \
    $(wildcard include/config/pkill.h) \
    $(wildcard include/config/pidof.h) \
    $(wildcard include/config/sestatus.h) \
    $(wildcard include/config/unicode/support.h) \
    $(wildcard include/config/feature/mtab/support.h) \
    $(wildcard include/config/feature/clean/up.h) \
    $(wildcard include/config/feature/devfs.h) \
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
  /Users/vihaannathan/Desktop/DarwinBuildCuzImBore/userland/libc/include/ctype.h \
  /Users/vihaannathan/Desktop/DarwinBuildCuzImBore/userland/libc/include/runetype.h \
  /Users/vihaannathan/Desktop/DarwinBuildCuzImBore/userland/libc/include/dirent.h \
  /Users/vihaannathan/Desktop/DarwinBuildCuzImBore/userland/libc/include/errno.h \
  /Users/vihaannathan/Desktop/DarwinBuildCuzImBore/userland/libc/include/fcntl.h \
  /Users/vihaannathan/Desktop/DarwinBuildCuzImBore/userland/libc/include/inttypes.h \
  /Users/vihaannathan/Desktop/DarwinBuildCuzImBore/userland/libc/include/netdb.h \
  /Users/vihaannathan/Desktop/DarwinBuildCuzImBore/userland/libc/include/sys/socket.h \
  /Users/vihaannathan/Desktop/DarwinBuildCuzImBore/userland/libc/include/setjmp.h \
  /Users/vihaannathan/Desktop/DarwinBuildCuzImBore/userland/libc/include/signal.h \
  /Users/vihaannathan/Desktop/DarwinBuildCuzImBore/userland/libc/include/paths.h \
  /Users/vihaannathan/Desktop/DarwinBuildCuzImBore/userland/libc/include/stdlib.h \
  /Users/vihaannathan/Desktop/DarwinBuildCuzImBore/userland/libc/include/sys/wait.h \
  /Applications/Xcode-beta.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/lib/clang/21/include/__stddef_rsize_t.h \
  /Users/vihaannathan/Desktop/DarwinBuildCuzImBore/userland/libc/include/string.h \
  /Users/vihaannathan/Desktop/DarwinBuildCuzImBore/userland/libc/include/libgen.h \
  /Users/vihaannathan/Desktop/DarwinBuildCuzImBore/userland/libc/include/poll.h \
  /Users/vihaannathan/Desktop/DarwinBuildCuzImBore/userland/libc/include/sys/ioctl.h \
  /Users/vihaannathan/Desktop/DarwinBuildCuzImBore/userland/libc/include/sys/mman.h \
  /Users/vihaannathan/Desktop/DarwinBuildCuzImBore/userland/libc/include/sys/stat.h \
  /Users/vihaannathan/Desktop/DarwinBuildCuzImBore/userland/libc/include/time.h \
  /Users/vihaannathan/Desktop/DarwinBuildCuzImBore/userland/libc/include/sys/sysmacros.h \
  /Users/vihaannathan/Desktop/DarwinBuildCuzImBore/userland/libc/include/termios.h \
  /Users/vihaannathan/Desktop/DarwinBuildCuzImBore/userland/libc/include/sys/param.h \
  /Users/vihaannathan/Desktop/DarwinBuildCuzImBore/userland/libc/include/pwd.h \
  /Users/vihaannathan/Desktop/DarwinBuildCuzImBore/userland/libc/include/grp.h \
  /Users/vihaannathan/Desktop/DarwinBuildCuzImBore/userland/libc/include/netinet/in.h \
  include/xatonum.h \
  /Users/vihaannathan/Desktop/DarwinBuildCuzImBore/userland/libc/include/sys/utsname.h \

libbb/kernel_version.o: $(deps_libbb/kernel_version.o)

$(deps_libbb/kernel_version.o):
