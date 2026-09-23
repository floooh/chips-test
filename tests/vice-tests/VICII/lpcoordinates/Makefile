COMPILER = acme
CFLAGS = -v1 --cpu 6502 -f cbm -o

.PHONY: all clean

.SUFFIXES: .prg .asm
%.prg: %.asm
	$(COMPILER) $(CFLAGS) $@ $<

all: lpcoords.prg

lpcoords.prg: lpcoords.asm

clean:
	-rm *.prg
	-rm *~
