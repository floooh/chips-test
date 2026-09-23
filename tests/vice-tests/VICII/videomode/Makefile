
TESTS=\
	rmwtest.prg \
	videomode-v.prg \
	videomode-w.prg \
	videomode-x.prg \
	videomode-y.prg \
	videomode-z.prg \
	videomode1.prg \
	videomode2.prg

TESTSNTSC=\
	rmwtest_ntsc.prg \
	videomode-v_ntsc.prg \
	videomode-w_ntsc.prg \
	videomode-x_ntsc.prg \
	videomode-y_ntsc.prg \
	videomode-z_ntsc.prg \
	videomode1_ntsc.prg \
	videomode2_ntsc.prg

TESTSNTSCOLD=\
	rmwtest_ntscold.prg \
	videomode-v_ntscold.prg \
	videomode-w_ntscold.prg \
	videomode-x_ntscold.prg \
	videomode-y_ntscold.prg \
	videomode-z_ntscold.prg \
	videomode1_ntscold.prg \
	videomode2_ntscold.prg

all: $(TESTS) $(TESTSNTSC) $(TESTSNTSCOLD)

%.prg:%.tas
	dasm $*.tas -o$*.prg -DCYCLES=63
%_ntsc.prg:%.tas
	dasm $*.tas -o$*_ntsc.prg -DCYCLES=65
%_ntscold.prg:%.tas
	dasm $*.tas -o$*_ntscold.prg -DCYCLES=64

