

all: cia1.prg cia2.prg cia1new.prg cia2new.prg irq-ack-vicii.prg irq-ackn_after_cli.prg irq-ackn_after_cli2.prg via1.prg via1-free.prg

cia1.prg: cia1.tas
	dasm cia1.tas -ocia1.prg -DNEWCIA=0
cia2.prg: cia2.tas
	dasm cia2.tas -ocia2.prg -DNEWCIA=0
cia1new.prg: cia1.tas
	dasm cia1.tas -ocia1new.prg -DNEWCIA=1
cia2new.prg: cia2.tas
	dasm cia2.tas -ocia2new.prg -DNEWCIA=1

irq-ack-vicii.prg: irq-ack-vicii.asm
	acme -f cbm -o irq-ack-vicii.prg irq-ack-vicii.asm
irq-ackn_after_cli.prg: irq-ackn_after_cli.tas
	dasm irq-ackn_after_cli.tas -oirq-ackn_after_cli.prg
irq-ackn_after_cli2.prg: irq-ackn_after_cli2.tas
	dasm irq-ackn_after_cli2.tas -oirq-ackn_after_cli2.prg
	
via1.prg: via1.tas
	dasm via1.tas -ovia1.prg
via1-free.prg: via1-free.tas
	dasm via1-free.tas -ovia1-free.prg
	
clean:
	rm -f *~
	rm -f *.prg
