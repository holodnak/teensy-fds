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

volatile struct {
  uint8_t tmr_int: 1;
  uint8_t adc_int: 1;
  uint8_t rx_int: 1;
} intflags;

static u8 sending = 0;

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


ISR(TIMER1_COMPA_vect)
{
  intflags.tmr_int = 1;
  PORTD ^= (1 << 6); // Toggle the LED
} 

void init(void)
{
  //initialize clock speed and interrupts
  CPU_PRESCALE(CPU_16MHz);
  cli();

  //initialize subsystems
  ks0108_init(0);
  usb_init();
  ramadapter_init();
  nespad_init();

  DDRD |= (1 << 6);

  //initialize timer
  TCCR1B |= (1 << WGM12); // Configure timer 1 for CTC mode
  TIMSK1 |= (1 << OCIE1A); // Enable CTC interrupt 
  OCR1A   = 166; // Set CTC compare value to 1Hz at 1MHz AVR clock, with a prescaler of 64
  TCCR1B |= 1; // Start timer at Fcpu

}

void bootloader(void)
{
  cli();
  // disable watchdog, if enabled
  // disable all peripherals
  UDCON = 1;
  USBCON = (1<<FRZCLK);  // disable USB
  UCSR1B = 0;
  _delay_ms(5);
#if defined(__AVR_AT90USB162__)                // Teensy 1.0
  EIMSK = 0; PCICR = 0; SPCR = 0; ACSR = 0; EECR = 0;
  TIMSK0 = 0; TIMSK1 = 0; UCSR1B = 0;
  DDRB = 0; DDRC = 0; DDRD = 0;
  PORTB = 0; PORTC = 0; PORTD = 0;
  asm volatile("jmp 0x3E00");
#elif defined(__AVR_ATmega32U4__)              // Teensy 2.0
  EIMSK = 0; PCICR = 0; SPCR = 0; ACSR = 0; EECR = 0; ADCSRA = 0;
  TIMSK0 = 0; TIMSK1 = 0; TIMSK3 = 0; TIMSK4 = 0; UCSR1B = 0; TWCR = 0;
  DDRB = 0; DDRC = 0; DDRD = 0; DDRE = 0; DDRF = 0; TWCR = 0;
  PORTB = 0; PORTC = 0; PORTD = 0; PORTE = 0; PORTF = 0;
  asm volatile("jmp 0x7E00");
#elif defined(__AVR_AT90USB646__)              // Teensy++ 1.0
  EIMSK = 0; PCICR = 0; SPCR = 0; ACSR = 0; EECR = 0; ADCSRA = 0;
  TIMSK0 = 0; TIMSK1 = 0; TIMSK2 = 0; TIMSK3 = 0; UCSR1B = 0; TWCR = 0;
  DDRA = 0; DDRB = 0; DDRC = 0; DDRD = 0; DDRE = 0; DDRF = 0;
  PORTA = 0; PORTB = 0; PORTC = 0; PORTD = 0; PORTE = 0; PORTF = 0;
  asm volatile("jmp 0xFC00");
#elif defined(__AVR_AT90USB1286__)             // Teensy++ 2.0
  EIMSK = 0; PCICR = 0; SPCR = 0; ACSR = 0; EECR = 0; ADCSRA = 0;
  TIMSK0 = 0; TIMSK1 = 0; TIMSK2 = 0; TIMSK3 = 0; UCSR1B = 0; TWCR = 0;
  DDRA = 0; DDRB = 0; DDRC = 0; DDRD = 0; DDRE = 0; DDRF = 0;
  PORTA = 0; PORTB = 0; PORTC = 0; PORTD = 0; PORTE = 0; PORTF = 0;
  asm volatile("jmp 0x1FC00");
#else
  #error unknown teensy board type
#endif 
}

int main(void)
{
  u8 data;
  rastate_t *ra;

  init();

  ks0108_clearscreen(0);
  ks0108_selectfont(System5x7,0);
  ks0108_gotoxy(8,0);
  ks0108_puts("inputs");
  ks0108_gotoxy(64 + 8,0);
  ks0108_puts("outputs");

  ks0108_gotoxy(0,8);
  ks0108_puts("scanmedi");

  ks0108_gotoxy(0,16);
  ks0108_puts("stopmotr");

  ks0108_gotoxy(0,24);
  ks0108_puts("write");

  ks0108_gotoxy(63,8);
  ks0108_puts("motoron");

  ks0108_gotoxy(63,16);
  ks0108_puts("-mediaset");

  ks0108_gotoxy(63,24);
  ks0108_puts("-ready");

  ks0108_gotoxy(63,32);
  ks0108_puts("-rwmedia");

  sei();

  ra = ramadapter_getstate();

  //give good battery status
  ramadapter_motoron(1);
  ramadapter_mediaset(1);
  ramadapter_ready(0);

  for(;;) {

    //send data
    if(intflags.tmr_int && sending) {

    }

    ramadapter_poll();

    if(ra->scanmedia) {

    }


    ks0108_gotoxy(9 * 6 - 2,8);
    ks0108_putchar(ramadapter_scanmedia() == 0 ? '0' : '1');

    ks0108_gotoxy(9 * 6 - 2,16);
    ks0108_putchar(ramadapter_stopmotor() == 0 ? '0' : '1');

    ks0108_gotoxy(9 * 6 - 2,24);
    ks0108_putchar(ramadapter_write() == 0 ? '0' : '1');

    ks0108_gotoxy(63 + 10 * 6 - 2,8);
    ks0108_putchar(ra->motoron == 0 ? '0' : '1');

    ks0108_gotoxy(63 + 10 * 6 - 2,16);
    ks0108_putchar(ra->mediaset == 0 ? '0' : '1');

    ks0108_gotoxy(63 + 10 * 6 - 2,24);
    ks0108_putchar(ra->ready == 0 ? '0' : '1');

    ks0108_gotoxy(63 + 10 * 6 - 2,32);
    ks0108_putchar(ra->rwmedia == 0 ? '0' : '1');

    nespad_poll();
    data = paddata;

    ks0108_gotoxy(63,48);
    ks0108_putchar(data & 0x80 ? '1' : '0');
    ks0108_putchar(data & 0x40 ? '1' : '0');
    ks0108_putchar(data & 0x20 ? '1' : '0');
    ks0108_putchar(data & 0x10 ? '1' : '0');
    ks0108_putchar(data & 0x08 ? '1' : '0');
    ks0108_putchar(data & 0x04 ? '1' : '0');
    ks0108_putchar(data & 0x02 ? '1' : '0');
    ks0108_putchar(data & 0x01 ? '1' : '0');

    if(data & 0x10) {
      ks0108_gotoxy(0,56);
      ks0108_puts("Entering bootloader...");
      bootloader();
    }

    data = pgm_read_byte(allnight1);
    data = pgm_read_byte(allnight2);
  }
}
