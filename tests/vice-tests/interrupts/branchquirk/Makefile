
all: branchquirk-old.prg branchquirk-new.prg \
	 branchquirk-nmiold.prg branchquirk-nminew.prg \
	 dumpold.prg dumpnew.prg

branchquirk-old.prg: branchquirk.asm
	acme -f cbm -DCIA=0 -DNMI=0 -o branchquirk-old.prg branchquirk.asm
branchquirk-new.prg: branchquirk.asm
	acme -f cbm -DCIA=1 -DNMI=0 -o branchquirk-new.prg branchquirk.asm
branchquirk-nmiold.prg: branchquirk.asm
	acme -f cbm -DCIA=0 -DNMI=1 -o branchquirk-nmiold.prg branchquirk.asm
branchquirk-nminew.prg: branchquirk.asm
	acme -f cbm -DCIA=1 -DNMI=1 -o branchquirk-nminew.prg branchquirk.asm

clean:
	$(RM) branchquirk-old.prg
	$(RM) branchquirk-new.prg
	$(RM) branchquirk-nmiold.prg
	$(RM) branchquirk-nminew.prg

