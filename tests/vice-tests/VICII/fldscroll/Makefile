
all: fldscroll.prg \
	fldscroll-20-60.prg \
	fldscroll-21-60.prg \
	fldscroll-22-60.prg \
	fldscroll-29-60.prg \
	fldscroll-2A-60.prg \
	fldscroll-2B-60.prg

fldscroll.prg: fldscroll.s
	64tass -D CYCLE=0 -D OFFSET=0 -o fldscroll.prg fldscroll.s
 
fldscroll-20-60.prg: fldscroll.s
	64tass -D CYCLE=32 -D OFFSET=96 -o fldscroll-20-60.prg fldscroll.s
fldscroll-21-60.prg: fldscroll.s
	64tass -D CYCLE=33 -D OFFSET=96 -o fldscroll-21-60.prg fldscroll.s
fldscroll-22-60.prg: fldscroll.s
	64tass -D CYCLE=34 -D OFFSET=96 -o fldscroll-22-60.prg fldscroll.s
fldscroll-29-60.prg: fldscroll.s
	64tass -D CYCLE=41 -D OFFSET=96 -o fldscroll-29-60.prg fldscroll.s
fldscroll-2A-60.prg: fldscroll.s
	64tass -D CYCLE=42 -D OFFSET=96 -o fldscroll-2A-60.prg fldscroll.s
fldscroll-2B-60.prg: fldscroll.s
	64tass -D CYCLE=43 -D OFFSET=96 -o fldscroll-2B-60.prg fldscroll.s

screenshots:
	x64sc -default -minimize -VICIIfilter 0 -VICIIextpal -VICIIpalette pepto-pal -debugcart -exitscreenshot references/fldscroll-20-60.prg.png fldscroll-20-60.prg
	x64sc -default -minimize -VICIIfilter 0 -VICIIextpal -VICIIpalette pepto-pal -debugcart -exitscreenshot references/fldscroll-21-60.prg.png fldscroll-21-60.prg
	x64sc -default -minimize -VICIIfilter 0 -VICIIextpal -VICIIpalette pepto-pal -debugcart -exitscreenshot references/fldscroll-22-60.prg.png fldscroll-22-60.prg
	x64sc -default -minimize -VICIIfilter 0 -VICIIextpal -VICIIpalette pepto-pal -debugcart -exitscreenshot references/fldscroll-29-60.prg.png fldscroll-29-60.prg
	x64sc -default -minimize -VICIIfilter 0 -VICIIextpal -VICIIpalette pepto-pal -debugcart -exitscreenshot references/fldscroll-2A-60.prg.png fldscroll-2A-60.prg
	x64sc -default -minimize -VICIIfilter 0 -VICIIextpal -VICIIpalette pepto-pal -debugcart -exitscreenshot references/fldscroll-2B-60.prg.png fldscroll-2B-60.prg
	
clean:
	$(RM) fldscroll.prg 
	$(RM) fldscroll-20-60.prg
	$(RM) fldscroll-21-60.prg
	$(RM) fldscroll-22-60.prg
	$(RM) fldscroll-29-60.prg
	$(RM) fldscroll-2A-60.prg
	$(RM) fldscroll-2B-60.prg
