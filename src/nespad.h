#ifndef __nespad_h__
#define __nespad_h__

#include "types.h"

extern u8 paddata;

void nespad_init(void);
void nespad_poll(void);

#endif
