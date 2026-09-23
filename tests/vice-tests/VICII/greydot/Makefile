# Makefile

all:	greydot.prg

greydot.prg: greydot.asm
	dasm $< -o$@

clean:
	rm -f *~ \#*\#
	rm -f a.out
	rm -f greydot.prg

# eof
