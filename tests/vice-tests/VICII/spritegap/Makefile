
all: spritegap2.prg spritegap3.prg

spritegap2.prg: spritegap2.tas
	dasm spritegap2.tas -ospritegap2.prg

spritegap3.prg: spritegap3.tas
	dasm spritegap3.tas -ospritegap3.prg

clean:
	rm -f  spritegap2.prg spritegap3.prg