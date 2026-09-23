# Makefile

all:	modesplit.prg

modesplit.prg: modesplit.asm ../common/startup.asm ../common/onescreen.asm
	dasm $< -o$@

clean:
	rm -f *~ \#*\#
	rm -f a.out
	rm -f modesplit.prg

# eof
