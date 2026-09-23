VICEOPTS=-default -minimize -VICIIfilter 0 -VICIIextpal -VICIIpalette pepto-pal -debugcart

TESTS=test1.prg test2.prg test3.prg test4.prg \
	test1-ntsc.prg test2-ntsc.prg test3-ntsc.prg \
	\
	test1-2a-03.prg \
	test1-2a-04.prg \
	test1-2a-10.prg \
	test1-2a-11.prg \
	test1-2a-16.prg \
	test1-2a-17.prg \
	test1-2a-18.prg \
	\
	test1-2e-03-ntsc.prg \
	test1-2e-04-ntsc.prg \
	test1-2e-10-ntsc.prg \
	test1-2e-11-ntsc.prg \
	test1-2e-16-ntsc.prg \
	test1-2e-17-ntsc.prg \
	test1-2e-18-ntsc.prg \
	\
	test2-28-05.prg \
	test2-28-06.prg \
	test2-28-11.prg \
	test2-28-12.prg \
	test2-28-16.prg \
	test2-28-17.prg \
	test2-28-18.prg \
	\
	test2-2c-05-ntsc.prg \
	test2-2c-06-ntsc.prg \
	test2-2c-11-ntsc.prg \
	test2-2c-12-ntsc.prg \
	test2-2c-16-ntsc.prg \
	test2-2c-17-ntsc.prg \
	test2-2c-18-ntsc.prg \
	\
	test3-28-07.prg \
	test3-28-08.prg \
	test3-28-13.prg \
	test3-28-14.prg \
	test3-28-18.prg \
	test3-28-19.prg \
	test3-28-1a.prg \
	\
	test3-2c-07-ntsc.prg \
	test3-2c-08-ntsc.prg \
	test3-2c-13-ntsc.prg \
	test3-2c-14-ntsc.prg \
	test3-2c-18-ntsc.prg \
	test3-2c-19-ntsc.prg \
	test3-2c-1a-ntsc.prg

all: $(TESTS)
	
test1.prg: dmadelay.s test1.s
	acme -f cbm -o test1.prg -DDEBUG=0 -DNTSC=0 test1.s
test1-2a-03.prg: dmadelay.s test1.s
	acme -f cbm -o test1-2a-03.prg -DDEBUG=1 -DNTSC=0 -DDEFXSTART=42 -DDEFXOFFS=3 test1.s
test1-2a-04.prg: dmadelay.s test1.s
	acme -f cbm -o test1-2a-04.prg -DDEBUG=1 -DNTSC=0 -DDEFXSTART=42 -DDEFXOFFS=4 test1.s
test1-2a-10.prg: dmadelay.s test1.s
	acme -f cbm -o test1-2a-10.prg -DDEBUG=1 -DNTSC=0 -DDEFXSTART=42 -DDEFXOFFS=16 test1.s
test1-2a-11.prg: dmadelay.s test1.s
	acme -f cbm -o test1-2a-11.prg -DDEBUG=1 -DNTSC=0 -DDEFXSTART=42 -DDEFXOFFS=17 test1.s
test1-2a-16.prg: dmadelay.s test1.s
	acme -f cbm -o test1-2a-16.prg -DDEBUG=1 -DNTSC=0 -DDEFXSTART=42 -DDEFXOFFS=22 test1.s
test1-2a-17.prg: dmadelay.s test1.s
	acme -f cbm -o test1-2a-17.prg -DDEBUG=1 -DNTSC=0 -DDEFXSTART=42 -DDEFXOFFS=23 test1.s
test1-2a-18.prg: dmadelay.s test1.s
	acme -f cbm -o test1-2a-18.prg -DDEBUG=1 -DNTSC=0 -DDEFXSTART=42 -DDEFXOFFS=24 test1.s

test1-ntsc.prg: dmadelay.s test1.s
	acme -f cbm -o test1-ntsc.prg -DDEBUG=0 -DNTSC=1 test1.s
