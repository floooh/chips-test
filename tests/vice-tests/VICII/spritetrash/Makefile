
all: spritetrash.prg

spritetrash.prg: spritetrash.asm
	acme -f cbm -o spritetrash.prg spritetrash.asm

test: spritetrash.prg
	x64sc -default -autostartprgmode 1 spritetrash.prg
clean:
	$(RM) spritetrash.prg
