#.SILENT:

all: nmitest.prg nmitest2.prg
nmitest.prg: main.s
	acme main.s
nmitest2.prg: main2.s
	acme main2.s
	
test: nmitest.prg
	x64 --autostart nmitest.prg

codenet: nmitest.prg
	codenet -x nmitest.prg
clean:
	rm -f  nmitest.prg
	rm -f  *~
 
