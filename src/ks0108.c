#include <avr/io.h>
#include <avr/pgmspace.h>
#include <util/delay.h>

/*

pin config:

cs1   = e6
cs2   = e7
d0-d7 = c0-c7
e     = d7
r/w   = e0
d/i   = e1
reset = ??

*/

void ks0108_init()
{
  DDRC = 0xFF;
  DDRD |= 0x80;
  DDRE |= 0xC3;
}

void ks0108_reset()
{
  //drive reset pin low to reset display
  PORTD &= ~0x40;
  _delay_ms(10);
  PORTD |= 0x40;
}
