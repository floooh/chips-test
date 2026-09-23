 
all: vicii_reg_timing.prg vicii_reg_timing-a5.prg vicii_reg_timing-ff.prg

vicii_reg_timing.prg: vicii_reg_timing.asm
#	cl65 -v -vm -m vicii_reg_timing.map -l -t none -C vicii_reg_timing.link vicii_reg_timing.asm -o vicii_reg_timing.prg
	cl65 --asm-define BMVALUE=0x5a -t none -C vicii_reg_timing.link vicii_reg_timing.asm -o vicii_reg_timing.prg
	rm vicii_reg_timing.o

vicii_reg_timing-a5.prg: vicii_reg_timing.asm
	cl65 --asm-define BMVALUE=0xa5 -t none -C vicii_reg_timing.link vicii_reg_timing.asm -o vicii_reg_timing-a5.prg
	rm vicii_reg_timing.o
	
vicii_reg_timing-ff.prg: vicii_reg_timing.asm
	cl65 --asm-define BMVALUE=0xff -t none -C vicii_reg_timing.link vicii_reg_timing.asm -o vicii_reg_timing-ff.prg
	rm vicii_reg_timing.o
	
clean:
	rm -f vicii_reg_timing.prg
	rm -f vicii_reg_timing.lst
	rm -f vicii_reg_timing.map
	rm -f vicii_reg_timing-a5.prg
	rm -f vicii_reg_timing-ff.prg
	rm -f *.o
