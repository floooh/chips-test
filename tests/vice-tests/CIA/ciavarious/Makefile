
all: build pack disk

.PHONY: pack disk build

build:
	make -C src all

pack: build
	exomizer sfx basic -o cia1.prg src/cia1.prg src/cia1data
	exomizer sfx basic -o cia2.prg src/cia2.prg src/cia2data
	exomizer sfx basic -o cia3.prg src/cia3.prg src/cia3data
	exomizer sfx basic -o cia3a.prg src/cia3a.prg src/cia3adata
	exomizer sfx basic -o cia4.prg src/cia4.prg src/cia4data
	exomizer sfx basic -o cia5.prg src/cia5.prg src/cia5data
	exomizer sfx basic -o cia6.prg src/cia6.prg src/cia6data
	exomizer sfx basic -o cia7.prg src/cia7.prg src/cia7data
	exomizer sfx basic -o cia8.prg src/cia8.prg src/cia8data
	exomizer sfx basic -o cia9.prg src/cia9.prg src/cia9data
	exomizer sfx basic -o cia10.prg src/cia10.prg src/cia10data
	exomizer sfx basic -o cia11.prg src/cia11.prg src/cia11data
	exomizer sfx basic -o cia12.prg src/cia12.prg src/cia12data
	exomizer sfx basic -o cia13.prg src/cia13.prg src/cia13data
	exomizer sfx basic -o cia14.prg src/cia14.prg src/cia14data
	exomizer sfx basic -o cia15.prg src/cia15.prg src/cia15data

	exomizer sfx basic -o cia3new.prg src/cia3.prg src/cia3data-new
	exomizer sfx basic -o cia3anew.prg src/cia3a.prg src/cia3adata-new
	exomizer sfx basic -o cia4new.prg src/cia4.prg src/cia4data-new
	exomizer sfx basic -o cia8new.prg src/cia8.prg src/cia8data-new
	
disk: pack
	c1541 -format "ciavarious,00" d64 cia.d64 \
		-write cia1.prg cia1 \
		-write cia2.prg cia2 \
		-write cia3.prg cia3 \
		-write cia3a.prg cia3a \
		-write cia4.prg cia4 \
		-write cia5.prg cia5 \
		-write cia6.prg cia6 \
		-write cia7.prg cia7 \
		-write cia8.prg cia8 \
		-write cia9.prg cia9 \
		-write cia10.prg cia10 \
		-write cia11.prg cia11 \
		-write cia12.prg cia12 \
		-write cia13.prg cia13 \
		-write cia14.prg cia14 \
		-write cia15.prg cia15

	c1541 -format "ciavarious,00" d64 cianew.d64 \
		-write cia1.prg cia1 \
		-write cia2.prg cia2 \
		-write cia3new.prg cia3 \
		-write cia3anew.prg cia3a \
		-write cia4new.prg cia4 \
		-write cia5.prg cia5 \
		-write cia6.prg cia6 \
		-write cia7.prg cia7 \
		-write cia8new.prg cia8 \
		-write cia9.prg cia9 \
		-write cia10.prg cia10 \
		-write cia11.prg cia11 \
		-write cia12.prg cia12 \
		-write cia13.prg cia13 \
		-write cia14.prg cia14 \
		-write cia15.prg cia15

clean:
	rm -f *~
	make -C src clean
