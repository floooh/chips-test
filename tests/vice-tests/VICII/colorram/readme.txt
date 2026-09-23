
quick colorram access check:

- in first page of the screen a test pattern is generated, which cycles through
  all colors and puts the same value into colorram and corrosponding screen
  ram.
- then the first page of color memory is read back, shown verbatim in the
  second page of the screen. also shown in the third page of the screen, ANDed
  with $f so you only see the low nibble.

the third page of the screen should show the @ABCDEFGHIJKLMNO pattern also shown
by the first page.

border turns green when the values read back are as expected, and red on failure
