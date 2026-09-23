
all: text26.prg stretch.prg fld.prg 3fff.prg \
	sprsync.prg lrborder.prg tech-tech.prg d011h3.prg


text26.prg: text26.asm
	acme -f cbm -o text26.prg text26.asm
stretch.prg: stretch.asm
	acme -f cbm -o stretch.prg stretch.asm
fld.prg: fld.asm
	acme -f cbm -o fld.prg fld.asm
3fff.prg: 3fff.asm
	acme -f cbm -o 3fff.prg 3fff.asm
sprsync.prg: sprsync.asm
	acme -f cbm -o sprsync.prg sprsync.asm
lrborder.prg: lrborder.asm
	acme -f cbm -o lrborder.prg lrborder.asm
tech-tech.prg: tech-tech.asm
	acme -f cbm -o tech-tech.prg tech-tech.asm
d011h3.prg: d011h3.asm
	acme -f cbm -o d011h3.prg d011h3.asm
	
clean:
	$(RM) text26.prg stretch.prg fld.prg 3fff.prg
	$(RM) sprsync.prg lrborder.prg tech-tech.prg d011h3.prg
