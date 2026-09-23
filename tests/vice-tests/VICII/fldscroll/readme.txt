this small test reproduces the FLD picture scroller used at the beginning
of "Cataball+3/Decibel".

press A and S to shift the timing left/right by one cycle
press left SHIFT to stop/hold movement
when shift is pressed, use Q and W to scroll up/down

when the displayed hex number is in the range 21-29 the text screen should
scroll up and down via FLD. outside of this range line crunching and/or FLI 
will happen:

1C-20 top: stretched char data 
      bottom: "rolling" screen (line crunching)
21-29 top: idle gfx
      bottom: FLD
2A    top: repeated first charline, 2 black $ff chars on the right
      bottom: "normal" text screen, scrolling 0-7 pixels
2B    top: repeated first charline, 1 black $ff chars on the right
      bottom: "normal" text screen, scrolling 0-7 pixels
2C    top: repeated first charline, no black $ff char on the right
      bottom: "normal" text screen, scrolling 0-7 pixels
2D    top: repeated first charline, 1 black $ff char on the left
      bottom: "normal" text screen, scrolling 0-7 pixels
2E    top: repeated first charline, 2 black $ff char on the left
      bottom: "normal" text screen, scrolling 0-7 pixels
2F    top: repeated first charline, 3 black $ff char on the left
      bottom: "normal" text screen, scrolling 0-7 pixels
30    top: repeated first charline, 3 black $ff char on the left, flickering "E" on the right
      bottom: "normal" text screen, scrolling 0-7 pixels, shifted 1 char to the right
31    top: repeated first charline, 3 black $ff char on the left, flickering "NG" on the right
      bottom: "normal" text screen, scrolling 0-7 pixels, shifted 2 chars to the right
32    top: repeated first charline, 3 black $ff char on the left, flickering "RAN" on the right
      bottom: "normal" text screen, scrolling 0-7 pixels, shifted 3 chars to the right
33    top: repeated first charline, 3 black $ff char on the left, flickering "RA" on the right
      bottom: "normal" text screen, scrolling 0-7 pixels, shifted 4 chars to the right
34    top: repeated first charline, 3 black $ff char on the left, flickering "R" on the right
      bottom: "normal" text screen, scrolling 0-7 pixels, shifted 5 chars to the right
35-57 top: repeated first charline, 3 black $ff char on the left
      bottom: "normal" text screen, scrolling 0-7 pixels, shifted 6-40 chars to the right
58-5A top: last charline of the screen repeats here
      bottom: "normal" text screen, scrolling 0-7 pixels

5B-   repeats as at 1C-
