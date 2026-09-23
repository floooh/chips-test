
VICEOPTS=-default -minimize -VICIIfilter 0 -VICIIextpal -VICIIpalette pepto-pal -debugcart

TESTS=test.prg \
	test-136-2a.prg

all: $(TESTS)

test.prg: test.asm
	acme -DDEBUG=0 -DXPOS=0 -DNUMLINES=42 -v1 --cpu 6502 -f cbm -o test.prg test.asm

test-136-2a.prg: test.asm
	acme -DDEBUG=0 -DXPOS=310 -DNUMLINES=42 -v1 --cpu 6502 -f cbm -o test-136-2a.prg test.asm
	
clean:
	$(RM) $(TESTS)
