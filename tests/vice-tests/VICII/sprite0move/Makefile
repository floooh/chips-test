all: sprite0move.prg 

#sprite0move_ntsc.prg sprite0move_ntscold.prg
	
%.prg: %.asm
	acme -DCYCLES=63 -v1 --cpu 6502 -f cbm -o $@ $<
%_ntsc.prg:%.asm
	acme -DCYCLES=65 -v1 --cpu 6502 -f cbm -o $@ $<
%_ntscold.prg:%.asm
	acme -DCYCLES=64 -v1 --cpu 6502 -f cbm -o $@ $<

clean:
	$(RM) sprite0move.prg sprite0move_ntsc.prg sprite0move_ntscold.prg
