#include <avr/io.h>
#include <avr/pgmspace.h>
#include <util/delay.h>
#include "usb_debug_only.h"
#include "print.h"
#include "ks0108.h"
#include "ramadapter.h"
#include "SystemFont5x7.h"

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

int main(void)
{
  CPU_PRESCALE(CPU_8MHz);

  usb_init();

  ramadapter_init();
  ks0108_init(0);
  ks0108_clearscreen(0);
  ks0108_selectfont(System5x7,0);

  for(;;) {

    ks0108_gotoxy(0,8);
    if(ramadapter_scanmedia() == 0)
      ks0108_puts("scanmedia: no \n");
    else
      ks0108_puts("scanmedia: yes\n");
    ks0108_gotoxy(0,32);
    if(ramadapter_stopmotor() == 0)
      ks0108_puts("stopmotor: no \n");
    else
      ks0108_puts("stopmotor: yes\n");
  }
}
