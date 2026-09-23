
all: vsp-tester.prg vsp-tester-ntsc.prg

vsp-tester.prg: vsp-tester.asm
	64tass -DNTSC=0 --m6502 -o vsp-tester.prg vsp-tester.asm
vsp-tester-ntsc.prg: vsp-tester.asm
	64tass -DNTSC=1 --m6502 -o vsp-tester-ntsc.prg vsp-tester.asm
	
test: vsp-tester.prg
	x64sc -default -autostartprgmode 1 vsp-tester.prg
test-ntsc: vsp-tester-ntsc.prg
	x64sc -default -ntsc -autostartprgmode 1 vsp-tester-ntsc.prg
	
clean:
	$(RM) vsp-tester.prg
	$(RM) vsp-tester-ntsc.prg
