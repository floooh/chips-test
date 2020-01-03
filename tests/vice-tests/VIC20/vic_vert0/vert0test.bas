   10 poke56,28:clr:poke648,28:sys58648:rem screen ram @ $1c00
   15 ifpeek(60901)=38thenpal=1
   20 poke36865,0:rem vertical pos=0
   25 s=506
   30 a=160:gosub998
   40 a=32:gosub998
   50 a=102:gosub998
   51 ifpalthenpoke36867,80:s=836:goto53
   52 poke36867,66:s=704:rem ntsc
   53 a=160:gosub998
   54 a=32:gosub998
   55 a=102:gosub998
   59 poke36867,46
   60 poke36865,peek(60901):rem reset to original rom position. works on pal & ntsc
   70 poke36878,46:end
  998 fori=0to21:poke7168+s+i,a:poke37888+s+i,0:next:rem poke extra line
  999 poke198,0:wait198,1:return

