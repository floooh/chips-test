# Makefile

all:	via_wrap2.prg via_wrap_pal.prg # via_wrap_ntsc.prg

via_wrap_pal.prg: via_wrap.asm macros.i
	dasm $< -o$@ -DIS_PAL=1

#via_wrap_ntsc.prg: via_wrap.asm macros.i
#	dasm $< -o$@ -DIS_PAL=0

via_wrap2.prg: via_wrap2.asm
	dasm $< -o$@

clean:
	rm -f *~ \#*\#
	rm -f a.out
	rm -f via_wrap_pal.prg
	rm -f via_wrap_ntsc.prg
	rm -f via_wrap2.prg
# eof
