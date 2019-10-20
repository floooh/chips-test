
all: cmp-b-counts-a-old.prg cmp-b-counts-a-new.prg \
	cmp-b-counts-a-old_ntsc.prg cmp-b-counts-a-new_ntsc.prg \
	cia-b-counts-a.prg cia-b-counts-a_ntsc.prg

cmp-b-counts-a-old.prg: cmp-b-counts-a-old.asm cmp-b-counts-a.asm
	acme -DCYCLES=63 -v1 --cpu 6502 -f cbm -o cmp-b-counts-a-old.prg cmp-b-counts-a-old.asm
cmp-b-counts-a-new.prg: cmp-b-counts-a-new.asm cmp-b-counts-a.asm
	acme -DCYCLES=63 -v1 --cpu 6502 -f cbm -o cmp-b-counts-a-new.prg cmp-b-counts-a-new.asm

cmp-b-counts-a-old_ntsc.prg: cmp-b-counts-a-old.asm cmp-b-counts-a.asm
	acme -DCYCLES=65 -v1 --cpu 6502 -f cbm -o cmp-b-counts-a-old_ntsc.prg cmp-b-counts-a-old.asm
cmp-b-counts-a-new_ntsc.prg: cmp-b-counts-a-new.asm cmp-b-counts-a.asm
	acme -DCYCLES=65 -v1 --cpu 6502 -f cbm -o cmp-b-counts-a-new_ntsc.prg cmp-b-counts-a-new.asm

cia-b-counts-a.prg: cia-b-counts-a.asm
	acme -DCYCLES=63 -v1 --cpu 6502 -f cbm -o cia-b-counts-a.prg cia-b-counts-a.asm
cia-b-counts-a_ntsc.prg: cia-b-counts-a.asm
	acme -DCYCLES=65 -v1 --cpu 6502 -f cbm -o cia-b-counts-a_ntsc.prg cia-b-counts-a.asm

clean:
	rm -f *~
	rm -f *.prg
