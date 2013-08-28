#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <util/delay.h>
#include "types.h"
#include "usb_debug_only.h"
#include "print.h"
#include "ks0108.h"
#include "ramadapter.h"
#include "nespad.h"
#include "SystemFont5x7.h"
#include "util.h"
#include "menu.h"

#include "allnight-part1.h"
#include "allnight-part2.h"

#define CPU_PRESCALE(n) (CLKPR = 0x80, CLKPR = (n))
#define CPU_16MHz       0x00
#define CPU_8MHz        0x01
#define CPU_4MHz        0x02
#define CPU_2MHz        0x03
#define CPU_1MHz        0x04
#define CPU_500kHz      0x05
#define CPU_250kHz      0x06
#define CPU_125kHz      0x07
#define CPU_62kHz       0x08

void init(void)
{
  //initialize clock speed and interrupts
  CPU_PRESCALE(CPU_16MHz);
  cli();

  //initialize subsystems
  ks0108_init(0);
  ks0108_clearscreen(0);
  ks0108_selectfont(System5x7,0);
  usb_init();
  ramadapter_init();
  nespad_init();
  menu_init();

  //led output
  DDRD |= (1 << 6);
}

int main(void)
{
  //initialize everything
  init();

  //enable interrupts
  sei();

  //the main loop
  for(;;) {

    //toggle the led
    PORTD ^= (1 << 6);

    //process the ram adapter
    if(ramadapter_tick() != 0)
      continue;

    //needs to be polled 60 times a second
    nespad_poll();

    //check for jumping to the bootloader
    if(paddata & BTN_START) {
      bootloader();
    }

    //update the menu system
    menu_tick();
  }
}
