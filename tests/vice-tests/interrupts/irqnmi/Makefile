
.SILENT:

all: tests

tests:
	cl65 -C c64-asm.cfg -u __EXEHDR__ --asm-define CIAOLD=1 -o irqnmi-old.prg test1.s
	cl65 -C c64-asm.cfg -u __EXEHDR__ --asm-define CIAOLD=0 -o irqnmi-new.prg test1.s
	cl65 -t none --asm-define expanded=0 --asm-define SHOW=0 -o irqnmi-vic20irq.prg test1vic20.s
	cl65 -t none --asm-define expanded=0 --asm-define SHOW=1 -o irqnmi-vic20nmi.prg test1vic20.s
	cl65 -t none --asm-define expanded=1 --asm-define SHOW=0 -o irqnmi-vic20irq-8k.prg test1vic20.s
	cl65 -t none --asm-define expanded=1 --asm-define SHOW=1 -o irqnmi-vic20nmi-8k.prg test1vic20.s
	rm -f *.o
	
disk: tests
	c1541 -format "test,00" d64 irqnmi.d64 \
		-write irqnmi-old.prg old \
		-write irqnmi-old.prg new \
		-write irqnmi-vic20.prg vic20irq \
		-write irqnmi-vic20.prg vic20nmi \
		-write irqnmi-vic20-8k.prg vic20irq \
		-write irqnmi-vic20-8k.prg vic20nmi

clean:
	rm -f *~
	rm -f *.prg
	rm -f *.o
