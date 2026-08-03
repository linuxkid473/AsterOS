/* Copyright (c) 2026 Vihaan Nathan -- see NSUserDefaults.h */
#import <Foundation/Foundation.h>

@implementation NSUserDefaults
{
	NSMutableDictionary *_dict;
	NSString *_path;
}

+ (instancetype)standardUserDefaults
{
	static NSUserDefaults *shared;
	if (!shared) {
		shared = [[self alloc] initWithSuiteName:[[NSProcessInfo processInfo] processName]];
	}
	return shared;
}

- (instancetype)initWithSuiteName:(NSString *)suiteName
{
	self = [super init];
	if (self) {
		_path = [[NSString stringWithFormat:[NSString stringWithUTF8String:"/tmp/%@.plist"], suiteName] retain];
		NSData *existing = [NSData dataWithContentsOfFile:_path];
		id plist = existing ? [NSPropertyListSerialization propertyListWithData:existing options:0 format:NULL error:NULL] : nil;
		_dict = plist ? [plist mutableCopy] : [[NSMutableDictionary alloc] init];
	}
	return self;
}

- (void)dealloc
{
	[_dict release];
	[_path release];
	[super dealloc];
}

- (id)objectForKey:(NSString *)key
{
	return [_dict objectForKey:key];
}

- (void)setObject:(id)value forKey:(NSString *)key
{
	if (value) {
		[_dict setObject:value forKey:key];
	} else {
		[_dict removeObjectForKey:key];
	}
}

- (void)removeObjectForKey:(NSString *)key
{
	[_dict removeObjectForKey:key];
}

- (NSString *)stringForKey:(NSString *)key
{
	id v = [_dict objectForKey:key];
	return [v isKindOfClass:[NSString class]] ? v : nil;
}

- (NSInteger)integerForKey:(NSString *)key { return [[_dict objectForKey:key] integerValue]; }
- (double)doubleForKey:(NSString *)key { return [[_dict objectForKey:key] doubleValue]; }
- (BOOL)boolForKey:(NSString *)key { return [[_dict objectForKey:key] boolValue]; }

- (NSArray *)arrayForKey:(NSString *)key
{
	id v = [_dict objectForKey:key];
	return [v isKindOfClass:[NSArray class]] ? v : nil;
}

- (NSDictionary *)dictionaryForKey:(NSString *)key
{
	id v = [_dict objectForKey:key];
	return [v isKindOfClass:[NSDictionary class]] ? v : nil;
}

- (void)setInteger:(NSInteger)value forKey:(NSString *)key { [_dict setObject:[NSNumber numberWithInteger:value] forKey:key]; }
- (void)setDouble:(double)value forKey:(NSString *)key { [_dict setObject:[NSNumber numberWithDouble:value] forKey:key]; }
- (void)setBool:(BOOL)value forKey:(NSString *)key { [_dict setObject:[NSNumber numberWithBool:value] forKey:key]; }

- (BOOL)synchronize
{
	NSData *data = [NSPropertyListSerialization dataWithPropertyList:_dict format:NSPropertyListXMLFormat_v1_0 options:0 error:NULL];
	if (!data) {
		return NO;
	}
	return [data writeToFile:_path atomically:NO];
}

@end
