# There's a bug in avr-gcc converting attin13a to the startup .o file,
# so use attiny13 instead.

PROG=spiro
SRCS=spiro.c

MCU=attiny13

OPT=-Os
#OPT=-Os -mcall-prologues

CC=avr-gcc
#CC=/home/tom/llvm/build/bin/clang --target=avr -D__AVR_ATtiny13__=1 \
#  -I/usr/lib/avr/include \
#  -I/usr/lib/avr/lib

CFLAGS=-mmcu=$(MCU) -std=gnu99 -Wall -g $(OPT)

all: $(PROG).hex $(PROG).lst

$(PROG).elf: $(SRCS:.c=.o)
	$(CC) $(CFLAGS) -o $@ $<
	avr-size $@

%.hex: %.elf
	avr-objcopy -j .text -j .data -O ihex $< $@

%.fuse: %.elf
	avr-objcopy -j .fuse -O binary $< $@

%.lfuse: %.fuse
	dd bs=1 count=1 <$< >$@

%.hfuse: %.fuse
	dd bs=1 count=1 skip=1 <$< >$@

%.lst: %.elf
	avr-objdump -h -S $< >$@

%.s: %.c
	$(CC) $(CFLAGS) -S $<

AVRDUDE=avrdude -p $(MCU) -c usbasp-clone
avr: $(PROG).elf
	$(AVRDUDE) -U flash:w:$<:e && \
	$(AVRDUDE) -U lfuse:w:$<:e && \
	$(AVRDUDE) -U hfuse:w:$<:e

clean:
	rm -f *.o *.s *.elf *.lst *.hex *.*fuse
