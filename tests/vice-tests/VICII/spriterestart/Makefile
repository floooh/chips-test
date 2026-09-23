# Makefile

all:	spriterestart.prg

spriterestart.prg: spriterestart.asm
	dasm $< -o$@

clean:
	rm -f *~ \#*\#
	rm -f a.out
	rm -f spriterestart.prg
# eof
