COMPILER = acme
CFLAGS = -v1 --cpu 6502 -f cbm -o

.PHONY: all clean

.SUFFIXES: .prg .asm
%.prg: %.asm
	$(COMPILER) $(CFLAGS) $@ $<

all: gfxfetch.prg gfxfetch_ntsc.prg gfxfetch_ntscold.prg

gfxfetch.prg: gfxfetch.asm core.asm
gfxfetch_ntsc.prg: gfxfetch_ntsc.asm core.asm
gfxfetch_ntscold.prg: gfxfetch_ntscold.asm core.asm

clean:
	-rm *.prg
