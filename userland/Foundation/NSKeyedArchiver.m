/* Copyright (c) 2026 Vihaan Nathan -- see NSKeyedArchiver.h */
#import <Foundation/Foundation.h>
#include <objc/runtime.h>

static BOOL
isPlistPrimitive(id v)
{
	return !v
		|| [v isKindOfClass:[NSString class]]
		|| [v isKindOfClass:[NSNumber class]]
		|| [v isKindOfClass:[NSArray class]]
		|| [v isKindOfClass:[NSDictionary class]]
		|| [v isKindOfClass:[NSData class]]
		|| [v isKindOfClass:[NSDate class]]
		|| [v isKindOfClass:[NSNull class]];
}

@implementation NSKeyedArchiver
{
	@public
	NSMutableDictionary *_dict;
}

+ (NSData *)archivedDataWithRootObject:(id)rootObject
{
	NSKeyedArchiver *a = [[[self alloc] init] autorelease];
	[a encodeRootObject:rootObject];
	return [a encodedData];
}

- (instancetype)init
{
	self = [super init];
	if (self) {
		_dict = [[NSMutableDictionary alloc] init];
	}
	return self;
}

- (void)dealloc
{
	[_dict release];
	[super dealloc];
}

- (void)encodeRootObject:(id)rootObject
{
	[self encodeObject:rootObject forKey:[NSString stringWithUTF8String:"root"]];
}

- (NSData *)encodedData
{
	return [NSPropertyListSerialization dataWithPropertyList:_dict format:NSPropertyListXMLFormat_v1_0 options:0 error:NULL];
}

- (void)encodeObject:(id)object forKey:(NSString *)key
{
	if (isPlistPrimitive(object)) {
		[_dict setObject:(object ? object : (id)[NSNull null]) forKey:key];
		return;
	}
	/* Custom NSCoding object: one level of -encodeWithCoder: recursion,
	 * tagged with $class so -decodeObjectForKey: can reconstruct it.
	 * See this class's header comment for what this does NOT cover
	 * (cycles, shared references, objects nested inside a collection
	 * passed as a whole). */
	NSKeyedArchiver *nested = [[[NSKeyedArchiver alloc] init] autorelease];
	[object encodeWithCoder:nested];
	NSMutableDictionary *wrapper = [NSMutableDictionary dictionaryWithCapacity:0];
	[wrapper setObject:[NSString stringWithUTF8String:class_getName([object class])] forKey:[NSString stringWithUTF8String:"$class"]];
	[wrapper setObject:nested->_dict forKey:[NSString stringWithUTF8String:"$props"]];
	[_dict setObject:wrapper forKey:key];
}

- (void)encodeInt:(int)value forKey:(NSString *)key { [_dict setObject:[NSNumber numberWithInt:value] forKey:key]; }
- (void)encodeInteger:(NSInteger)value forKey:(NSString *)key { [_dict setObject:[NSNumber numberWithInteger:value] forKey:key]; }
- (void)encodeDouble:(double)value forKey:(NSString *)key { [_dict setObject:[NSNumber numberWithDouble:value] forKey:key]; }
- (void)encodeBool:(BOOL)value forKey:(NSString *)key { [_dict setObject:[NSNumber numberWithBool:value] forKey:key]; }

- (BOOL)containsValueForKey:(NSString *)key { return [_dict objectForKey:key] != nil; }

@end

@implementation NSKeyedUnarchiver
{
	@public
	NSDictionary *_dict;
}

+ (id)unarchiveObjectWithData:(NSData *)data
{
	NSKeyedUnarchiver *u = [[[self alloc] initForReadingWithData:data] autorelease];
	return [u decodeObjectForKey:[NSString stringWithUTF8String:"root"]];
}

- (instancetype)initForReadingWithData:(NSData *)data
{
	self = [super init];
	if (self) {
		id plist = [NSPropertyListSerialization propertyListWithData:data options:0 format:NULL error:NULL];
		_dict = [plist retain];
	}
	return self;
}

- (void)dealloc
{
	[_dict release];
	[super dealloc];
}

- (id)decodeObjectForKey:(NSString *)key
{
	id value = [_dict objectForKey:key];
	if ([value isKindOfClass:[NSDictionary class]]) {
		NSString *className = [value objectForKey:[NSString stringWithUTF8String:"$class"]];
		if (className) {
			Class cls = objc_getClass([className UTF8String]);
			if (!cls) {
				return nil;
			}
			NSKeyedUnarchiver *nested = [[[NSKeyedUnarchiver alloc] init] autorelease];
			nested->_dict = [[value objectForKey:[NSString stringWithUTF8String:"$props"]] retain];
			return [[[cls alloc] initWithCoder:nested] autorelease];
		}
	}
	return value;
}

- (int)decodeIntForKey:(NSString *)key { return [[_dict objectForKey:key] intValue]; }
- (NSInteger)decodeIntegerForKey:(NSString *)key { return [[_dict objectForKey:key] integerValue]; }
- (double)decodeDoubleForKey:(NSString *)key { return [[_dict objectForKey:key] doubleValue]; }
- (BOOL)decodeBoolForKey:(NSString *)key { return [[_dict objectForKey:key] boolValue]; }

- (BOOL)containsValueForKey:(NSString *)key { return [_dict objectForKey:key] != nil; }

@end
