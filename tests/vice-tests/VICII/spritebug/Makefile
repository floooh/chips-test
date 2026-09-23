
VICEOPTS=-default -minimize -VICIIfilter 0 -VICIIextpal -VICIIpalette pepto-pal -debugcart

all: spritebug.prg spritebug-104.prg spritebug-105.prg spritebug-106.prg

spritebug.prg: spritebug.s Makefile
	xa -DCOUNTER=0 -o spritebug.prg spritebug.s
spritebug-104.prg: spritebug.s Makefile
	xa -DCOUNTER=104 -o spritebug-104.prg spritebug.s
spritebug-105.prg: spritebug.s Makefile
	xa -DCOUNTER=105 -o spritebug-105.prg spritebug.s
spritebug-106.prg: spritebug.s Makefile
	xa -DCOUNTER=106 -o spritebug-106.prg spritebug.s

	
screenshots: all
	x64sc $(VICEOPTS) -exitscreenshot references/spritebug-104.prg.png spritebug-104.prg
	x64sc $(VICEOPTS) -exitscreenshot references/spritebug-105.prg.png spritebug-105.prg
	x64sc $(VICEOPTS) -exitscreenshot references/spritebug-106.prg.png spritebug-106.prg
	
clean:
	rm -f *~
	rm -f spritebug.prg
	rm -f spritebug-104.prg
	rm -f spritebug-105.prg
	rm -f spritebug-106.prg

