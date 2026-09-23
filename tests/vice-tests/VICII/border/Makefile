.PHONY: all clean

.SUFFIXES: .prg .asm .tas

all: border-250.prg border-251.prg border-252.prg \
     border-bm-idle.prg border-bm-ysh.prg border-bm-ysh2.prg \
     border-mcbm.prg vborder.prg vborder_ntsc.prg vborder_ntscold.prg \
     vborder2.prg vborder2_ntsc.prg vborder2_ntscold.prg \
     hvborder1.prg hvborder2.prg \
     vborder-32-08.prg vborder-33-08.prg \
     vborder-32-09.prg vborder-33-09.prg \
     vborder2-21.prg vborder2-22.prg \
     vborder2-35.prg vborder2-36.prg \
     vborder2-63.prg vborder2-64.prg

%.prg: %.tas
	acme -v1 --cpu 6502 -f cbm -o $@ $<

border-250.prg: border-250.tas
border-251.prg: border-251.tas
border-252.prg: border-252.tas
border-bm-idle.prg: border-bm-idle.tas
border-bm-ysh.prg: border-bm-ysh.tas
border-bm-ysh2.prg: border-bm-ysh2.tas
border-mcbm.prg: border-mcbm.tas

vborder.prg: vborder.asm
	dasm $< -o$@ -DCYCLES=63 -DDEBUG=1 -DOFFSET=32 -DDELAY=32
vborder_ntsc.prg: vborder.asm
	dasm $< -o$@ -DCYCLES=65 -DDEBUG=1 -DOFFSET=32 -DDELAY=32
vborder_ntscold.prg: vborder.asm
	dasm $< -o$@ -DCYCLES=64 -DDEBUG=1 -DOFFSET=32 -DDELAY=32
	
vborder-32-08.prg: vborder.asm Makefile
	dasm $< -o$@ -DCYCLES=63 -DDEBUG=0 -DOFFSET=14 -DDELAY=56
vborder-32-09.prg: vborder.asm Makefile
	dasm $< -o$@ -DCYCLES=63 -DDEBUG=0 -DOFFSET=14 -DDELAY=55
vborder-33-08.prg: vborder.asm Makefile
	dasm $< -o$@ -DCYCLES=63 -DDEBUG=0 -DOFFSET=13 -DDELAY=56
vborder-33-09.prg: vborder.asm Makefile
	dasm $< -o$@ -DCYCLES=63 -DDEBUG=0 -DOFFSET=13 -DDELAY=55

vborder2.prg: vborder2.asm
	dasm $< -o$@ -DCYCLES=63 -DDEBUG=1 -DOFFSET=64
vborder2_ntsc.prg: vborder2.asm
	dasm $< -o$@ -DCYCLES=65 -DDEBUG=1 -DOFFSET=64
vborder2_ntscold.prg: vborder2.asm
	dasm $< -o$@ -DCYCLES=64 -DDEBUG=1 -DOFFSET=64
	
vborder2-21.prg: vborder2.asm Makefile
	dasm $< -o$@ -DCYCLES=63 -DDEBUG=0 -DOFFSET=95
vborder2-22.prg: vborder2.asm Makefile
	dasm $< -o$@ -DCYCLES=63 -DDEBUG=0 -DOFFSET=94
vborder2-35.prg: vborder2.asm Makefile
	dasm $< -o$@ -DCYCLES=63 -DDEBUG=0 -DOFFSET=75
vborder2-36.prg: vborder2.asm Makefile
	dasm $< -o$@ -DCYCLES=63 -DDEBUG=0 -DOFFSET=74
vborder2-63.prg: vborder2.asm Makefile
	dasm $< -o$@ -DCYCLES=63 -DDEBUG=0 -DOFFSET=29
vborder2-64.prg: vborder2.asm Makefile
	dasm $< -o$@ -DCYCLES=63 -DDEBUG=0 -DOFFSET=28

hvborder1.prg: hvborder.asm
	acme -f cbm -DMODE=0 -o hvborder1.prg hvborder.asm
hvborder2.prg: hvborder.asm
	acme -f cbm -DMODE=1 -o hvborder2.prg hvborder.asm

screenshots:
	x64sc -default -minimize -VICIIfilter 0 -VICIIextpal -VICIIpalette pepto-pal -debugcart -exitscreenshot references/vborder-32-08.prg.png vborder-32-08.prg
	x64sc -default -minimize -VICIIfilter 0 -VICIIextpal -VICIIpalette pepto-pal -debugcart -exitscreenshot references/vborder-32-09.prg.png vborder-32-09.prg
	x64sc -default -minimize -VICIIfilter 0 -VICIIextpal -VICIIpalette pepto-pal -debugcart -exitscreenshot references/vborder-33-08.prg.png vborder-33-08.prg
	x64sc -default -minimize -VICIIfilter 0 -VICIIextpal -VICIIpalette pepto-pal -debugcart -exitscreenshot references/vborder-33-09.prg.png vborder-33-09.prg
	x64sc -default -minimize -VICIIfilter 0 -VICIIextpal -VICIIpalette pepto-pal -debugcart -exitscreenshot references/vborder2-21.prg.png vborder2-21.prg
	x64sc -default -minimize -VICIIfilter 0 -VICIIextpal -VICIIpalette pepto-pal -debugcart -exitscreenshot references/vborder2-22.prg.png vborder2-22.prg
	x64sc -default -minimize -VICIIfilter 0 -VICIIextpal -VICIIpalette pepto-pal -debugcart -exitscreenshot references/vborder2-35.prg.png vborder2-35.prg
	x64sc -default -minimize -VICIIfilter 0 -VICIIextpal -VICIIpalette pepto-pal -debugcart -exitscreenshot references/vborder2-36.prg.png vborder2-36.prg
	x64sc -default -minimize -VICIIfilter 0 -VICIIextpal -VICIIpalette pepto-pal -debugcart -exitscreenshot references/vborder2-63.prg.png vborder2-63.prg
	x64sc -default -minimize -VICIIfilter 0 -VICIIextpal -VICIIpalette pepto-pal -debugcart -exitscreenshot references/vborder2-64.prg.png vborder2-64.prg
	
clean:
	$(RM) *.prg
