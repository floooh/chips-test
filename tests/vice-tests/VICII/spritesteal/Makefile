
.PHONY: all clean

all: spritesteal.prg spritesteal_ntsc.prg spritesteal_ntscold.prg

spritesteal.prg: core.asm
	acme -f cbm -DCYCLES=63 -o spritesteal.prg core.asm
spritesteal_ntsc.prg: core.asm
	acme -f cbm -DCYCLES=65 -o spritesteal_ntsc.prg core.asm
spritesteal_ntscold.prg: core.asm
	acme -f cbm -DCYCLES=64 -o spritesteal_ntscold.prg core.asm
clean:
	-rm *.prg
