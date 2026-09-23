COMPILER = acme
CFLAGS = -v1 --cpu 6502 -f cbm -o

.PHONY: all clean

.SUFFIXES: .prg .asm
%.prg: %.asm
	$(COMPILER) $(CFLAGS) $@ $<

all: phi1timing.prg phi1timing_ntsc.prg phi1timing_ntscold.prg

phi1timing.prg: phi1timing.asm core.asm
phi1timing_ntsc.prg: phi1timing_ntsc.asm core.asm
phi1timing_ntscold.prg: phi1timing_ntscold.asm core.asm

clean:
	-rm *.prg
