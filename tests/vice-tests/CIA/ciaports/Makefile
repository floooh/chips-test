COMPILER = acme
CFLAGS = -v1 --cpu 6502 -f cbm -o

.PHONY: all clean

.SUFFIXES: .prg .asm
%.prg: %.asm
	$(COMPILER) $(CFLAGS) $@ $<

all: ciaports.prg ghosting.prg

ciaports.prg: ciaports.asm
ghosting.prg: ghosting.asm

clean:
	-rm *.prg
	-rm *~
