
NAME := ks0108

HEX := $(NAME).hex
OUT := $(NAME).out
MAP := $(NAME).map

SOURCES := $(wildcard *.cpp)
HEADERS := $(wildcard *.h)
OBJECTS := $(patsubst %.cpp,%.o,$(SOURCES))

MCU := at90usb1286
MCU_AVRDUDE := m168
MCU_FREQ := 8000000UL

CC := avr-gcc
CXX := avr-g++
OBJCOPY := avr-objcopy
SIZE := avr-size -A --format=avr --mcu=$(MCU)

CFLAGS := -Wall -pedantic -mmcu=$(MCU) -g -Os -DF_CPU=$(MCU_FREQ)
CPPFLAGS := -Wall -pedantic -mmcu=$(MCU) -g -Os -DF_CPU=$(MCU_FREQ)

all: $(HEX)

clean:
	rm -f $(HEX) $(OUT) $(MAP) $(OBJECTS)
	rm -rf doc/html

flash: $(HEX)
	avrdude -y -c avr910 -p $(MCU_AVRDUDE) -U flash:w:$(HEX)

$(HEX): $(OUT)
	$(OBJCOPY) -R .eeprom -O ihex $< $@

$(OUT): $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ -Wl,-Map,$(MAP) $^
	@echo
	@$(SIZE) $@
	@echo

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c -o $@ $<

%.pp: %.c
	$(CC) $(CFLAGS) -E -o $@ $<

%.ppo: %.c
	$(CC) $(CFLAGS) -E $<

doc: $(HEADERS) $(SOURCES) Doxyfile
	$(DOXYGEN) Doxyfile

.PHONY: all clean flash doc
