/* Copyright (c) 2026 Vihaan Nathan
 *
 * Toll-free bridged to CFData (userland/CoreFoundation/CFData.h).
 */
#ifndef FOUNDATION_NSDATA_H
#define FOUNDATION_NSDATA_H

#include <Foundation/NSObject.h>

#ifdef __cplusplus
extern "C" {
#endif

@interface NSData : NSObject <NSCopying, NSMutableCopying, NSCoding>

+ (instancetype)data;
+ (instancetype)dataWithBytes:(const void *)bytes length:(NSUInteger)length;
+ (instancetype)dataWithContentsOfFile:(NSString *)path;

- (instancetype)initWithBytes:(const void *)bytes length:(NSUInteger)length;
- (instancetype)initWithContentsOfFile:(NSString *)path;

- (NSUInteger)length;
- (const void *)bytes;
- (void)getBytes:(void *)buffer length:(NSUInteger)length;
- (BOOL)isEqualToData:(NSData *)other;
- (BOOL)writeToFile:(NSString *)path atomically:(BOOL)atomically;

@end

@interface NSMutableData : NSData

+ (instancetype)dataWithCapacity:(NSUInteger)capacity;
- (instancetype)initWithCapacity:(NSUInteger)capacity;

- (void *)mutableBytes;
- (void)appendBytes:(const void *)bytes length:(NSUInteger)length;
- (void)appendData:(NSData *)other;
- (void)setLength:(NSUInteger)length;

@end

#ifdef __cplusplus
}
#endif

#endif /* FOUNDATION_NSDATA_H */
