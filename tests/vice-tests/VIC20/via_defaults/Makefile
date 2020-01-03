all: defaults.prg defaults.d64

defaults.prg: main.asm
	acme -f cbm -o defaults.prg main.asm

defaults.d64: defaults.prg
	c1541 -format "defaults,00" d64 defaults.d64 \
		-write defaults.prg defaults \

test: all
	xvic -default -memory 8k defaults.prg

clean:
	rm -f *~
	rm -f defaults.d64
	rm -f defaults.prg
