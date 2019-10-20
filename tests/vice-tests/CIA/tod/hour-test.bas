    0 rem *********************************
    7 rem * original tod-test program
    8 rem * by paul foerster
    9 rem *********************************

   10 cs=1
   20 c(1)=56320
   30 c(2)=56576
   40 for i=0 to 21
   50 read p
   60 poke 8192+i,p
   70 next
   80 def fn b(x)=int(x/10)*16+x-int(x/10)*10

   90 print chr$(14);chr$(147);
  110 poke 646,1:print "24hr: Wr => Rd    => Nx     (CIA#";cs;")";:poke 646,14

  130 for i=0 to 23
  140 rem  if i>2 and i<11 or i>14 and i<23 then next

  150 poke 8193,fn b(i):sys 8192:h=peek(2)
  154 er=h
  155 if (i=0) then er=er and 127
  156 if (i=12) then er=er or 128
  160 ni=i+1:if ni=24 then ni=0
  170 poke 8193,fn b(ni):sys 8192:eh=peek(2)
  175 if (ni=0) then eh=eh and 127
  176 if (ni=12) then eh=eh or 128
  180 gosub 210
  190 next

  198 if f=1 then poke 53280,10:poke 55295,255: rem failure
  199 if f=0 then poke 53280,13:poke 55295,0: rem success
  200 goto 200

  210 print:if i<10 then print " ";
  220 print i;": ";
  230 x=h
  240 gosub 400

  250 print " => ";
  260 poke c(cs)+11,h
  270 poke c(cs)+10,fn b(59)
  280 poke c(cs)+9,fn b(59)
  290 poke c(cs)+8,fn b(08)
  300 x=peek(c(cs)+8)
  310 x=peek(c(cs)+11)
  315 poke 646,13:if x<>er then poke 646,10:f=1
  320 gosub 400
  325 poke 646,14:print " ";: x=er: gosub 400

  330 print " => ";
  340 y=peek(c(cs)+8)
  350 if x=peek(c(cs)+11) then 340
  360 x=peek(c(cs)+11)
  365 poke 646,13:if x<>eh then poke 646,10:f=1
  370 gosub 400
  375 poke 646,14:print " ";: x=eh: gosub 400
  390 return

  400 y=int(x/16)
  410 gosub 430
  420 y=x-y*16
  430 print chr$(y+48);
  440 return
  450 data 169, 35, 208, 4, 169, 146
  460 data 208, 10, 248, 201, 19, 144
  470 data 5, 56, 233, 18, 9, 128, 216
  480 data 133, 2, 96
