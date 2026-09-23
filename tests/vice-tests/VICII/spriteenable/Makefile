COMPILER = acme
CFLAGS = -v1 --cpu 6502 -f cbm -o

.PHONY: all clean

.SUFFIXES: .prg .asm
%.prg: %.asm
	$(COMPILER) $(CFLAGS) $@ $<

all: spriteenable1.prg spriteenable2.prg spriteenable3.prg spriteenable4.prg spriteenable5.prg \
     spriteenable1_ntsc.prg spriteenable2_ntsc.prg spriteenable3_ntsc.prg spriteenable4_ntsc.prg spriteenable5_ntsc.prg \
     spriteenable1_ntscold.prg spriteenable2_ntscold.prg spriteenable3_ntscold.prg spriteenable4_ntscold.prg spriteenable5_ntscold.prg

spriteenable1.prg: spriteenable1.asm core1.asm
spriteenable2.prg: spriteenable2.asm core2.asm
spriteenable3.prg: spriteenable3.asm core3.asm
spriteenable4.prg: spriteenable4.asm core4.asm
spriteenable5.prg: spriteenable5.asm core5.asm

spriteenable1_ntsc.prg: spriteenable1_ntsc.asm core1.asm
spriteenable2_ntsc.prg: spriteenable2_ntsc.asm core2.asm
spriteenable3_ntsc.prg: spriteenable3_ntsc.asm core3.asm
spriteenable4_ntsc.prg: spriteenable4_ntsc.asm core4.asm
spriteenable5_ntsc.prg: spriteenable5_ntsc.asm core5.asm

spriteenable1_ntscold.prg: spriteenable1_ntscold.asm core1.asm
spriteenable2_ntscold.prg: spriteenable2_ntscold.asm core2.asm
spriteenable3_ntscold.prg: spriteenable3_ntscold.asm core3.asm
spriteenable4_ntscold.prg: spriteenable4_ntscold.asm core4.asm
spriteenable5_ntscold.prg: spriteenable5_ntscold.asm core5.asm

spriteenable.d64: all
	c1541 -format spriteenable,12 d64 $@
	c1541 -attach $@ \
        -write spriteenable1.prg se1-pal \
        -write spriteenable2.prg se2-pal \
        -write spriteenable3.prg se3-pal \
        -write spriteenable4.prg se4-pal \
        -write spriteenable5.prg se5-pal
	c1541 -attach $@ \
        -write spriteenable1_ntsc.prg se1-ntsc \
        -write spriteenable2_ntsc.prg se2-ntsc \
        -write spriteenable3_ntsc.prg se3-ntsc \
        -write spriteenable4_ntsc.prg se4-ntsc \
        -write spriteenable5_ntsc.prg se5-ntsc
	c1541 -attach $@ \
        -write spriteenable1_ntscold.prg se1-ntscold \
        -write spriteenable2_ntscold.prg se2-ntscold \
        -write spriteenable3_ntscold.prg se3-ntscold \
        -write spriteenable4_ntscold.prg se4-ntscold \
        -write spriteenable5_ntscold.prg se5-ntscold

clean:
	rm *.prg
