1 poke 37139,0: poke 37154,0
2 s1=peek(37137): s2=peek(37152)
3 poke 37139,255  : poke 37154,255
4 s3=peek(37137): s4=peek(37152)
5 poke 37139,0:poke 37154,255
10 print s1;s2;s3;s4
100 run
