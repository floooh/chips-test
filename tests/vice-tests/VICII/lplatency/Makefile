# Makefile

all:	lplatency.prg

lplatency.prg: lplatency.asm
	dasm $< -o$@

clean:
	rm -f *~ \#*\#
	rm -f a.out
	rm -f lplatency.prg

# eof
