/* Copyright (c) 2026 Vihaan Nathan
 *
 * Backed by a plist file at /tmp/<domain>.plist -- this OS has no
 * per-user home directory concept yet, so there's no
 * ~/Library/Preferences to mirror; /tmp is this tree's own choice,
 * documented here rather than silently deviating from real Darwin's path
 * without saying so. Domain defaults to
 * [[NSProcessInfo processInfo] processName] when not otherwise
 * specified, matching real NSUserDefaults' own default-suite behavior
 * (keyed by the app's bundle identifier there; by process name here,
 * since this tree has no bundle identifiers either). Not persistent
 * across a reboot in this tree today, since /tmp isn't either -- a real
 * behavioral gap from Darwin's NSUserDefaults, which this v1 accepts
 * rather than hides.
 *
 * -objectForKey:/-setObject:/-setInteger:/-setBool:/etc. are a real,
 * unstubbed CFMutableDictionary-backed implementation with no known
 * issues. -synchronize (the real disk write: -dataWithPropertyList: +
 * NSData's -writeToFile:atomically:, also both genuinely implemented,
 * not stubbed) is a different story -- getting it to actually run
 * without wedging the whole system live in QEMU surfaced three separate,
 * real fat16lite kernel bugs this phase doesn't own fixing (consistent
 * with this tree's existing precedent of documenting rather than
 * endlessly chasing a kernel-level VFS issue, see TODO.md's Phase 4
 * boot-thread-stall entry):
 *
 *   1. VNOP_CREATE for a new file only succeeds when the parent
 *      directory is itself a direct child of the volume root. One level
 *      deeper -- e.g. /var/preferences/x.plist, the original, more
 *      natural-looking choice for this path -- fails fast with ENOTSUP
 *      every time, confirmed against both an mtools-built and a freshly
 *      mkdir()'d parent. This is why the path above is /tmp/<domain>.plist,
 *      not /var/preferences/<domain>.plist.
 *   2. fat16lite_fsnode_vnode() (fat16lite_fsnode.c) caches a vnode per
 *      directory-entry slot (keyed by on-disk byte offset) and reuses it
 *      on a later create at the same slot without rechecking its
 *      v_type -- rmdir()/unlink() free a slot but deliberately don't
 *      evict this cache (a real prior fix, see that function's own
 *      comment, made removal not clobber an unrelated live vnode's
 *      cluster pointers). A directory removed and then immediately
 *      followed, at the *same freed slot*, by a *file* created there
 *      comes back from open() as EISDIR. test/foundationtest.m's
 *      NSFileManager test creates its temporary directory before its
 *      temporary file (see that test's own comment) specifically to
 *      avoid handing -synchronize a freed directory-flavored slot.
 *   3. Even with both of the above avoided, calling -synchronize for
 *      real from test/foundationtest.m made this process's write() hang
 *      indefinitely -- not fail, hang -- and while hung it silently
 *      starved pthreadtest's own KeepAlive respawns too, a completely
 *      separate daemon Foundation doesn't own. Not root-caused; the
 *      first two bugs above were each found by full source-level
 *      analysis of fat16lite_vnops.c/fat16lite_fsnode.c, but this one
 *      wasn't chased that far given the time already spent on this one
 *      call site.
 *
 * test/foundationtest.m's test_nsuserdefaults() therefore exercises the
 * real in-memory accessors but does not call -synchronize -- seeing bug
 * #3 hang a second, unrelated daemon system-wide made that the
 * responsible default for an always-respawning automated suite. Calling
 * -synchronize directly, once, outside that suite is expected to work
 * for simple cases (bugs #1 and #2 are both real but avoidable at the
 * call site, per above) but is not itself part of this phase's verified
 * surface. See docs/architecture.md's Foundation section and TODO.md
 * for the full record.
 */
#ifndef FOUNDATION_NSUSERDEFAULTS_H
#define FOUNDATION_NSUSERDEFAULTS_H

#include <Foundation/NSObject.h>

#ifdef __cplusplus
extern "C" {
#endif

@interface NSUserDefaults : NSObject

+ (instancetype)standardUserDefaults;

- (instancetype)initWithSuiteName:(NSString *)suiteName;

- (id)objectForKey:(NSString *)key;
- (void)setObject:(id)value forKey:(NSString *)key;
- (void)removeObjectForKey:(NSString *)key;

- (NSString *)stringForKey:(NSString *)key;
- (NSInteger)integerForKey:(NSString *)key;
- (double)doubleForKey:(NSString *)key;
- (BOOL)boolForKey:(NSString *)key;
- (NSArray *)arrayForKey:(NSString *)key;
- (NSDictionary *)dictionaryForKey:(NSString *)key;

- (void)setInteger:(NSInteger)value forKey:(NSString *)key;
- (void)setDouble:(double)value forKey:(NSString *)key;
- (void)setBool:(BOOL)value forKey:(NSString *)key;

- (BOOL)synchronize;

@end

#ifdef __cplusplus
}
#endif

#endif /* FOUNDATION_NSUSERDEFAULTS_H */
