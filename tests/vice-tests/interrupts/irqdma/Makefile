
.PHONY: all clean

all: record1.prg record2.prg record3.prg \
     record4.prg record5.prg record6.prg \
     record1b.prg record2b.prg record3b.prg \
     record4b.prg record5b.prg record6b.prg \
     record7.prg record7b.prg \
     nmirecord6.prg nmirecord6b.prg \
     test1.prg test2.prg test3.prg \
     test4.prg test5.prg test6.prg \
     test1b.prg test2b.prg test3b.prg \
     test4b.prg test5b.prg test6b.prg \
     test7.prg test7b.prg \
     nmitest6.prg nmitest6b.prg

record1.prg: irqdma.asm
	acme -DTESTNUM=1 -DB_MODE=0 -DTEST_MODE=0 -f cbm -o record1.prg irqdma.asm
record2.prg: irqdma.asm
	acme -DTESTNUM=2 -DB_MODE=0 -DTEST_MODE=0 -f cbm -o record2.prg irqdma.asm
record3.prg: irqdma.asm
	acme -DTESTNUM=3 -DB_MODE=0 -DTEST_MODE=0 -f cbm -o record3.prg irqdma.asm
record4.prg: irqdma.asm
	acme -DTESTNUM=4 -DB_MODE=0 -DTEST_MODE=0 -f cbm -o record4.prg irqdma.asm
record5.prg: irqdma.asm
	acme -DTESTNUM=5 -DB_MODE=0 -DTEST_MODE=0 -f cbm -o record5.prg irqdma.asm
record6.prg: irqdma.asm
	acme -DTESTNUM=6 -DB_MODE=0 -DTEST_MODE=0 -f cbm -o record6.prg irqdma.asm
record7.prg: irqdma.asm
	acme -DTESTNUM=7 -DB_MODE=0 -DTEST_MODE=0 -f cbm -o record7.prg irqdma.asm

record1b.prg: irqdma.asm
	acme -DTESTNUM=1 -DB_MODE=1 -DTEST_MODE=0 -f cbm -o record1b.prg irqdma.asm
record2b.prg: irqdma.asm
	acme -DTESTNUM=2 -DB_MODE=1 -DTEST_MODE=0 -f cbm -o record2b.prg irqdma.asm
record3b.prg: irqdma.asm
	acme -DTESTNUM=3 -DB_MODE=1 -DTEST_MODE=0 -f cbm -o record3b.prg irqdma.asm
record4b.prg: irqdma.asm
	acme -DTESTNUM=4 -DB_MODE=1 -DTEST_MODE=0 -f cbm -o record4b.prg irqdma.asm
record5b.prg: irqdma.asm
	acme -DTESTNUM=5 -DB_MODE=1 -DTEST_MODE=0 -f cbm -o record5b.prg irqdma.asm
record6b.prg: irqdma.asm
	acme -DTESTNUM=6 -DB_MODE=1 -DTEST_MODE=0 -f cbm -o record6b.prg irqdma.asm
record7b.prg: irqdma.asm
	acme -DTESTNUM=7 -DB_MODE=1 -DTEST_MODE=0 -f cbm -o record7b.prg irqdma.asm

nmirecord6.prg: nmidma.asm
	acme -DTESTNUM=6 -DB_MODE=0 -DTEST_MODE=0 -f cbm -o nmirecord6.prg nmidma.asm

nmirecord6b.prg: nmidma.asm
	acme -DTESTNUM=6 -DB_MODE=1 -DTEST_MODE=0 -f cbm -o nmirecord6b.prg nmidma.asm


test1.prg: dumps/irq1.dump irqdma.asm
	acme -DTESTNUM=1 -DB_MODE=0 -DTEST_MODE=1 -f cbm -o test1.prg irqdma.asm
test2.prg: dumps/irq2.dump irqdma.asm
	acme -DTESTNUM=2 -DB_MODE=0 -DTEST_MODE=1 -f cbm -o test2.prg irqdma.asm
test3.prg: dumps/irq3.dump irqdma.asm
	acme -DTESTNUM=3 -DB_MODE=0 -DTEST_MODE=1 -f cbm -o test3.prg irqdma.asm
test4.prg: dumps/irq4.dump irqdma.asm
	acme -DTESTNUM=4 -DB_MODE=0 -DTEST_MODE=1 -f cbm -o test4.prg irqdma.asm
test5.prg: dumps/irq5.dump irqdma.asm
	acme -DTESTNUM=5 -DB_MODE=0 -DTEST_MODE=1 -f cbm -o test5.prg irqdma.asm
test6.prg: dumps/irq6.dump irqdma.asm
	acme -DTESTNUM=6 -DB_MODE=0 -DTEST_MODE=1 -f cbm -o test6.prg irqdma.asm
test7.prg: dumps/irq7.dump irqdma.asm
	acme -DTESTNUM=7 -DB_MODE=0 -DTEST_MODE=1 -f cbm -o test7.prg irqdma.asm

test1b.prg: dumps/irq1b.dump irqdma.asm
	acme -DTESTNUM=1 -DB_MODE=1 -DTEST_MODE=1 -f cbm -o test1b.prg irqdma.asm
test2b.prg: dumps/irq2b.dump irqdma.asm
	acme -DTESTNUM=2 -DB_MODE=1 -DTEST_MODE=1 -f cbm -o test2b.prg irqdma.asm
test3b.prg: dumps/irq3b.dump irqdma.asm
	acme -DTESTNUM=3 -DB_MODE=1 -DTEST_MODE=1 -f cbm -o test3b.prg irqdma.asm
test4b.prg: dumps/irq4b.dump irqdma.asm
	acme -DTESTNUM=4 -DB_MODE=1 -DTEST_MODE=1 -f cbm -o test4b.prg irqdma.asm
test5b.prg: dumps/irq5b.dump irqdma.asm
	acme -DTESTNUM=5 -DB_MODE=1 -DTEST_MODE=1 -f cbm -o test5b.prg irqdma.asm
test6b.prg: dumps/irq6b.dump irqdma.asm
	acme -DTESTNUM=6 -DB_MODE=1 -DTEST_MODE=1 -f cbm -o test6b.prg irqdma.asm
test7b.prg: dumps/irq7b.dump irqdma.asm
	acme -DTESTNUM=7 -DB_MODE=1 -DTEST_MODE=1 -f cbm -o test7b.prg irqdma.asm

nmitest6.prg: dumps/nmi6.dump nmidma.asm
	acme -DTESTNUM=6 -DB_MODE=0 -DTEST_MODE=1 -f cbm -o nmitest6.prg nmidma.asm

nmitest6b.prg: dumps/nmi6b.dump nmidma.asm
	acme -DTESTNUM=6 -DB_MODE=1 -DTEST_MODE=1 -f cbm -o nmitest6b.prg nmidma.asm

.PHONY: irqdma.d64

irqdma.d64:
	c1541 -format "irqdma,00" d64 irqdma.d64 \
		-write test6.prg test6 \
		-write test6b.prg test6b \
		-write test7.prg test7 \
		-write test7b.prg test7b

clean:
	rm -f *.prg
