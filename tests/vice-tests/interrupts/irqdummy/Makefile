COMPILER = acme
CFLAGS = -v1 --cpu 6502 -f cbm -o

.PHONY: all clean

all: irqdummy.prg

irqdummy.prg: irqdummy.asm
	$(COMPILER) $(CFLAGS) $@ $<

clean:
	-rm irqdummy.prg
