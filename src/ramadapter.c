#include <avr/io.h>
#include <avr/pgmspace.h>
#include <util/delay.h>

/*

pin config:

stop motor    = f0
media set     = f1
battery good  = f2
ready         = f3
read data     = f4
rw media      = f5
write data    = f6
scan media    = f7
write gate    = d5

*/

void ramadapter_init()
{
  DDRF = 0xFF;
  DDRD |= 0x20;
}
