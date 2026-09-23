when a new badline is forced by writing values to $d011 so the badline condition
matches every line, the first 3 characters do not get proper color information.

blackmail.prg:

display routine from "Blackmail FLI Designer 2.2" - which allows to use some
additional colors in the first 3 characters for the "11" pixels, by using
various illegal opcodes in the display routine which leave different values on
the otherwise floating bus. "01" and "10" pixels will remain light grey ($0f)

the original displayer relies on the "magic constant" of the LAX #imm opcode
($AB) being all 1s in the lowest 3 bits. fortunately the displayer can be 
trivially patched to not rely on this behaviour (see STABLEFIX).
