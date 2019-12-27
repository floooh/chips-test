# Makefile

all:	lightpen.prg lightpen_ntsc.prg

lightpen.prg: lightpen.asm ../common/startup.asm ../common/scandump.asm
	dasm $< -o$@

lightpen_ntsc.prg: lightpen.asm ../common/startup.asm ../common/scandump.asm
	dasm $< -o$@ -DHARD_NTSC

analyse: analyse.c
	gcc -Wall -o $@ $<

clean:
	rm -f *~ \#*\#
	rm -f a.out
	rm -f lightpen.prg
	rm -f lightpen_ntsc.prg
	rm -f analyse
# eof