test1-2e-03-ntsc.prg: dmadelay.s test1.s
	acme -f cbm -o test1-2e-03-ntsc.prg -DDEBUG=1 -DNTSC=1 -DDEFXSTART=46 -DDEFXOFFS=3 test1.s
test1-2e-04-ntsc.prg: dmadelay.s test1.s
	acme -f cbm -o test1-2e-04-ntsc.prg -DDEBUG=1 -DNTSC=1 -DDEFXSTART=46 -DDEFXOFFS=4 test1.s
test1-2e-10-ntsc.prg: dmadelay.s test1.s
	acme -f cbm -o test1-2e-10-ntsc.prg -DDEBUG=1 -DNTSC=1 -DDEFXSTART=46 -DDEFXOFFS=16 test1.s
test1-2e-11-ntsc.prg: dmadelay.s test1.s
	acme -f cbm -o test1-2e-11-ntsc.prg -DDEBUG=1 -DNTSC=1 -DDEFXSTART=46 -DDEFXOFFS=17 test1.s
test1-2e-16-ntsc.prg: dmadelay.s test1.s
	acme -f cbm -o test1-2e-16-ntsc.prg -DDEBUG=1 -DNTSC=1 -DDEFXSTART=46 -DDEFXOFFS=22 test1.s
test1-2e-17-ntsc.prg: dmadelay.s test1.s
	acme -f cbm -o test1-2e-17-ntsc.prg -DDEBUG=1 -DNTSC=1 -DDEFXSTART=46 -DDEFXOFFS=23 test1.s
test1-2e-18-ntsc.prg: dmadelay.s test1.s
	acme -f cbm -o test1-2e-18-ntsc.prg -DDEBUG=1 -DNTSC=1 -DDEFXSTART=46 -DDEFXOFFS=24 test1.s

test2.prg: dmadelay.s test2.s
	acme -f cbm -o test2.prg -DDEBUG=0 -DNTSC=0 test2.s
test2-28-05.prg: dmadelay.s test2.s
	acme -f cbm -o test2-28-05.prg -DDEBUG=1 -DNTSC=0 -DDEFXSTART=40 -DDEFXOFFS=5 test2.s
test2-28-06.prg: dmadelay.s test2.s
	acme -f cbm -o test2-28-06.prg -DDEBUG=1 -DNTSC=0 -DDEFXSTART=40 -DDEFXOFFS=6 test2.s
test2-28-11.prg: dmadelay.s test2.s
	acme -f cbm -o test2-28-11.prg -DDEBUG=1 -DNTSC=0 -DDEFXSTART=40 -DDEFXOFFS=17 test2.s
test2-28-12.prg: dmadelay.s test2.s
	acme -f cbm -o test2-28-12.prg -DDEBUG=1 -DNTSC=0 -DDEFXSTART=40 -DDEFXOFFS=18 test2.s
test2-28-16.prg: dmadelay.s test2.s
	acme -f cbm -o test2-28-16.prg -DDEBUG=1 -DNTSC=0 -DDEFXSTART=40 -DDEFXOFFS=22 test2.s
test2-28-17.prg: dmadelay.s test2.s
	acme -f cbm -o test2-28-17.prg -DDEBUG=1 -DNTSC=0 -DDEFXSTART=40 -DDEFXOFFS=23 test2.s
test2-28-18.prg: dmadelay.s test2.s
	acme -f cbm -o test2-28-18.prg -DDEBUG=1 -DNTSC=0 -DDEFXSTART=40 -DDEFXOFFS=24 test2.s

test2-ntsc.prg: dmadelay.s test2.s
	acme -f cbm -o test2-ntsc.prg -DDEBUG=0 -DNTSC=1 test2.s
