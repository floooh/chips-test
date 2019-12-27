
all: raster-ntsc.prg

raster-ntsc.prg: raster-ntsc.bas
	petcat -w2 -l 1001 -o raster-ntsc.prg -- raster-ntsc.bas

test: raster-ntsc.prg
	xvic -default -ntsc raster-ntsc.prg

clean:
	rm -f raster-ntsc.prg

