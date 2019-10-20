# Makefile

all:	cia-int-irq.prg cia-int-nmi.prg cia-int-irq-new.prg cia-int-nmi-new.prg

cia-int-irq.prg: cia-int.asm
	dasm $< -o$@ -DTEST_IRQ -DNEWCIA=0
cia-int-irq-new.prg: cia-int.asm
	dasm $< -o$@ -DTEST_IRQ -DNEWCIA=1

cia-int-nmi.prg: cia-int.asm
	dasm $< -o$@ -DTEST_NMI -DNEWCIA=0
cia-int-nmi-new.prg: cia-int.asm
	dasm $< -o$@ -DTEST_NMI -DNEWCIA=1

clean:
	rm -f *~ \#*\#
	rm -f a.out
	rm -f cia-int-irq.prg cia-int-nmi.prg
	rm -f cia-int-irq-new.prg cia-int-nmi-new.prg

# eof
