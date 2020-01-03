
TARGETS = ramtest.prg banktest.prg

all: $(TARGETS)

clean:
	rm -f $(TARGETS)

.phony: all clean
.SUFFIXES: .s .prg

.s.prg:
	xa -o $@ $<

