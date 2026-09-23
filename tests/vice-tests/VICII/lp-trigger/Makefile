 

all: lpcart.crt test1.prg test2.prg test2new.prg

test1.prg: test1.s stableraster_lightpen.s
	cl65 -t c64 -C c64-asm.cfg -u __EXEHDR__  -o test1.prg test1.s stableraster_lightpen.s

test2.prg: test2.s stableraster_lightpen.s
	cl65 -t c64 -C c64-asm.cfg -u __EXEHDR__  -o test2.prg test2.s
test2new.prg: test2.s stableraster_lightpen.s
	cl65 -t c64 --asm-define NEWVIC -C c64-asm.cfg -u __EXEHDR__  -o test2new.prg test2.s

lpcart.crt: lpcart.s
	acme lpcart.s
	cartconv -t ulti -i lpcart.bin -o lpcart.crt

clean:
	rm -f *~
	rm -f *.o
	rm -f lpcart.bin lpcart.crt
	rm -f *.prg

