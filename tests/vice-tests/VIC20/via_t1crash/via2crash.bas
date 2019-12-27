0 via = 37152 : rem $9120
1 print "{clr}via2 - set timers to 0":poke via+14,64:
2 print "$9128  t2l   ";:poke via+8,0:gosub 100:print "ok"
3 print "$9129  t2h   ";:poke via+9,0:gosub 100:print "ok"
4 print "$9124  t1l   ";:poke via+4,0:gosub 100:print "ok"
5 print "$9126  t1ll  ";:poke via+6,0:gosub 100:print "ok"
6 print "$9125  t1h   ";:poke via+5,0:gosub 100:print "ok"
7 print "$9127  t1lhh ";:poke via+7,0:gosub 100:print "ok"
10 print "all ok."
20 poke 36879,5: rem green border
30 poke 37135,0: rem success
40 poke via+14,128+64
99 end
100 for i = 0 to 1000:next:return:rem wait a bit
