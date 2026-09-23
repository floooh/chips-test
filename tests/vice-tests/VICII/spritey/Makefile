# Makefile

all:	spritey.prg

spritey.prg: spritey.asm
	dasm $< -o$@

clean:
	rm -f *~ \#*\#
	rm -f a.out
	rm -f spritey.prg

# eof
