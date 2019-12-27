
    0 forn=0to121:reada:poke820+n,a:next
    1 print"{clr}interlace-mode (y/n)":poke36864,5
    2 geta$:ifa$<>"y"anda$<>"n"then2
    3 ifa$="y"thenpoke36864,peek(36864)or128
   20 print"{clr}{down}{blk}{rvon} {rvof} {rvon} {rvof} {rvon} {rvof} {rvon} {rvof} {rvon} {rvof} {rvon} {rvof} {rvon} {rvof} {rvon} {rvof} {rvon} {rvof} {rvon} {rvof} {rvon} {rvof}"
   30 print"     11111222223333344";
   40 print"0246802468024680246802"
   45 print"interlace mode o";:ifa$="n"thenprint"ff":print:goto52
   46 print"n":print:poke930,146
   50 fori=0to131:print"{up}$9004:    {left}{left}{left}{left}"i:poke834,i:poke822,131-i:sys820:next:goto50
   52 fori=0to130:print"{up}$9004:    {left}{left}{left}{left}"i:poke834,i:sys828:next:goto52
  100 data 120,169,131,205,4,144,208,249
  110 data 120,160,12,162,54,169,0,205
  120 data 4,144,208,251,234,234,234,234
  130 data 234,234,234,234,234,234,234,234
  140 data 234,234,234,234,234,234,234,234
  150 data 234,234,234,234,234,234,234,234
  160 data 234,234,202,208,223,234,234,234
  170 data 234,234,234,234,234,234,234,234
  180 data 234,234,234,234,234,234,234,234
  190 data 36,36,169,27,140,15,144,141
  200 data 15,144,173,255,31,240,3,10
  210 data 144,2,73,29,141,255,31,170
  220 data 202,208,253,169,239,141,32,145
  230 data 173,33,145,201,254,208,156,169
  240 data 247,141,32,145,88,169,0,133
  250 data 198,96
