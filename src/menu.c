#include <avr/io.h>
#include <avr/pgmspace.h>
#include <util/delay.h>
#include "menu.h"
#include "types.h"
#include "util.h"
#include "ks0108.h"
#include "nespad.h"
#include "ramadapter.h"

static int selection;

#define T_END   0
#define T_TITLE 1
#define T_ITEM  2

typedef struct menu_s {
  u8 type;
  char text[64];
  void (*handler)(void);
} menu_t;

menu_t *curmenu = 0;

static void entermenu(menu_t *menu)
{
  ks0108_clearscreen(0);
  selection = 0;
  curmenu = menu;
}

static void tick_debugmenu(void)
{

  ks0108_gotoxy(0,8 + 24);
  ks0108_puts("scanmedi");

  ks0108_gotoxy(0,16 + 24);
  ks0108_puts("stopmotr");

  ks0108_gotoxy(0,24 + 24);
  ks0108_puts("write");

  ks0108_gotoxy(63,8 + 24);
  ks0108_puts("motoron");

  ks0108_gotoxy(63,16 + 24);
  ks0108_puts("-mediaset");

  ks0108_gotoxy(63,24 + 24);
  ks0108_puts("-ready");

  ks0108_gotoxy(63,32 + 24);
  ks0108_puts("-rwmedia");

  ks0108_gotoxy(9 * 6 - 2,8 + 24);
  ks0108_putchar(rastate.scanmedia == 0 ? '0' : '1');

  ks0108_gotoxy(9 * 6 - 2,16 + 24);
  ks0108_putchar(rastate.stopmotor == 0 ? '0' : '1');

  ks0108_gotoxy(9 * 6 - 2,24 + 24);
  ks0108_putchar(rastate.write == 0 ? '0' : '1');

  ks0108_gotoxy(63 + 10 * 6 - 2,8 + 24);
  ks0108_putchar(rastate.motoron == 0 ? '0' : '1');

  ks0108_gotoxy(63 + 10 * 6 - 2,16 + 24);
  ks0108_putchar(rastate.mediaset == 0 ? '0' : '1');

  ks0108_gotoxy(63 + 10 * 6 - 2,24 + 24);
  ks0108_putchar(rastate.ready == 0 ? '0' : '1');

  ks0108_gotoxy(63 + 10 * 6 - 2,32 + 24);
  ks0108_putchar(rastate.rwmedia == 0 ? '0' : '1');
}

static void handle_backtomain(void)
{
  menu_init();
}

menu_t debugmenu[] = {
  {T_TITLE, "Debug Menu",   tick_debugmenu},
  {T_ITEM,  "Back to main", handle_backtomain},
  {T_END,   "",             0},
};

static void handle_start(void)
{
  ks0108_clearscreen(0);
  ks0108_gotoxy(0,0);
  ks0108_puts("Running...");
  entermenu(0);
}

static void handle_options(void)
{
  
}

static void handle_dump(void)
{
  
}

static void handle_debug(void)
{
  entermenu((menu_t*)&debugmenu);
}

static void handle_bootloader(void)
{
  bootloader();
}

menu_t rootmenu[] = {
  {T_TITLE, "Main Menu",  0},
  {T_ITEM,  "Start",      handle_start},
  {T_ITEM,  "Options",    handle_options},
  {T_ITEM,  "Dump",       handle_dump},
  {T_ITEM,  "Debug",      handle_debug},
  {T_ITEM,  "Bootloader", handle_bootloader},
  {T_END,   "",           0},
};

void menu_init(void)
{
  selection = 0;
  curmenu = (menu_t*)&rootmenu;
}

static void drawmenu(menu_t *menu)
{
  int i;

  ks0108_gotoxy(8,0);
  ks0108_puts(menu[0].text);
  for(i=1;menu[i].type == T_ITEM;i++) {
    ks0108_gotoxy(0,i * 8 + 8);
    if(selection == (i - 1)) {
      ks0108_invert(1);
      ks0108_puts(menu[i].text);
      ks0108_invert(0);
    }
    else
      ks0108_puts(menu[i].text);
  }
}

static u8 oldpaddata = 0;

void menu_tick(void)
{
  if(curmenu == 0)
    return;

  if(paddata & BTN_A && (oldpaddata & BTN_A) == 0) {
    curmenu[selection + 1].handler();
  }
  if(paddata & BTN_UP && (oldpaddata & BTN_UP) == 0) {
    if(selection && curmenu[selection - 1 + 1].type == T_ITEM)
      selection--;
  }
  if(paddata & BTN_DOWN && (oldpaddata & BTN_DOWN) == 0) {
    if(curmenu[selection + 1 + 1].type == T_ITEM)
      selection++;
  }
  oldpaddata = paddata;

  //draw menu
  if(curmenu) {
    if(curmenu[0].handler)
      curmenu[0].handler();
    drawmenu(curmenu);
  }
}
