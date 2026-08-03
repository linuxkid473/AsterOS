/* Copyright (c) 2026 Vihaan Nathan
 *
 * Umbrella header, grown incrementally alongside the phase that adds
 * each class (see TODO.md's Foundation phase writeup for what's
 * implemented vs documented-as-missing). Matches real Foundation's own
 * umbrella-header convention: real .m client code does
 * `#import <Foundation/Foundation.h>` and gets everything.
 */
#ifndef FOUNDATION_FOUNDATION_H
#define FOUNDATION_FOUNDATION_H

/* NSObjCRuntime.h (via objc/objc.h) must come first: it defines
 * `nil`/`NULL`/Class/SEL self-sufficiently. CoreFoundation.h's own
 * MacTypes.h has `#ifndef nil`-guarded definitions of the same macros
 * that route through __DARWIN_NULL, a symbol nothing in this include
 * chain otherwise provides -- fine when CF is included alone (nothing
 * in this tree's C code needs `nil`), but a real conflict once
 * Objective-C code includes both. Whichever header runs its #define
 * first wins; only this order leaves both guards satisfied. */
#include <Foundation/NSObjCRuntime.h>
#include <CoreFoundation/CoreFoundation.h>

#include <Foundation/NSObject.h>
#include <Foundation/NSString.h>
#include <Foundation/NSValue.h>
#include <Foundation/NSNull.h>
#include <Foundation/NSArray.h>
#include <Foundation/NSDictionary.h>
#include <Foundation/NSSet.h>
#include <Foundation/NSData.h>
#include <Foundation/NSException.h>
#include <Foundation/NSError.h>
#include <Foundation/NSDate.h>
#include <Foundation/NSTimeZone.h>
#include <Foundation/NSLocale.h>
#include <Foundation/NSURL.h>
#include <Foundation/NSProcessInfo.h>
#include <Foundation/NSFileManager.h>
#include <Foundation/NSBundle.h>
#include <Foundation/NSNotification.h>
#include <Foundation/NSTimer.h>
#include <Foundation/NSRunLoop.h>
#include <Foundation/NSPropertyListSerialization.h>
#include <Foundation/NSJSONSerialization.h>
#include <Foundation/NSCoder.h>
#include <Foundation/NSKeyedArchiver.h>
#include <Foundation/NSUserDefaults.h>

#endif /* FOUNDATION_FOUNDATION_H */
