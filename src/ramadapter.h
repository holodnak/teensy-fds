
#ifndef __ramadapter_h__
#define __ramadapter_h__

#include "types.h"

typedef struct rastate_s {

  //outputs to ram adapter
  u8 mediaset;
  u8 motoron;
  u8 ready;
  u8 rwmedia;

  //inputs from ramadapter
  u8 stopmotor;
  u8 scanmedia;
  u8 write;
} rastate_t;

void ramadapter_init(void);
void ramadapter_mediaset(u8 state);
void ramadapter_motoron(u8 state);
void ramadapter_ready(u8 state);
void ramadapter_rwmedia(u8 state);
u8 ramadapter_stopmotor(void);
u8 ramadapter_scanmedia(void);
u8 ramadapter_write(void);

void ramadapter_poll(void);

rastate_t *ramadapter_getstate(void);

#endif
