# Makefile

all:	spritescan.prg spritescan_drean.prg

spritescan.prg: spritescan.asm ../common/startup.asm ../common/scandump.asm
	dasm $< -DVICTYPE=6569 -o$@

spritescan_drean.prg: spritescan.asm ../common/startup.asm ../common/scandump.asm
	dasm $< -DVICTYPE=6572 -o$@

clean:
	rm -f *~ \#*\#
	rm -f a.out
	rm -f spritescan.prg
	rm -f spritescan_drean.prg

# eof
