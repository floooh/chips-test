
TESTS=\
	cia-sp-test-oneshot-old.prg \
	cia-sp-test-continues-old.prg \
	cia-icr-test-oneshot-old.prg \
	cia-icr-test-continues-old.prg \
	cia-sp-test-oneshot-new.prg \
	cia-sp-test-continues-new.prg \
	cia-icr-test-oneshot-new.prg \
	cia-icr-test-continues-new.prg \
	cia-icr-test2-oneshot.prg \
	cia-icr-test2-continues.prg

all: $(TESTS)

cia-sp-test-oneshot-old.prg: cia-sp-test.asm
	acme -DONESHOT=1 -DCIA=0 -f cbm -o cia-sp-test-oneshot-old.prg cia-sp-test.asm
cia-sp-test-continues-old.prg: cia-sp-test.asm
	acme -DONESHOT=0 -DCIA=0 -f cbm -o cia-sp-test-continues-old.prg cia-sp-test.asm

cia-icr-test-oneshot-old.prg: cia-icr-test.asm
	acme -DONESHOT=1 -DCIA=0 -f cbm -o cia-icr-test-oneshot-old.prg cia-icr-test.asm
cia-icr-test-continues-old.prg: cia-icr-test.asm
	acme -DONESHOT=0 -DCIA=0 -f cbm -o cia-icr-test-continues-old.prg cia-icr-test.asm

cia-sp-test-oneshot-new.prg: cia-sp-test.asm
	acme -DONESHOT=1 -DCIA=1 -f cbm -o cia-sp-test-oneshot-new.prg cia-sp-test.asm
cia-sp-test-continues-new.prg: cia-sp-test.asm
	acme -DONESHOT=0 -DCIA=1 -f cbm -o cia-sp-test-continues-new.prg cia-sp-test.asm

cia-icr-test-oneshot-new.prg: cia-icr-test.asm
	acme -DONESHOT=1 -DCIA=1 -f cbm -o cia-icr-test-oneshot-new.prg cia-icr-test.asm
cia-icr-test-continues-new.prg: cia-icr-test.asm
	acme -DONESHOT=0 -DCIA=1 -f cbm -o cia-icr-test-continues-new.prg cia-icr-test.asm

cia-icr-test2-oneshot.prg: cia-icr-test2.asm
	acme -DONESHOT=1 -f cbm -o cia-icr-test2-oneshot.prg cia-icr-test2.asm
cia-icr-test2-continues.prg: cia-icr-test2.asm
	acme -DONESHOT=0 -f cbm -o cia-icr-test2-continues.prg cia-icr-test2.asm

clean:
	rm -f *.prg
