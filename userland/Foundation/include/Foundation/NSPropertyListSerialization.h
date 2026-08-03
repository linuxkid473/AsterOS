/* Copyright (c) 2026 Vihaan Nathan
 *
 * Real recursive-descent XML plist parser/writer, adapted from the
 * proven approach already in userland/launchd/plist.c (locate the root
 * tag via strstr, no <?xml ...?>/<!DOCTYPE> validation, no
 * entity/CDATA generality -- there's exactly one producer of these
 * files in practice) -- generalized here to build/walk real
 * NSDictionary/NSArray/NSString/NSNumber/NSData/NSDate object graphs
 * instead of launchd's flat C struct.
 *
 * v1 scope cuts, documented:
 * - Only NSPropertyListXMLFormat_v1_0 is actually implemented.
 *   NSPropertyListBinaryFormat_v1_0 is declared (for API completeness --
 *   real callers pattern-match on the format constant) but requesting it
 *   fails with an error rather than silently falling back to XML; real
 *   Apple's binary format is a whole separate, non-trivial wire format
 *   not worth implementing for what this tree needs.
 * - NSNull has no plist representation (matches real Apple property
 *   lists too -- NSNull was never a valid plist object type there
 *   either).
 * - <date> values use real ISO-8601 ("YYYY-MM-DDTHH:MM:SSZ", always UTC
 *   -- this tree's libc has no other timezone, see CFTimeZone.h).
 */
#ifndef FOUNDATION_NSPROPERTYLISTSERIALIZATION_H
#define FOUNDATION_NSPROPERTYLISTSERIALIZATION_H

#include <Foundation/NSObject.h>

#ifdef __cplusplus
extern "C" {
#endif

@class NSData;
@class NSError;

typedef NSUInteger NSPropertyListFormat;
enum {
	NSPropertyListXMLFormat_v1_0 = 100,
	NSPropertyListBinaryFormat_v1_0 = 200,
};

typedef NSUInteger NSPropertyListMutabilityOptions;
enum {
	NSPropertyListImmutable = 0,
};

@interface NSPropertyListSerialization : NSObject

+ (NSData *)dataWithPropertyList:(id)plist format:(NSPropertyListFormat)format options:(NSUInteger)opt error:(NSError **)error;
+ (id)propertyListWithData:(NSData *)data options:(NSUInteger)opt format:(NSPropertyListFormat *)format error:(NSError **)error;
+ (BOOL)propertyList:(id)plist isValidForFormat:(NSPropertyListFormat)format;

@end

#ifdef __cplusplus
}
#endif

#endif /* FOUNDATION_NSPROPERTYLISTSERIALIZATION_H */
