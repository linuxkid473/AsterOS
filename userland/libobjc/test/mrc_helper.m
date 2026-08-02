/* Copyright (c) 2026 Vihaan Nathan
 *
 * A tiny non-ARC helper, compiled as its own translation unit (see
 * build.sh: no -fobjc-arc here). ARC only auto-inserts retain/release
 * bracketing around known Objective-C conventions -- message sends and
 * a specific set of runtime function names it recognizes by name (like
 * objc_autorelease itself, ground-truthed the hard way, see TODO.md
 * Phase 13). An ordinary C function from another translation unit gets
 * no such treatment, so calling -autorelease through here instead of
 * from test.m directly avoids the fragile ARC/pool interaction entirely
 * rather than trying to out-guess clang's cooperative fast-path
 * protocol for a single edge case.
 */
#import <objc/objc.h>
#import <objc/runtime.h>
#import <objc/message.h>

id
test_mrc_autorelease(id obj)
{
	return objc_msgSend(obj, sel_registerName("autorelease"));
}
