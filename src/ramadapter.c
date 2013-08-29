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

16000000 / 96400 = 165.97510373443983402489626556017
16000000 / 166 = 96385.542168674698795180722891566

166 is close enough
166 / 2 = 83 for clock up -> down transitioning
*/

volatile u8 toggle;
volatile u8 sending;

//buffer for storing disk data read off of the disk
volatile u8 bufferpos,curbuffer;
volatile u8 buffer[2][256];

//total size of the current block we are sending (includes start byte and crc)
volatile int blocksize;

//for when disk is being read from a gap period
volatile u16 gapperiod;

//the current bit/byte being output
volatile u8 outbit,outbyte;

//number of bits from the current byte being output
volatile u8 bitssent;

//for requesting a buffer be filled (0 = do nothing, 1 = fill buffer 1, 2 = fill buffer 2)
volatile u8 fillbuffer;

//this flag is set when we are transferring to/from the ram adapter
volatile u8 transfer;

//timer interrupt for sending data out to the ram adapter
ISR(TIMER1_COMPA_vect)
{
  //if we are not transferring, return
  if(transfer == 0)
    return;

  //toggle the phony clock
  toggle ^= 1;

  //if this is gap period just keep toggling with the rate
  if(gapperiod) {
    PORTF ^= 0x10;
    return;
  }

  //clock is low, beginning of the clock
  if(toggle == 0) {

    //output byte to pin
    if((outbyte >> bitssent) & 1) {
      PORTF |= 0x10;
    }
    else {
      PORTF &= ~0x10;
    }

    //increment bit counter
    bitssent++;

    //see if we have sent the entire byte
    if(bitssent == 8) {
      bitssent = 0;

      //get next byte from the buffer
      outbyte = buffer[curbuffer][bufferpos++];

      //if that was the last byte, request buffer be filled and switch buffers
      if(bufferpos == 0) {
        fillbuffer = curbuffer;
        curbuffer ^= 1;
      }
    }
  }

  //clock going high
  else {
    PORTF ^= 0x10;
  }
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

//current position on the disk
int diskpos = 0;

//current disk side
u8 diskside = 0;

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
//  TCNT1   = 83;
  OCR1A   = 83;
  TCCR1B |= 1; // Start timer at Fcpu

  ramadapter_mediaset(1);
  ramadapter_motoron(0);
  ramadapter_ready(0);
//  ramadapter_rwmedia(1);

  diskpos = 0;
  diskside = 0;
  transfer = 0;
  bitssent = 0;
  toggle = 0;
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

  //see if we need more data
  if(fillbuffer) {
    //fill buffer here!
    fillbuffer = 0;
  }

  //check if timer interrupt has happened
/*  if(timerint) {

    //see if we are sending data
    if(sending) {

      //listen to 'stopmotor' command
      if(rastate.stopmotor) {
        sending = 0;
        ramadapter_motoron(0);
      }

      //output data
      outbit = (diskbyte >> bitssent) & 1;
      if(outbit) {
        PORTF |= 0x10;
      }
      else {
        PORTF &= ~0x10;
      }

      //increment sent bits count and check if we need more data
      bitssent++;
      if(bitssent == 8) {
        needbyte = 1;
        bitssent = 0;
      }
    }
  }*/

  //if we are sending/recieving data, tell main loop to not do anything!
  if(transfer) {

    //listen to 'stopmotor' command
    if(rastate.stopmotor) {
      transfer = 0;
      ramadapter_motoron(0);
    }

    return(1);
  }

  //if the motor is running...
  if(rastate.stopmotor == 0) {
    ramadapter_motoron(1);

    //...and ram adapter set scanmedia, we must be transferring data
    if(rastate.scanmedia) {

      //set transfer flag
      transfer = 1;

      //set gap period delay
      gapperiod = 14000;

      //fill both of the buffers now!
      fillbuffer = 1 | 2;
      //fill them here!

      //setup data transferring variables
      curbuffer = 0;
      bufferpos = 0;

      //prime outbyte with data
      outbyte = buffer[curbuffer][bufferpos++];

      //tell main loop we need all cpu we can get
      return(1);
    }
  }

  return(0);
}
