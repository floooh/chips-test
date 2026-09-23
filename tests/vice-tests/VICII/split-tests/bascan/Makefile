# Makefile

all:	bascan.prg

bascan.prg: bascan.asm ../common/startup.asm ../common/scandump.asm
	dasm $< -o$@

clean:
	rm -f *~ \#*\#
	rm -f a.out
	rm -f bascan.prg

# eof
