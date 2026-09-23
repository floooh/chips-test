
VICEOPTS=-default -minimize -VICIIfilter 0 -VICIIextpal -VICIIpalette pepto-pal -debugcart

TESTS= spritecrunch.prg spritecrunch2.prg \
	spritecrunch-3b-00.prg \
	spritecrunch-3c-00.prg \
	spritecrunch-3d-00.prg \
	spritecrunch2-07.prg \
	spritecrunch2-08.prg \
	spritecrunch2-09.prg \
	spritecrunch2-25.prg \
	spritecrunch2-26.prg \
	spritecrunch2-27.prg \
	spritecrunch2-28.prg \
	spritecrunch2-29.prg

all: $(TESTS)

spritecrunch.prg: spritecrunch.asm
	KickAss :debug=1 :delay=60 :offset=0 spritecrunch.asm -o spritecrunch.prg
spritecrunch-3b-00.prg: spritecrunch.asm
	KickAss :debug=0 :delay=60 :offset=0 spritecrunch.asm -o spritecrunch-3b-00.prg
spritecrunch-3c-00.prg: spritecrunch.asm
	KickAss :debug=0 :delay=60 :offset=0 spritecrunch.asm -o spritecrunch-3c-00.prg
spritecrunch-3d-00.prg: spritecrunch.asm
	KickAss :debug=0 :delay=60 :offset=0 spritecrunch.asm -o spritecrunch-3d-00.prg

spritecrunch2.prg: spritecrunch2.asm
	KickAss :debug=1 :delay=8 spritecrunch2.asm -o spritecrunch2.prg
spritecrunch2-07.prg: spritecrunch2.asm
	KickAss :debug=0 :delay=7 spritecrunch2.asm -o spritecrunch2-07.prg
spritecrunch2-08.prg: spritecrunch2.asm
	KickAss :debug=0 :delay=8 spritecrunch2.asm -o spritecrunch2-08.prg
spritecrunch2-09.prg: spritecrunch2.asm
	KickAss :debug=0 :delay=9 spritecrunch2.asm -o spritecrunch2-09.prg
spritecrunch2-25.prg: spritecrunch2.asm
	KickAss :debug=0 :delay=37 spritecrunch2.asm -o spritecrunch2-25.prg
spritecrunch2-26.prg: spritecrunch2.asm
	KickAss :debug=0 :delay=38 spritecrunch2.asm -o spritecrunch2-26.prg
spritecrunch2-27.prg: spritecrunch2.asm
	KickAss :debug=0 :delay=39 spritecrunch2.asm -o spritecrunch2-27.prg
spritecrunch2-28.prg: spritecrunch2.asm
	KickAss :debug=0 :delay=40 spritecrunch2.asm -o spritecrunch2-28.prg
spritecrunch2-29.prg: spritecrunch2.asm
	KickAss :debug=0 :delay=41 spritecrunch2.asm -o spritecrunch2-29.prg

screenshots1: all
	x64sc $(VICEOPTS) -exitscreenshot references/spritecrunch-3b-00.prg.png spritecrunch-3b-00.prg
	x64sc $(VICEOPTS) -exitscreenshot references/spritecrunch-3c-00.prg.png spritecrunch-3c-00.prg
	x64sc $(VICEOPTS) -exitscreenshot references/spritecrunch-3d-00.prg.png spritecrunch-3d-00.prg
	
screenshots2: all
	x64sc $(VICEOPTS) -exitscreenshot references/spritecrunch2-07.prg.png spritecrunch2-07.prg
	x64sc $(VICEOPTS) -exitscreenshot references/spritecrunch2-08.prg.png spritecrunch2-08.prg
	x64sc $(VICEOPTS) -exitscreenshot references/spritecrunch2-09.prg.png spritecrunch2-09.prg
	x64sc $(VICEOPTS) -exitscreenshot references/spritecrunch2-25.prg.png spritecrunch2-25.prg
	x64sc $(VICEOPTS) -exitscreenshot references/spritecrunch2-26.prg.png spritecrunch2-26.prg
	x64sc $(VICEOPTS) -exitscreenshot references/spritecrunch2-27.prg.png spritecrunch2-27.prg
	x64sc $(VICEOPTS) -exitscreenshot references/spritecrunch2-28.prg.png spritecrunch2-28.prg
	x64sc $(VICEOPTS) -exitscreenshot references/spritecrunch2-29.prg.png spritecrunch2-29.prg
	
screenshots: screenshots1 screenshots2	
	
clean:
	$(RM) *~ *.sym
	$(RM) $(TESTS)