test2-2c-05-ntsc.prg: dmadelay.s test2.s
	acme -f cbm -o test2-2c-05-ntsc.prg -DDEBUG=1 -DNTSC=1 -DDEFXSTART=44 -DDEFXOFFS=5 test2.s
test2-2c-06-ntsc.prg: dmadelay.s test2.s
	acme -f cbm -o test2-2c-06-ntsc.prg -DDEBUG=1 -DNTSC=1 -DDEFXSTART=44 -DDEFXOFFS=6 test2.s
test2-2c-11-ntsc.prg: dmadelay.s test2.s
	acme -f cbm -o test2-2c-11-ntsc.prg -DDEBUG=1 -DNTSC=1 -DDEFXSTART=44 -DDEFXOFFS=17 test2.s
test2-2c-12-ntsc.prg: dmadelay.s test2.s
	acme -f cbm -o test2-2c-12-ntsc.prg -DDEBUG=1 -DNTSC=1 -DDEFXSTART=44 -DDEFXOFFS=18 test2.s
test2-2c-16-ntsc.prg: dmadelay.s test2.s
	acme -f cbm -o test2-2c-16-ntsc.prg -DDEBUG=1 -DNTSC=1 -DDEFXSTART=44 -DDEFXOFFS=22 test2.s
test2-2c-17-ntsc.prg: dmadelay.s test2.s
	acme -f cbm -o test2-2c-17-ntsc.prg -DDEBUG=1 -DNTSC=1 -DDEFXSTART=44 -DDEFXOFFS=23 test2.s
test2-2c-18-ntsc.prg: dmadelay.s test2.s
	acme -f cbm -o test2-2c-18-ntsc.prg -DDEBUG=1 -DNTSC=1 -DDEFXSTART=44 -DDEFXOFFS=24 test2.s

test3.prg: dmadelay.s test3.s
	acme -f cbm -o test3.prg -DDEBUG=0 -DNTSC=0 test3.s
test3-28-07.prg: dmadelay.s test3.s
	acme -f cbm -o test3-28-07.prg -DDEBUG=1 -DNTSC=0 -DDEFXSTART=40 -DDEFXOFFS=7 test3.s
test3-28-08.prg: dmadelay.s test3.s
	acme -f cbm -o test3-28-08.prg -DDEBUG=1 -DNTSC=0 -DDEFXSTART=40 -DDEFXOFFS=8 test3.s
test3-28-13.prg: dmadelay.s test3.s
	acme -f cbm -o test3-28-13.prg -DDEBUG=1 -DNTSC=0 -DDEFXSTART=40 -DDEFXOFFS=19 test3.s
test3-28-14.prg: dmadelay.s test3.s
	acme -f cbm -o test3-28-14.prg -DDEBUG=1 -DNTSC=0 -DDEFXSTART=40 -DDEFXOFFS=20 test3.s
test3-28-18.prg: dmadelay.s test3.s
	acme -f cbm -o test3-28-18.prg -DDEBUG=1 -DNTSC=0 -DDEFXSTART=40 -DDEFXOFFS=24 test3.s
test3-28-19.prg: dmadelay.s test3.s
	acme -f cbm -o test3-28-19.prg -DDEBUG=1 -DNTSC=0 -DDEFXSTART=40 -DDEFXOFFS=25 test3.s
test3-28-1a.prg: dmadelay.s test3.s
	acme -f cbm -o test3-28-1a.prg -DDEBUG=1 -DNTSC=0 -DDEFXSTART=40 -DDEFXOFFS=26 test3.s

test3-ntsc.prg: dmadelay.s test3.s
	acme -f cbm -o test3-ntsc.prg -DDEBUG=0 -DNTSC=1 test3.s
test3-2c-07-ntsc.prg: dmadelay.s test3.s
	acme -f cbm -o test3-2c-07-ntsc.prg -DDEBUG=1 -DNTSC=1 -DDEFXSTART=44 -DDEFXOFFS=7 test3.s
