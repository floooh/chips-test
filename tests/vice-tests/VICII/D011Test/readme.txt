
various small interactive tests written by andreas boose, extracted from D011test.d64:

TODO: document what these do exactly
TODO: fix/update tests (eg some rely on keyboard repeat enabled)
TODO: make non interactive versions if possible

d011h1:

d011h2:

d011n:

d011ntsc:

d011sc:

d011v1:

d011v2:

d011ver:

d011vt:

"F7 gedrückt halten, bis der Scroll einsetzt
F7 wiederholt drücken, bis das rechte der gelben F den rechten Rand erreicht hat 
und damit alle G aus der F Reihe verschwunden sind.
Jetzt ist cycle=54 und damit der untere Codezweig aktiv.
5 mal cursor down, damit der $ff cursor auf der Doppelreihe E steht.
Die Linie zwischen den beiden E ist der idle state, d.h auch der cursor ist 
durch diese Linie getrennt.
39 mal cursor right, damit der cursor ganz rechts steht.

Und hier sehen wir den Fehler: Der obere und untere cursor berührt sich dieser 
Stelle, da schon in diesem Zyklus der display state eingeschaltet wird. Das ist 
bei meinem C64 nicht der Fall, der ist in diesem Zyklus noch im idle state. Beim 
C64 kann ich mit F5 einen Zyklus zurückschalten und an dieser Stelle wird aus 
dem idle state der display state. Damit ist bewiesen, dass bei VICE an dieser 
Stelle der display state genau einen Zyklus zu früh eingeschaltet wird."

hh:

hh3:

s1:

vicnt2:

--------------------------------------------------------------------------------

disable-bad:
Exposes a VICE bug also visible in demo "angle +1".
A badline is forced in cycle n and then immidiately disabled where n is in {54,10}.
The first line should display normal A where the B in the second line has has almost
double height.
