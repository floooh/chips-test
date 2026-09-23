
charset/bitmap/background bits:

- 00 (d021, black) and 01 (d022, red) are always background (sprites are always 
  in front of it)
- 10 (d023, green) and 11 (colram, blue) are foreground. it will be in front of 
  sprites with the respective priority bit set.

sprite bits:

- 00/0  (transparent) (background or sprite with lower priority)
- 10/1  (sprite color, red/blue)
- 01    (d025, yellow) (Multicolor 1)
- 11    (d026, green) (Multicolor 2)

the priority can be made up by these two rules (this order!)

- starting from sprite with lowest number, find a non transparent (0/00) color.
  if none found, background (any) shows.
- if a non transparent sprite color was found in the previous step, and the 
  priority bit of the corresponding sprite is set, 10/11 background bits will
  show in front, else the sprite color shows.

the interesting case is when eg sprite 1 and sprite 0 overlap, and sprite 0 has
the priority bit set (and sprite 1 has not). in this case 10/11 background bits
show in front of whole sprite 0.
