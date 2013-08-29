#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <avr/sleep.h>
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
#include "../lib/sd-reader/fat.h"
#include "../lib/sd-reader/fat_config.h"
#include "../lib/sd-reader/partition.h"
#include "../lib/sd-reader/sd_raw.h"
#include "../lib/sd-reader/sd_raw_config.h"

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

struct partition_struct *partition;
struct fat_fs_struct *fs;
struct fat_dir_entry_struct directory;
struct fat_dir_struct *dd;

void init(void)
{
  //led output
  DDRD |= (1 << 6);

  //power reduction register
  PRR0 = 0;

  //initialize subsystems
  ks0108_init(0);
  ks0108_clearscreen(0);
  ks0108_selectfont(System5x7,0);
  ks0108_gotoxy(0,0);
  usb_init();
  ramadapter_init();
  nespad_init();
  menu_init();

  if(sd_raw_init() == 0) {
    ks0108_gotoxy(0,56);
    ks0108_puts("MMC/SD init failed");
    _delay_ms(500);
    return;
  }

  //open partition
  partition = partition_open(sd_raw_read,sd_raw_read_interval,sd_raw_write,sd_raw_write_interval,0);
  if(partition == 0) {
    //if it failed try in no-mbr mode
    partition = partition_open(sd_raw_read,sd_raw_read_interval,sd_raw_write,sd_raw_write_interval,-1);
    if(!partition) {
      ks0108_gotoxy(0,56);
      ks0108_puts("opening partition failed");
      _delay_ms(500);
      return;
    }
  }

  //open the fat filesystem
  if((fs = fat_open(partition)) == 0) {
    ks0108_gotoxy(0,56);
    ks0108_puts("opening filesystem failed");
    _delay_ms(500);
    return;
  }

  fat_get_dir_entry_of_path(fs,"/",&directory);
  dd = fat_open_dir(fs, &directory);
  if(dd == 0) {
    ks0108_gotoxy(0,56);
    ks0108_puts("opening root directory failed");
    _delay_ms(500);
    return;
  }

  ks0108_gotoxy(0,56);
  ks0108_puts("sd card init ok");
  _delay_ms(500);
}

int main(void)
{
  //initialize clock speed and interrupts
  CPU_PRESCALE(CPU_16MHz);
  set_sleep_mode(SLEEP_MODE_IDLE);

  cli();
  sei();

  //initialize everything
  init();

  //clear screen
  ks0108_clearscreen(0);

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
