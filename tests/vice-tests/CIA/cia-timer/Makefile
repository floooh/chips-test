
all: cia-timer-oldcias.prg cia-timer-newcias.prg

cia-timer-oldcias.prg: cia-timer.asm dump-oldcia.bin
	dasm cia-timer.asm -DDUMP=0 -ocia-timer-oldcias.prg
	
cia-timer-newcias.prg: cia-timer.asm dump-newcia.bin
	dasm cia-timer.asm -DDUMP=1 -ocia-timer-newcias.prg
	
clean:
	rm -f *~
	rm -f cia-timer-oldcias.prg
	rm -f cia-timer-newcias.prg
