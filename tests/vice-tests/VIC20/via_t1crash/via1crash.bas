0 print "{clr}via1 - set timers to 0"
1 print "$9118  t2l   ";:poke 37140+4,0:gosub 100:print "ok"
2 print "$9119  t2h   ";:poke 37140+5,0:gosub 100:print "ok"
3 print "$9116  t1ll  ";:poke 37140+2,0:gosub 100:print "ok"
4 print "$9117  t1lhh ";:poke 37140+3,0:gosub 100:print "ok"
5 print "$9114  t1l   ";:poke 37140+0,0:gosub 100:print "ok"
6 print "$9115  t1h   ";:poke 37140+1,0:gosub 100:print "ok"
10 print "all ok."
20 poke 36879,5: rem green border
30 poke 37135,0: rem success
99 end
100 for i = 0 to 1000:next:return:rem wait a bit
