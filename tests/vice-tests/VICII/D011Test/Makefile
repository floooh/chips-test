
TESTS=\
	d011vt.prg d011h1.prg d011h2.prg d011n.prg \
	d011ntsc.prg d011sc.prg d011v1.prg d011v2.prg \
	d011ver.prg hh.prg hh3.prg s1.prg vicnt2.prg \
	disable-bad.prg disable-bad_ntsc.prg disable-bad_ntscold.prg

all: $(TESTS)
	
disable-bad.prg: disable-bad.tas
	acme -f cbm -DCYCLES=63 -o $@ $<
disable-bad_ntsc.prg: disable-bad.tas
	acme -f cbm -DCYCLES=65 -o $@ $<
disable-bad_ntscold.prg: disable-bad.tas
	acme -f cbm -DCYCLES=64 -o $@ $<

d011vt.prg: d011vt.asm
	acme -f cbm -o d011vt.prg d011vt.asm
d011h1.prg: d011h1.asm
	acme -f cbm -o d011h1.prg d011h1.asm
d011h2.prg: d011h2.asm
	acme -f cbm -o d011h2.prg d011h2.asm
d011n.prg: d011n.asm
	acme -f cbm -o d011n.prg d011n.asm
d011ntsc.prg: d011ntsc.asm
	acme -f cbm -o d011ntsc.prg d011ntsc.asm
d011sc.prg: d011sc.asm
	acme -f cbm -o d011sc.prg d011sc.asm
d011v1.prg: d011v1.asm
	acme -f cbm -o d011v1.prg d011v1.asm
d011v2.prg: d011v2.asm
	acme -f cbm -o d011v2.prg d011v2.asm
d011ver.prg: d011ver.asm
	acme -f cbm -o d011ver.prg d011ver.asm
hh.prg: hh.asm
	acme -f cbm -o hh.prg hh.asm
hh3.prg: hh3.asm
	acme -f cbm -o hh3.prg hh3.asm
s1.prg: s1.asm
	acme -f cbm -o s1.prg s1.asm
vicnt2.prg: vicnt2.asm
	acme -f cbm -o vicnt2.prg vicnt2.asm

clean:
	$(RM) $(TESTS)	
