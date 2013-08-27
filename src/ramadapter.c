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
write         = f6 (input)
scan media    = f7 (input)  (active low)
write data    = d5 (input)  (active low)

*/

/*
 If the RAM adaptor is going to attempt writing to the media during the 
transfer, make sure to activate the "-writable media" input, otherwise the 
FDS BIOS will report error #3 (disk write protected). Note that on a real 
disk drive, "-writable media" will active at the same time "-media set" does 
(which is when a non-write protected disk is inserted into the drive). A few 
FDS games rely on this, therefore for writable disks, the "-write enable" 
flag should be activated simultaniously with "-media set".
*/

static rastate_t ra;

void ramadapter_init(void)
{
  DDRF = 0x3E;
  DDRD &= ~0x20;

  //enable pullups
  PORTF |= 0xC1;
  PORTD |= 0x20;

  ramadapter_mediaset(0);
  ramadapter_motoron(1);
  ramadapter_ready(1);
//  ramadapter_rwmedia(1);
}

void ramadapter_mediaset(u8 state)
{
  ra.mediaset = state;
  ramadapter_rwmedia(state); //hax
  if(state == 0)
    PORTF |= 2;
  else
    PORTF &= ~2;
}

void ramadapter_motoron(u8 state)
{
  ra.motoron = state;
  if(state)
    PORTF |= 4;
  else
    PORTF &= ~4;
}

void ramadapter_ready(u8 state)
{
  ra.ready = state;
  if(state == 0)
    PORTF |= 8;
  else
    PORTF &= ~8;
}

void ramadapter_rwmedia(u8 state)
{
  ra.rwmedia = state;
  if(state == 0)
    PORTF |= 0x10;
  else
    PORTF &= ~0x10;
}

u8 ramadapter_stopmotor(void)
{
  return(ra.stopmotor ? 0 : 1);
}

u8 ramadapter_scanmedia(void)
{
  return(ra.scanmedia ? 0 : 1);
}

u8 ramadapter_write(void)
{
  return(ra.write ? 0 : 1);
}

void ramadapter_poll(void)
{
  ra.stopmotor = PINF & 1;
  ra.scanmedia = PINF & 0x80;
  ra.write = PINF & 0x40;
}

rastate_t *ramadapter_getstate(void)
{
  return(&ra);
}
