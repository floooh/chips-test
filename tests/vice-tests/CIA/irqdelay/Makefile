
all:
	cl65 -C c64-asm.cfg -u __EXEHDR__ -o irqdelay.prg irqdelay.s
	cl65 -C c64-asm.cfg -u __EXEHDR__ --asm-define ONESHOT -o irqdelay-oneshot.prg irqdelay.s
	cl65 -C c64-asm.cfg -u __EXEHDR__ --asm-define TESTNEW -o irqdelay-new.prg irqdelay.s
	cl65 -C c64-asm.cfg -u __EXEHDR__ --asm-define TESTNEW --asm-define ONESHOT -o irqdelay-oneshot-new.prg irqdelay.s
	
	cl65 -C c64-asm.cfg -u __EXEHDR__ -o irqdelay2.prg irqdelay2.s
	cl65 -C c64-asm.cfg -u __EXEHDR__ --asm-define TESTNEW -o irqdelay2-new.prg irqdelay2.s

	cl65 -C c64-asm.cfg -u __EXEHDR__ --asm-define CIA=0 --asm-define TESTVAL=4 --asm-define ONESHOT -o irqdelay-cia1-oneshot-4-old.prg irqdelay-cia1.s
	cl65 -C c64-asm.cfg -u __EXEHDR__ --asm-define CIA=1 --asm-define TESTVAL=4 --asm-define ONESHOT -o irqdelay-cia1-oneshot-4-new.prg irqdelay-cia1.s
	cl65 -C c64-asm.cfg -u __EXEHDR__ --asm-define CIA=0 --asm-define TESTVAL=4 -o irqdelay-cia1-4-old.prg irqdelay-cia1.s
	cl65 -C c64-asm.cfg -u __EXEHDR__ --asm-define CIA=1 --asm-define TESTVAL=4 -o irqdelay-cia1-4-new.prg irqdelay-cia1.s

	cl65 -C c64-asm.cfg -u __EXEHDR__ --asm-define CIA=0 --asm-define TESTVAL=255 -o irqdelay-cia1.prg irqdelay-cia1.s
	cl65 -C c64-asm.cfg -u __EXEHDR__ --asm-define CIA=0 --asm-define TESTVAL=255 --asm-define ONESHOT -o irqdelay-cia1-oneshot.prg irqdelay-cia1.s

	cl65 -C c64-asm.cfg -u __EXEHDR__ --asm-define CIA=0 --asm-define TESTVAL=255 -o irqdelay-cia2.prg irqdelay-cia2.s
	cl65 -C c64-asm.cfg -u __EXEHDR__ --asm-define CIA=0 --asm-define TESTVAL=5 -o irqdelay-cia2-4.prg irqdelay-cia2.s
	cl65 -C c64-asm.cfg -u __EXEHDR__ --asm-define CIA=0 --asm-define TESTVAL=255 --asm-define ONESHOT -o irqdelay-cia2-oneshot.prg irqdelay-cia2.s
	cl65 -C c64-asm.cfg -u __EXEHDR__ --asm-define CIA=0 --asm-define TESTVAL=4 --asm-define ONESHOT -o irqdelay-cia2-oneshot-4.prg irqdelay-cia2.s

#	cl65 -o irqdelay-cia1-scan.prg irqdelay-cia1-scan.s
#	cl65 --asm-define ONESHOT -o irqdelay-cia1-scan-oneshot.prg irqdelay-cia1-scan.s

clean:
	rm -f *.prg
	rm -f *.o
	rm -f *~
