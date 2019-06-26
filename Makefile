TARGET=hg_node_basic
MCU=attiny88
F_CPU=1000000
SOURCES=hg_node_basic.c tm1637.c timer.c button.c pump.c buzzer.c display.c power.c data.c twimaster.c temp.c

PROGRAMMER=arduino
#auskommentieren f�r automatische Wahl
PORT=-P/dev/ttyS0
BAUD=-B115200

#Ab hier nichts ver�ndern
OBJECTS=$(SOURCES:.c=.o)
CFLAGS=-c -Os
LDFLAGS=

all: hex eeprom

hex: $(TARGET).hex size

eeprom: $(TARGET)_eeprom.hex

$(TARGET).hex: $(TARGET).elf
	avr-objcopy -O ihex -j .data -j .text $(TARGET).elf $(TARGET).hex

$(TARGET)_eeprom.hex: $(TARGET).elf
	avr-objcopy -O ihex -j .eeprom --change-section-lma .eeprom=1 $(TARGET).elf $(TARGET)_eeprom.hex

$(TARGET).elf: $(OBJECTS)
	avr-gcc $(LDFLAGS) -mmcu=$(MCU) $(OBJECTS) -o $(TARGET).elf

.c.o:
	avr-gcc $(CFLAGS) -mmcu=$(MCU) $< -o $@

size:
	avr-size --mcu=attiny88 -C $(TARGET).elf

program:
	avrdude -p$(MCU) $(PORT) $(BAUD) -c$(PROGRAMMER) -Uflash:w:$(TARGET).hex:a

clean_tmp:
	rm -rf *.o
	rm -rf *.elf

clean:
	rm -rf *.o
	rm -rf *.elf
	rm -rf *.hex