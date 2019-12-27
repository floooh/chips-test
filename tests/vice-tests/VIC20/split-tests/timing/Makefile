# Makefile

all:	timing.prg timing_ntsc.prg

timing.prg: timing.asm ../common/startup.asm ../common/scandump.asm
	dasm $< -o$@

timing_ntsc.prg: timing.asm ../common/startup.asm ../common/scandump.asm
	dasm $< -o$@ -DHARD_NTSC

clean:
	rm -f *~ \#*\#
	rm -f a.out
	rm -f timing.prg
	rm -f timing_ntsc.prg
# eof
