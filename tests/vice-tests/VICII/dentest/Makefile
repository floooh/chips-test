
TESTS=\
	den01-48-0.prg \
	den01-48-1.prg \
	den01-48-2.prg \
	den01-49-0.prg \
	den01-49-1.prg \
	den01-49-2.prg \
	den10-48-0.prg \
	den10-48-1.prg \
	den10-48-2.prg \
	den10-51-0.prg \
	den10-51-1.prg \
	den10-51-2.prg \
	den10-51-3.prg \
	denrsel-0.prg \
	denrsel-1.prg \
	denrsel-2.prg \
	denrsel-63.prg \
	denrsel-s0.prg \
	denrsel-s1.prg \
	denrsel-s2.prg \
	denrsel55.prg

TESTSNTSC=\
	den01-48-0_ntsc.prg \
	den01-48-1_ntsc.prg \
	den01-48-2_ntsc.prg \
	den01-49-0_ntsc.prg \
	den01-49-1_ntsc.prg \
	den01-49-2_ntsc.prg \
	den10-48-0_ntsc.prg \
	den10-48-1_ntsc.prg \
	den10-48-2_ntsc.prg \
	den10-51-0_ntsc.prg \
	den10-51-1_ntsc.prg \
	den10-51-2_ntsc.prg \
	den10-51-3_ntsc.prg \
	denrsel-0_ntsc.prg \
	denrsel-1_ntsc.prg \
	denrsel-2_ntsc.prg \
	denrsel-63_ntsc.prg \
	denrsel-s0_ntsc.prg \
	denrsel-s1_ntsc.prg \
	denrsel-s2_ntsc.prg \
	denrsel55_ntsc.prg

TESTSNTSCOLD=\
	den01-48-0_ntscold.prg \
	den01-48-1_ntscold.prg \
	den01-48-2_ntscold.prg \
	den01-49-0_ntscold.prg \
	den01-49-1_ntscold.prg \
	den01-49-2_ntscold.prg \
	den10-48-0_ntscold.prg \
	den10-48-1_ntscold.prg \
	den10-48-2_ntscold.prg \
	den10-51-0_ntscold.prg \
	den10-51-1_ntscold.prg \
	den10-51-2_ntscold.prg \
	den10-51-3_ntscold.prg \
	denrsel-0_ntscold.prg \
	denrsel-1_ntscold.prg \
	denrsel-2_ntscold.prg \
	denrsel-63_ntscold.prg \
	denrsel-s0_ntscold.prg \
	denrsel-s1_ntscold.prg \
	denrsel-s2_ntscold.prg \
	denrsel55_ntscold.prg

all: $(TESTS) $(TESTSNTSC) $(TESTSNTSCOLD)

%.prg:%.tas
	dasm $*.tas -o$*.prg -DCYCLES=63
%_ntsc.prg:%.tas
	dasm $*.tas -o$*_ntsc.prg -DCYCLES=65
%_ntscold.prg:%.tas
	dasm $*.tas -o$*_ntscold.prg -DCYCLES=64

