
this program shows sprite fetch bugs in sideborder as described in this bug 
report: https://sourceforge.net/p/vice-emu/bugs/217/ (and is based on the 
program posted there)

Use keys <- and 1 to move sprites, keys 2 and CTRL change number of lines to 
open sideborder, space resets to defaults.

"move sprites until the first number reads 136. On real C64, C128 and x64 the 
 sprite bug is no longer multicolor, however that's not the case on x64sc."

"Interestingly I cannot reproduce this on my x64 (6569R3). The bug is multicolor 
 on my c64 exactly like in x64sc. I do not have a c64c (856x) based machine here 
 but there may be a difference between those." (tlr)

"The bug can be reproduced on a C64C. Values of 136 and 13e (and 146, 14e, ...) 
 show the "not multicolor" bug. ... and 13a and 142" (amatthies)

TODO: more testing, collect results and relate to chip revision(s)