test3-2c-08-ntsc.prg: dmadelay.s test3.s
	acme -f cbm -o test3-2c-08-ntsc.prg -DDEBUG=1 -DNTSC=1 -DDEFXSTART=44 -DDEFXOFFS=8 test3.s
test3-2c-13-ntsc.prg: dmadelay.s test3.s
	acme -f cbm -o test3-2c-13-ntsc.prg -DDEBUG=1 -DNTSC=1 -DDEFXSTART=44 -DDEFXOFFS=19 test3.s
test3-2c-14-ntsc.prg: dmadelay.s test3.s
	acme -f cbm -o test3-2c-14-ntsc.prg -DDEBUG=1 -DNTSC=1 -DDEFXSTART=44 -DDEFXOFFS=20 test3.s
test3-2c-18-ntsc.prg: dmadelay.s test3.s
	acme -f cbm -o test3-2c-18-ntsc.prg -DDEBUG=1 -DNTSC=1 -DDEFXSTART=44 -DDEFXOFFS=24 test3.s
test3-2c-19-ntsc.prg: dmadelay.s test3.s
	acme -f cbm -o test3-2c-19-ntsc.prg -DDEBUG=1 -DNTSC=1 -DDEFXSTART=44 -DDEFXOFFS=25 test3.s
test3-2c-1a-ntsc.prg: dmadelay.s test3.s
	acme -f cbm -o test3-2c-1a-ntsc.prg -DDEBUG=1 -DNTSC=1 -DDEFXSTART=44 -DDEFXOFFS=26 test3.s

test4.prg: test4.s
	acme -f cbm -o test4.prg -DDEBUG=0 test4.s

screenshots1: all
	x64sc $(VICEOPTS) -exitscreenshot references/test1-2a-03.prg.png test1-2a-03.prg
	x64sc $(VICEOPTS) -exitscreenshot references/test1-2a-04.prg.png test1-2a-04.prg
	x64sc $(VICEOPTS) -exitscreenshot references/test1-2a-10.prg.png test1-2a-10.prg
	x64sc $(VICEOPTS) -exitscreenshot references/test1-2a-11.prg.png test1-2a-11.prg
	x64sc $(VICEOPTS) -exitscreenshot references/test1-2a-16.prg.png test1-2a-16.prg
	x64sc $(VICEOPTS) -exitscreenshot references/test1-2a-17.prg.png test1-2a-17.prg
	x64sc $(VICEOPTS) -exitscreenshot references/test1-2a-18.prg.png test1-2a-18.prg
	
screenshots1ntsc: all
	x64sc $(VICEOPTS) -ntsc -exitscreenshot references/test1-2e-03-ntsc.prg.png test1-2e-03-ntsc.prg
	x64sc $(VICEOPTS) -ntsc -exitscreenshot references/test1-2e-04-ntsc.prg.png test1-2e-04-ntsc.prg
	x64sc $(VICEOPTS) -ntsc -exitscreenshot references/test1-2e-10-ntsc.prg.png test1-2e-10-ntsc.prg
	x64sc $(VICEOPTS) -ntsc -exitscreenshot references/test1-2e-11-ntsc.prg.png test1-2e-11-ntsc.prg
	x64sc $(VICEOPTS) -ntsc -exitscreenshot references/test1-2e-16-ntsc.prg.png test1-2e-16-ntsc.prg
	x64sc $(VICEOPTS) -ntsc -exitscreenshot references/test1-2e-17-ntsc.prg.png test1-2e-17-ntsc.prg
	x64sc $(VICEOPTS) -ntsc -exitscreenshot references/test1-2e-18-ntsc.prg.png test1-2e-18-ntsc.prg
	
