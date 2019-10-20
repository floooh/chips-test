
all: reload0a.prg reload0b.prg

reload0a.prg: reload0a.asm
	acme -f cbm -o reload0a.prg reload0a.asm
reload0b.prg: reload0b.asm
	acme -f cbm -o reload0b.prg reload0b.asm

clean:
	rm -f *.prg
