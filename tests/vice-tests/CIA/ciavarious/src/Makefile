
PROGS = \
	cia1.prg \
	cia2.prg \
	cia3.prg \
	cia3a.prg \
	cia4.prg \
	cia5.prg \
	cia6.prg \
	cia7.prg \
	cia8.prg \
	cia9.prg \
	cia10.prg \
	cia11.prg \
	cia12.prg \
	cia13.prg \
	cia14.prg \
	cia15.prg

all: $(PROGS)

%.prg:%.a65 common.a65
	xa -o $*.prg $*.a65

clean:
	rm -f *~
	rm -f *.prg
	