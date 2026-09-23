all: screenpos.prg screenpos_ntsc.prg screenpos_ntscold.prg
	
%.prg: %.asm
	acme -DCYCLES=63 -v1 --cpu 6502 -f cbm -o $@ $<
%_ntsc.prg:%.asm
	acme -DCYCLES=65 -v1 --cpu 6502 -f cbm -o $@ $<
%_ntscold.prg:%.asm
	acme -DCYCLES=64 -v1 --cpu 6502 -f cbm -o $@ $<
	