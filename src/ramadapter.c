#include <avr/io.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include "ramadapter.h"

#include "allnight-part1.h"
#include "allnight-part2.h"

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
Rate is the signal that represents the intervals at which data (bits) are 
transfered. A single data bit is transfered on every 0 to 1 (_-) transition 
of this signal. The RAM adaptor expects a transfer rate of 96.4kHz, although 
the tolerance it has for this rate is ñ10%. This tolerance is neccessary 
since, the disk drive can NOT turn the disk at a constant speed. Even though 
it may seem constant, there is a small varying degree in rotation speed, due 
to it's physical architecture.

//16000000 / 96400 = 165.97510373443983402489626556017
//16000000 / 8 / 96400 = 20.746887966804979253112033195021

166
*/

volatile u8 timerint;

ISR(TIMER1_COMPA_vect)
{
  timerint = 1;
}

u16 crc;

//fron nesdev board.  thanks bisquit
static void updatecrc(u8 data)
{
  u8 c;
  int n;

  for(n = 0x01; n <= 0x80; n = n << 1) {
    c = (u8)(crc & 1);
    crc >>= 1;
    if(c)
      crc = crc ^ 0x8408;
    if(data & n)
      crc = crc ^ 0x8000;
  }
}

int diskpos = 0;
u8 diskside = 0;
u8 diskbyte;

static u8 getdiskbyte(void)
{
  int pos = diskpos + 16;
  u8 data;

  if(pos < 32758)
    data = pgm_read_byte(allnight1 + pos);
  else
    data = pgm_read_byte(allnight2 + (pos - 32758));
  return(data);
}

u8 bitssent;
u8 needbyte;
u8 sending;

/*
 If the RAM adaptor is going to attempt writing to the media during the 
transfer, make sure to activate the "-writable media" input, otherwise the 
FDS BIOS will report error #3 (disk write protected). Note that on a real 
disk drive, "-writable media" will active at the same time "-media set" does 
(which is when a non-write protected disk is inserted into the drive). A few 
FDS games rely on this, therefore for writable disks, the "-write enable" 
flag should be activated simultaniously with "-media set".
*/

rastate_t rastate;

void ramadapter_mediaset(u8 state)
{
  rastate.mediaset = state;
  ramadapter_rwmedia(state); //hax
  if(state == 0)
    PORTF |= 2;
  else
    PORTF &= ~2;
}

void ramadapter_motoron(u8 state)
{
  rastate.motoron = state;
  if(state)
    PORTF |= 4;
  else
    PORTF &= ~4;
}

void ramadapter_ready(u8 state)
{
  rastate.ready = state;
  if(state == 0)
    PORTF |= 8;
  else
    PORTF &= ~8;
}

void ramadapter_rwmedia(u8 state)
{
  rastate.rwmedia = state;
  if(state == 0)
    PORTF |= 0x20;
  else
    PORTF &= ~0x20;
}

void ramadapter_outputbit(u8 bit)
{
  if(bit == 0)
    PORTF |= 0x10;
  else
    PORTF &= ~0x10;
}

void ramadapter_poll(void)
{
  rastate.stopmotor = (PINF & 1) ? 0 : 1;
  rastate.scanmedia = (PINF & 0x80) ? 0 : 1;
  rastate.write = (PINF & 0x40) ? 0 : 1;
}

void ramadapter_init(void)
{
  //set port input/outputs
  DDRF = 0x3E;
  DDRD &= ~0x20;

  //enable pullups
  PORTF |= 0xC1;
  PORTD |= 0x20;

  //initialize timer (for sending data)
  TCCR1B |= (1 << WGM12); // Configure timer 1 for CTC mode
  TIMSK1 |= (1 << OCIE1A); // Enable CTC interrupt
  TCNT1   = 166;
//  OCR1A   = 166;
  TCCR1B |= 1; // Start timer at Fcpu

  ramadapter_mediaset(1);
  ramadapter_motoron(0);
  ramadapter_ready(0);
//  ramadapter_rwmedia(1);

  bitssent = 0;
  needbyte = 1;
  diskpos = 0;
  diskside = 0;
  sending = 0;
}

//returns 1 if we are currently sending data
u8 ramadapter_tick(void)
{

  //poll ramadapter inputs
  ramadapter_poll();

  /*- It is okay to tie "-ready" up to the "-scan media" signal, if the media 
  needs no time to prepare for a data xfer after "-scan media" is activated. 
  Don't try to tie "-ready" active all the time- while this will work for 95% 
  of the disk games i've tested, some will not load unless "-ready" is 
  disabled after a xfer. */
//  ramadapter_ready(rastate.scanmedia);
  ramadapter_ready((rastate.scanmedia && rastate.motoron) ? 1 : 0);

  //check if timer interrupt has happened
  if(timerint) {
    if(sending) {
      if(rastate.stopmotor) {
        sending = 0;
        ramadapter_motoron(0);
      }
      return(1);
    }
  }

  //listen to stop motor
  if(rastate.stopmotor == 0) {
    ramadapter_motoron(1);
    if(rastate.scanmedia)
      sending = 1;
  }

/*  if(needbyte) {
    diskbyte = getdiskbyte();
    needbyte = 0;
  }

  if(sending) {
    if((diskbyte >> bitssent) & 1)
      PORTF |= 0x10;
    else
      PORTF &= ~0x10;
    bitssent++;
    if(bitssent == 8) {
      needbyte = 1;
      bitssent = 0;
    }
  }*/

  return(0);
}
