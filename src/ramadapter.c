#include <avr/io.h>
#include <avr/pgmspace.h>
#include <util/delay.h>
#include "ramadapter.h"

/*

pin config:

stop motor    = f0 (input)  (active low)
media set     = f1 (output) (active low)
batt/motor on = f2 (output)
ready         = f3 (output) (active low)
read data     = f4 (output)
rw media      = f5 (output) (active low)
write data    = f6 (input)
scan media    = f7 (input)  (active low)
write gate    = d5 (input)  (active low)

*/

static u8 mediaset;
static u8 motoron;
static u8 ready;
static u8 rwmedia;
static u8 stopmotor;
static u8 scanmedia;
static u8 writedata;

void ramadapter_init(void)
{
  DDRF = 0x3E;
  DDRD &= ~0x20;

  ramadapter_mediaset(1);
  ramadapter_motoron(0);
  ramadapter_ready(1);
  ramadapter_rwmedia(1);
}

void ramadapter_mediaset(u8 state)
{
  mediaset = state;
  if(mediaset == 0)
    PORTF |= 2;
  else
    PORTF &= ~2;
}

void ramadapter_motoron(u8 state)
{
  motoron = state;
  if(motoron)
    PORTF |= 4;
  else
    PORTF &= ~4;
}

void ramadapter_ready(u8 state)
{
  ready = state;
  if(ready == 0)
    PORTF |= 8;
  else
    PORTF &= ~8;
}

void ramadapter_rwmedia(u8 state)
{
  rwmedia = state;
  if(rwmedia == 0)
    PORTF |= 0x10;
  else
    PORTF &= ~0x10;
}

u8 ramadapter_stopmotor(void)
{
  stopmotor = PINF & 1;
  return(stopmotor ? 0 : 1);
}

u8 ramadapter_scanmedia(void)
{
  scanmedia = PINF & 0x80;
  return(scanmedia ? 0 : 1);
}
