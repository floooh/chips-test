ASSEMBLER = acme
EMU = xvic
EMUFLAGS = -mem 8k


all: bugvicevia1.prg bugvicevia2.prg

bugvicevia1.prg labels1.tlb: bugvicevia.asm
	$(ASSEMBLER) -f cbm -v1 --cpu 6502 -DTESTVIA=1 -o $@ $<

bugvicevia2.prg labels2.tlb: bugvicevia.asm
	$(ASSEMBLER) -f cbm -v1 --cpu 6502 -DTESTVIA=2 -o $@ $<

clean:
	rm -f bugvicevia1.prg bugvicevia2.prg
