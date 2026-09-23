
TESTS=\
	sprite-gfx-collision-cycle.prg \
	sprite-sprite-collision-cycle.prg \
	sprite-sprite.prg \
	sprite-sprite-mc-mc.prg \
	sprite-sprite-mc-hi.prg \
	sprite-sprite-hi-mc.prg \
	sprite-sprite-hi-hi.prg \
	sprite-gfx-mc-mc.prg \
	sprite-gfx-hi-mc.prg \
	sprite-gfx-mc-hi.prg \
	sprite-gfx-hi-hi.prg

TESTSNTSC=\
	sprite-gfx-collision-cycle_ntsc.prg \
	sprite-sprite-collision-cycle_ntsc.prg \
	sprite-sprite_ntsc.prg

TESTSNTSCOLD=\
	sprite-gfx-collision-cycle_ntscold.prg \
	sprite-sprite-collision-cycle_ntscold.prg \
	sprite-sprite_ntscold.prg

all: $(TESTS) 

# FIXME: NTSC tests are not working
#$(TESTSNTSC) $(TESTSNTSCOLD)

sprite-sprite-mc-mc.prg: sprite-sprite-collision.tas
	dasm sprite-sprite-collision.tas -osprite-sprite-mc-mc.prg -DSPRMUCO=3
sprite-sprite-mc-hi.prg: sprite-sprite-collision.tas
	dasm sprite-sprite-collision.tas -osprite-sprite-mc-hi.prg -DSPRMUCO=2
sprite-sprite-hi-mc.prg: sprite-sprite-collision.tas
	dasm sprite-sprite-collision.tas -osprite-sprite-hi-mc.prg -DSPRMUCO=1
sprite-sprite-hi-hi.prg: sprite-sprite-collision.tas
	dasm sprite-sprite-collision.tas -osprite-sprite-hi-hi.prg -DSPRMUCO=0

sprite-gfx-mc-mc.prg: sprite-gfx-collision.tas
	dasm sprite-gfx-collision.tas -osprite-gfx-mc-mc.prg -DSPRMUCO=1 -DCHRMUCO=1
sprite-gfx-mc-hi.prg: sprite-gfx-collision.tas
	dasm sprite-gfx-collision.tas -osprite-gfx-mc-hi.prg -DSPRMUCO=1 -DCHRMUCO=0
sprite-gfx-hi-mc.prg: sprite-gfx-collision.tas
	dasm sprite-gfx-collision.tas -osprite-gfx-hi-mc.prg -DSPRMUCO=0 -DCHRMUCO=1
sprite-gfx-hi-hi.prg: sprite-gfx-collision.tas
	dasm sprite-gfx-collision.tas -osprite-gfx-hi-hi.prg -DSPRMUCO=0 -DCHRMUCO=0

%.prg:%.tas
	dasm $*.tas -o$*.prg -DCYCLES=63 -DDTV=0
%_ntsc.prg:%.tas
	dasm $*.tas -o$*_ntsc.prg -DCYCLES=65 -DDTV=0
%_ntscold.prg:%.tas
	dasm $*.tas -o$*_ntscold.prg -DCYCLES=64 -DDTV=0

clean:
	rm -f $(TESTS) $(TESTSNTSC) $(TESTSNTSCOLD)
