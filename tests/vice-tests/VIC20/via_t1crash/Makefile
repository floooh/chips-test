
all: via1crash.prg via2crash.prg

via1crash.prg: via1crash.bas
	petcat -l 1201 -w2 -o via1crash.prg -- via1crash.bas
via2crash.prg: via2crash.bas
	petcat -l 1201 -w2 -o via2crash.prg -- via2crash.bas

clean:
	rm -f via1crash.prg via2crash.prg
