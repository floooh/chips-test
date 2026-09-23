# Makefile

all:	rasterirq_hold.prg

rasterirq_hold.prg: rasterirq_hold.asm
	dasm $< -o$@

clean:
	rm -f *~ \#*\#
	rm -f a.out
	rm -f rasterirq_hold.prg

# eof
