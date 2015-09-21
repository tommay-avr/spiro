# There's a bug in avr-gcc converting attin13a to the startup .o file,
# so use attiny13 instead.

PROG=spiro
SRCS=spiro.c

MCU=attiny13

OPT=-Os
#OPT=-Os -mcall-prologues

CC=avr-gcc
CFLAGS=-mmcu=$(MCU) -std=gnu99 -Wall -g $(OPT)

all: $(PROG).elf $(PROG).lst

$(PROG).elf: $(SRCS:.c=.o)
	$(CC) $(CFLAGS) -o $@ $<
	avr-size $@

%.lst: %.elf
	avr-objdump -h -S $< >$@

%.s: %.c
	$(CC) $(CFLAGS) -S $<

AVRDUDE=avrdude -p $(MCU) -c usbasp-clone

flash: $(PROG).elf
	$(AVRDUDE) -U flash:w:$<:e && \
	$(AVRDUDE) -U lfuse:w:$<:e && \
	$(AVRDUDE) -U hfuse:w:$<:e

clean:
	rm -f *.o *.s *.elf *.lst
