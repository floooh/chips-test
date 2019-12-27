NAME = unconnected
BINARY = $(NAME).prg
SOURCE = $(NAME).asm
COMPILER = acme
CFLAGS = -v1 --cpu 6502 -f cbm -o
DEBUGGER = xvic
DEBUGFLAGS = -mem none
REMOVE = rm -f

.PHONY: all debug clean

all: $(BINARY)

debug: $(BINARY)
	$(DEBUGGER) $(DEBUGFLAGS) $(BINARY)

$(BINARY): $(SOURCE)
	$(COMPILER) $(CFLAGS) $@ $<

clean:
	-$(REMOVE) $(BINARY)
