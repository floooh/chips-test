# Makefile

all:	fetchsplit.prg

fetchsplit.prg: fetchsplit.asm ../common/startup.asm ../common/onescreen.asm
	dasm $< -o$@

clean:
	rm -f *~ \#*\#
	rm -f a.out
	rm -f fetchsplit.prg

# eof
