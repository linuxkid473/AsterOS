/* Copyright (c) 2026 Vihaan Nathan */
#ifndef _OBJC_MESSAGE_H_
#define _OBJC_MESSAGE_H_

#include <objc/objc.h>

struct objc_super {
	id receiver;
	Class super_class;
};

id objc_msgSend(id self, SEL op, ...);
id objc_msgSendSuper(struct objc_super *super, SEL op, ...);

/* What [super ...] actually compiles to under the modern ABI -- see
 * userland/libobjc/msgSend.S for why the second field means something
 * different here than in objc_msgSendSuper's struct objc_super. Same
 * physical layout (id + Class), so no separate struct type is needed. */
id objc_msgSendSuper2(struct objc_super *super, SEL op, ...);

#endif /* _OBJC_MESSAGE_H_ */
