/* Copyright (c) 2026 Vihaan Nathan
 *
 * Minimal keyed archiving, wire format = a plain plist (XML, via
 * NSPropertyListSerialization -- NSPropertyListXMLFormat_v1_0 is a
 * real historical NSKeyedArchiver output format, not an invented one)
 * under a single "root" key.
 *
 * v1 scope, documented (not silent): NOT real Apple keyed-archiver
 * format (no $class/$objects/$top/CF$UID flat-object-table structure).
 * Supports: (a) root objects that are plain plist-compatible graphs
 * (NSDictionary/NSArray/NSString/NSNumber/NSData/NSDate/NSNull,
 * arbitrarily nested) -- these round-trip via NSPropertyListSerialization
 * directly; (b) custom NSCoding-conforming objects passed directly to
 * -encodeObject:forKey:/decoded via -decodeObjectForKey:, one level of
 * -encodeWithCoder:/-initWithCoder: recursion, tagged with a "$class"
 * marker resolved via objc_getClass() at decode time. NOT supported:
 * object graphs with cycles or shared-object back-references (real
 * NSKeyedArchiver's UID-reference scheme), and custom NSCoding objects
 * nested *inside* an NSArray/NSDictionary that itself gets passed to
 * -encodeObject:forKey: as a whole (only the plist-primitive leaves of
 * such a collection survive) -- pass those objects individually instead.
 */
#ifndef FOUNDATION_NSKEYEDARCHIVER_H
#define FOUNDATION_NSKEYEDARCHIVER_H

#include <Foundation/NSCoder.h>

#ifdef __cplusplus
extern "C" {
#endif

@interface NSKeyedArchiver : NSCoder

+ (NSData *)archivedDataWithRootObject:(id)rootObject;

- (instancetype)init;
- (void)encodeRootObject:(id)rootObject;
- (NSData *)encodedData;

@end

@interface NSKeyedUnarchiver : NSCoder

+ (id)unarchiveObjectWithData:(NSData *)data;

- (instancetype)initForReadingWithData:(NSData *)data;

@end

#ifdef __cplusplus
}
#endif

#endif /* FOUNDATION_NSKEYEDARCHIVER_H */
