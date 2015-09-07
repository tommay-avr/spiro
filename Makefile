# There's a bug in avr-gcc converting attin13a to the startup .o file,
# so use attiny13 instead.

PROG=spiro
SRCS=spiro.c

CC=avr-gcc
#CFLAGS=-mmcu=attiny13 -std=gnu99 -Os -mcall-prologues -Wall -g
CFLAGS=-mmcu=attiny13 -std=gnu99 -O2 -Wall -g

all: $(PROG).hex $(PROG).lst

%.s: %.c
	$(CC) $(CFLAGS) -S $<

$(PROG).elf: $(SRCS:.c=.o)
	$(CC) $(CFLAGS) -o $@ $<

%.hex: %.elf
	avr-objcopy -j .text -j .data -O ihex $< $@

%.lst: %.elf
	avr-objdump -h -S $< >$@

avrdude:
	avrdude -c ... -p attiny13

clean:
	rm -f *.o *.s *.elf *.lst *.hex
