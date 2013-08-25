
#ifndef __ramadapter_h__
#define __ramadapter_h__

#include "types.h"

void ramadapter_init(void);
void ramadapter_setbatterystate(u8 state);
void ramadapter_mediaset(u8 state);
void ramadapter_motoron(u8 state);
void ramadapter_ready(u8 state);
void ramadapter_rwmedia(u8 state);
u8 ramadapter_stopmotor();
u8 ramadapter_scanmedia();

#endif