screenshots2: all
	x64sc $(VICEOPTS) -exitscreenshot references/test2-28-05.prg.png test2-28-05.prg
	x64sc $(VICEOPTS) -exitscreenshot references/test2-28-06.prg.png test2-28-06.prg
	x64sc $(VICEOPTS) -exitscreenshot references/test2-28-11.prg.png test2-28-11.prg
	x64sc $(VICEOPTS) -exitscreenshot references/test2-28-12.prg.png test2-28-12.prg
	x64sc $(VICEOPTS) -exitscreenshot references/test2-28-16.prg.png test2-28-16.prg
	x64sc $(VICEOPTS) -exitscreenshot references/test2-28-17.prg.png test2-28-17.prg
	x64sc $(VICEOPTS) -exitscreenshot references/test2-28-18.prg.png test2-28-18.prg

screenshots2ntsc: all
	x64sc $(VICEOPTS) -ntsc -exitscreenshot references/test2-2c-05-ntsc.prg.png test2-2c-05-ntsc.prg
	x64sc $(VICEOPTS) -ntsc -exitscreenshot references/test2-2c-06-ntsc.prg.png test2-2c-06-ntsc.prg
	x64sc $(VICEOPTS) -ntsc -exitscreenshot references/test2-2c-11-ntsc.prg.png test2-2c-11-ntsc.prg
	x64sc $(VICEOPTS) -ntsc -exitscreenshot references/test2-2c-12-ntsc.prg.png test2-2c-12-ntsc.prg
	x64sc $(VICEOPTS) -ntsc -exitscreenshot references/test2-2c-16-ntsc.prg.png test2-2c-16-ntsc.prg
	x64sc $(VICEOPTS) -ntsc -exitscreenshot references/test2-2c-17-ntsc.prg.png test2-2c-17-ntsc.prg
	x64sc $(VICEOPTS) -ntsc -exitscreenshot references/test2-2c-18-ntsc.prg.png test2-2c-18-ntsc.prg

screenshots3: all
	x64sc $(VICEOPTS) -exitscreenshot references/test3-28-07.prg.png test3-28-07.prg
	x64sc $(VICEOPTS) -exitscreenshot references/test3-28-08.prg.png test3-28-08.prg
	x64sc $(VICEOPTS) -exitscreenshot references/test3-28-13.prg.png test3-28-13.prg
	x64sc $(VICEOPTS) -exitscreenshot references/test3-28-14.prg.png test3-28-14.prg
	x64sc $(VICEOPTS) -exitscreenshot references/test3-28-18.prg.png test3-28-18.prg
	x64sc $(VICEOPTS) -exitscreenshot references/test3-28-19.prg.png test3-28-19.prg
	x64sc $(VICEOPTS) -exitscreenshot references/test3-28-1a.prg.png test3-28-1a.prg
	
screenshots3ntsc: all
	x64sc $(VICEOPTS) -ntsc -exitscreenshot references/test3-2c-07-ntsc.prg.png test3-2c-07-ntsc.prg
	x64sc $(VICEOPTS) -ntsc -exitscreenshot references/test3-2c-08-ntsc.prg.png test3-2c-08-ntsc.prg
	x64sc $(VICEOPTS) -ntsc -exitscreenshot references/test3-2c-13-ntsc.prg.png test3-2c-13-ntsc.prg
	x64sc $(VICEOPTS) -ntsc -exitscreenshot references/test3-2c-14-ntsc.prg.png test3-2c-14-ntsc.prg
	x64sc $(VICEOPTS) -ntsc -exitscreenshot references/test3-2c-18-ntsc.prg.png test3-2c-18-ntsc.prg
	x64sc $(VICEOPTS) -ntsc -exitscreenshot references/test3-2c-19-ntsc.prg.png test3-2c-19-ntsc.prg
	x64sc $(VICEOPTS) -ntsc -exitscreenshot references/test3-2c-1a-ntsc.prg.png test3-2c-1a-ntsc.prg
	
screenshots: screenshots1 screenshots2 screenshots3
	
clean:
	$(RM) *~
	$(RM) $(TESTS)
