/* Copyright (c) 2026 Vihaan Nathan
 *
 * Real recursive-descent JSON parser/writer over
 * NSDictionary/NSArray/NSString/NSNumber/NSNull object graphs. v1 scope
 * cut, documented: NSJSONWritingOptions/NSJSONReadingOptions are
 * accepted but ignored (no pretty-printing, no fragments-allowed mode --
 * top-level value must be an object or array, matching strict JSON and
 * pre-10.15 NSJSONSerialization behavior).
 */
#ifndef FOUNDATION_NSJSONSERIALIZATION_H
#define FOUNDATION_NSJSONSERIALIZATION_H

#include <Foundation/NSObject.h>

#ifdef __cplusplus
extern "C" {
#endif

@class NSData;
@class NSError;

typedef NSUInteger NSJSONWritingOptions;
typedef NSUInteger NSJSONReadingOptions;

@interface NSJSONSerialization : NSObject

+ (BOOL)isValidJSONObject:(id)obj;
+ (NSData *)dataWithJSONObject:(id)obj options:(NSJSONWritingOptions)opt error:(NSError **)error;
+ (id)JSONObjectWithData:(NSData *)data options:(NSJSONReadingOptions)opt error:(NSError **)error;

@end

#ifdef __cplusplus
}
#endif

#endif /* FOUNDATION_NSJSONSERIALIZATION_H */
