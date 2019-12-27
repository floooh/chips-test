
all: main.prg disk

main.prg: main.s
	acme -Dexpanded=0 -f cbm -o main.prg main.s
main-exp.prg: main.s
	acme -Dexpanded=1 -f cbm -o main-exp.prg main.s

disk: main.prg main-exp.prg
	c1541 -format "viapb7,00" d64 viapb7.d64 -write main.prg unexpanded -write main-exp.prg expanded

clean:
	rm -f main.prg
	rm -f main-exp.prg
	rm -f viapb7.d64
