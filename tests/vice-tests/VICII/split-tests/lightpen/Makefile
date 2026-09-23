# Makefile

all:	lightpen.prg

lightpen.prg: lightpen.asm ../common/startup.asm ../common/scandump.asm \
	dumps/dump6567.bin dumps/dump8562r4.bin dumps/dump8564.bin \
	dumps/dump8564r5.bin \
	dumps/dump6569.bin dumps/dump6569r1.bin dumps/dump8565.bin \
	dumps/dump6572.bin \
	dumps/dumpdtv3.bin
	dasm $< -o$@

dumps/dump6567.bin:	dumps/dump6567.prg makeref
	./makeref $< $@

dumps/dump8562r4.bin:	dumps/dump8562r4.prg makeref
	./makeref $< $@

dumps/dump8564.bin:	dumps/dump8564.prg makeref
	./makeref $< $@

dumps/dump8564r5.bin:	dumps/dump8564r5.prg makeref
	./makeref $< $@

dumps/dump6569.bin:	dumps/dump6569.prg makeref
	./makeref $< $@

dumps/dump6569r1.bin:	dumps/dump6569r1.prg makeref
	./makeref $< $@

dumps/dump8565.bin:	dumps/dump8565.prg makeref
	./makeref $< $@

dumps/dump6572.bin:	dumps/dump6572.prg makeref
	./makeref $< $@

dumps/dumpdtv3.bin:	dumps/dumpdtv3.prg makeref
	./makeref $< $@

makeref: makeref.c
	gcc -Wall -o $@ $<

analyse: analyse.c
	gcc -Wall -o $@ $<

clean:
	rm -f *~ \#*\#
	rm -f a.out
	rm -f dumps/*.bin
	rm -f makeref analyse
	rm -f lightpen.prg

# eof
