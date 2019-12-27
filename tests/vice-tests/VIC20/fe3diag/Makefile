# Makefile

all:	fe3diag.prg

fe3diag.prg: fe3diag.asm
	dasm $< -o$@

clean:
	rm -f *~ \#*\#
	rm -f a.out
	rm -f fe3diag.prg
# eof
