  500 rem  "ciatest64"  for 64 mode only.
  510 rem  * = interrupt flag error.
  520 rem  reset after test.
  530 rem
  540 n=12800:fori=nton+103:reada:pokei,a:z=z+a
  550 next:ifz<>11949thenprint"data error":end
  560 sys65412:x=notx:poke251,xand255
  570 printchr$(147);"any key switches timer."
  580 print"testing timer ";chr$(65-x):sysn
  590 wait198,7:poke198,0:goto560
  600 data 169,  84, 162,  98,  36, 251,  48,   3
  610 data 170, 169,  98, 160,   3, 141,   4, 221
  620 data 140,   5, 221, 142,   6, 221, 140,   7
  630 data 221, 169,  17, 141,  14, 221, 141,  15
  640 data 221, 162,   2, 160,   7,  36, 251,  48
  650 data   3, 202, 160,   5, 134, 252, 140,  77
  660 data  50, 140,  85,  50, 138,  73, 131, 162
  670 data  72, 160,  50, 142,  24,   3, 140,  25
  680 data   3, 174,  13, 221, 141,  13, 221,  96
  690 data  72, 138,  72, 152, 172,   7, 221,  72
  700 data 173,  13, 221, 216, 204,   7, 221, 176
  710 data  12,  13,  13, 221,  37, 252, 208,   5
  720 data 169,  42,  32, 210, 255,  76, 188, 254

