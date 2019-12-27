
bandits-via1.prg
bandits-via2.prg

this program produces a simple raster irq in a similar way the game "bandits"
does. whats special about it is that it relies on the interupt flag being
cleared when writing to the high latch (which the synertek notes say does NOT(!)
happen)

expected output looks like this:

--@ (first two characters are inverted)
-@@ (first character is inverted)

C0 C0 00 C0 00 00 VIA1

- confirmed on my vic20 (MOS VIAs) (gpz)
- confirmed to work the same on synertek VIA by Dirk Vroomen
