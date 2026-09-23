
all: blackmail-ee.prg blackmail-fixed.prg

blackmail-ee.prg: blackmail.asm
	acme -f cbm -DSTABLEFIX=0 -o blackmail-ee.prg blackmail.asm
blackmail-fixed.prg: blackmail.asm
	acme -f cbm -DSTABLEFIX=1 -o blackmail-fixed.prg blackmail.asm

clean:
	rm -f blackmail-ee.prg
