
all: sbsprf24.prg sbsprf24-163.prg sbsprf24-164.prg

sbsprf24.prg: sbsprf24.asm
	acme -f cbm -DACT_ANIM=1 -DSPR6_X=100 -o sbsprf24.prg sbsprf24.asm

sbsprf24-163.prg: sbsprf24.asm
	acme -f cbm -DACT_ANIM=0 -DSPR6_X=99 -o sbsprf24-163.prg sbsprf24.asm
sbsprf24-164.prg: sbsprf24.asm
	acme -f cbm -DACT_ANIM=0 -DSPR6_X=100 -o sbsprf24-164.prg sbsprf24.asm
	
clean:
	$(RM) sbsprf24.prg
	$(RM) sbsprf24-163.prg
	$(RM) sbsprf24-164.prg
