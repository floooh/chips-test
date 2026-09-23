# Makefile

all:	spritemcbase.prg

spritemcbase.prg: spritemcbase.asm
	dasm $< -o$@

clean:
	rm -f *~ \#*\#
	rm -f a.out
	rm -f spritemcbase.prg

# eof
