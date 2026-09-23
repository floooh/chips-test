
all: testsuite.prg testsuite_ntsc.prg testsuite_ntscold.prg \
	demusinterruptus.prg
	
%.prg: %.tas
	dasm $< -o$@ -DCYCLES=63
%_ntsc.prg:%.tas
	dasm $*.tas -o$*_ntsc.prg -DCYCLES=65
%_ntscold.prg:%.tas
	dasm $*.tas -o$*_ntscold.prg -DCYCLES=64
	
demusinterruptus.prg: demusinterruptus.asm
	acme -f cbm -o demusinterruptus.prg demusinterruptus.asm
