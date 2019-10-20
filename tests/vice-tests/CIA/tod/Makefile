
TESTS=\
	alarm.prg alarm-cond.prg alarm-cond2.prg hour-test.prg stability.prg stability-ntsc.prg \
	4tod.prg 4todcia1.prg 5tod.prg 6tod.prg powerup.prg fix-hour.prg fix-min.prg fix-sec.prg fix-tsec.prg \
	write-stop.prg read-latch.prg hzsync0.prg hzsync1.prg hzsync2.prg hzsync3.prg hzsync4.prg hzsync5.prg \
	0alarm.prg 1alarm.prg hammerfist0.prg hammerfist1.prg \
	frogger.prg

all: $(TESTS)
	

alarm.prg: alarm.s
	acme -o alarm.prg alarm.s
alarm-cond.prg: alarm-cond.s
	acme -o alarm-cond.prg alarm-cond.s
alarm-cond2.prg: alarm-cond2.s
	acme -o alarm-cond2.prg alarm-cond2.s

hzsync0.prg: hzsync0.asm
	64tass -a -o hzsync0.prg hzsync0.asm
hzsync1.prg: hzsync1.asm
	64tass -a -o hzsync1.prg hzsync1.asm
hzsync2.prg: hzsync2.asm
	64tass -a -o hzsync2.prg hzsync2.asm
hzsync3.prg: hzsync3.asm
	64tass -a -o hzsync3.prg hzsync3.asm
hzsync4.prg: hzsync4.asm
	64tass -a -o hzsync4.prg hzsync4.asm
hzsync5.prg: hzsync5.asm
	64tass -a -o hzsync5.prg hzsync5.asm
	
stability.prg: stability.s
	acme -DSYNC=0 -o stability.prg stability.s
stability-ntsc.prg: stability.s
	acme -DSYNC=1 -o stability-ntsc.prg stability.s

powerup.prg: powerup.s
	acme -o powerup.prg powerup.s
	
read-latch.prg: read-latch.s
	acme -o read-latch.prg read-latch.s
write-stop.prg: write-stop.s
	acme -o write-stop.prg write-stop.s
	
fix-hour.prg: fix-hour.s
	acme -o fix-hour.prg fix-hour.s
fix-min.prg: fix-min.s
	acme -o fix-min.prg fix-min.s
fix-sec.prg: fix-sec.s
	acme -o fix-sec.prg fix-sec.s
fix-tsec.prg: fix-tsec.s
	acme -o fix-tsec.prg fix-tsec.s

0alarm.prg: 0alarm.asm
	64tass -a -o 0alarm.prg 0alarm.asm
1alarm.prg: 1alarm.asm
	64tass -a -o 1alarm.prg 1alarm.asm
	
4tod.prg: 4tod.asm
	64tass -a -o 4tod.prg 4tod.asm
4todcia1.prg: 4todcia1.asm
	64tass -a -o 4todcia1.prg 4todcia1.asm
5tod.prg: 5tod.asm
	64tass -a -o 5tod.prg 5tod.asm
6tod.prg: 6tod.asm
	64tass -a -o 6tod.prg 6tod.asm
	
hour-test.prg: hour-test.bas
	petcat -w2 -o hour-test.prg -- hour-test.bas
	
hammerfist0.prg: hammerfist0.asm
	dasm hammerfist0.asm -ohammerfist0.prg
hammerfist1.prg: hammerfist1.asm
	dasm hammerfist1.asm -ohammerfist1.prg

frogger.prg: frogger.asm
	acme -f cbm -o frogger.prg frogger.asm

clean:
	rm -f *.prg