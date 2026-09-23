# Makefile

all:	colorsplit.prg

colorsplit.prg:	colorsplit.asm
	dasm $< -o$@

clean:
	rm -f *~ \#*\#
	rm -f a.out
	rm -f colorsplit.prg

# eof
