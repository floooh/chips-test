
TESTS=\
	d017-54.prg \
	d017-57.prg

TESTSNTSC=\
	d017-54_ntsc.prg \
	d017-57_ntsc.prg

TESTSNTSCOLD=\
	d017-54_ntscold.prg \
	d017-57_ntscold.prg

all: $(TESTS) $(TESTSNTSC) $(TESTSNTSCOLD)

%.prg:%.tas
	dasm $*.tas -o$*.prg -DCYCLES=63
%_ntsc.prg:%.tas
	dasm $*.tas -o$*_ntsc.prg -DCYCLES=65
%_ntscold.prg:%.tas
	dasm $*.tas -o$*_ntscold.prg -DCYCLES=64

