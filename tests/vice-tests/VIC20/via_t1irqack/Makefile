
PROGS = bandits-via1.prg bandits-via2.prg bandits-via1-8k.prg bandits-via2-8k.prg

all: $(PROGS) disk

bandits-via1.prg: bandits.asm
	acme -Dexpanded=0 -DVIA=1 -f cbm -o bandits-via1.prg bandits.asm
bandits-via2.prg: bandits.asm
	acme -Dexpanded=0 -DVIA=2 -f cbm -o bandits-via2.prg bandits.asm

bandits-via1-8k.prg: bandits.asm
	acme -Dexpanded=1 -DVIA=1 -f cbm -o bandits-via1-8k.prg bandits.asm
bandits-via2-8k.prg: bandits.asm
	acme -Dexpanded=1 -DVIA=2 -f cbm -o bandits-via2-8k.prg bandits.asm

disk:
	c1541 -format "test,00" d64 viat1irqack.d64 \
		-write bandits-via1.prg bandits-via1 \
		-write bandits-via2.prg bandits-via2 \
		-write bandits-via1.prg bandits-via1-8k \
		-write bandits-via2.prg bandits-via2-8k

test: all
	xvic -default -pal bandits-via2.prg

clean:
	rm -f *~
	rm -f viat1irqack.d64
	rm -f $(PROGS)
