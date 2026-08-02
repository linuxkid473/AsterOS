cmd_libbb/hash_md5_sha_x86-64_shaNI.o := /Users/vihaannathan/Desktop/DarwinBuildCuzImBore/build/tools/bin/cc-nogroup -Wp,-MD,libbb/.hash_md5_sha_x86-64_shaNI.o.d   -std=gnu99 -Iinclude -Ilibbb  -include include/autoconf.h -D_GNU_SOURCE -DNDEBUG  -DBB_VER='"1.36.1"'  -Wall -Wshadow -Wwrite-strings -Wundef -Wstrict-prototypes -Wunused -Wunused-parameter -Wunused-function -Wunused-value -Wmissing-prototypes -Wmissing-declarations -Wno-format-security -Wdeclaration-after-statement -Wold-style-definition -finline-limit=0 -fno-builtin-strlen -fomit-frame-pointer -ffunction-sections -fdata-sections  -funsigned-char -falign-functions=1 -falign-jumps=1 -falign-labels=1 -falign-loops=1 -fno-unwind-tables -fno-asynchronous-unwind-tables -fno-builtin-printf -Oz        -c -o libbb/hash_md5_sha_x86-64_shaNI.o libbb/hash_md5_sha_x86-64_shaNI.S

deps_libbb/hash_md5_sha_x86-64_shaNI.o := \
  libbb/hash_md5_sha_x86-64_shaNI.S \
    $(wildcard include/config/sha1/hwaccel.h) \

libbb/hash_md5_sha_x86-64_shaNI.o: $(deps_libbb/hash_md5_sha_x86-64_shaNI.o)

$(deps_libbb/hash_md5_sha_x86-64_shaNI.o):
