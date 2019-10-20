irqdummy (w) 2010 Hannu Nuotio

Test code based on:   http://bel.fi/~alankila/irqack/
...which is a fixed version of Freshness79's IRQ test in:
http://noname.c64.org/csdb/forums/?roomid=11&topicid=79331&showallposts=1

Checks if interrupt sequence dummy loads are from PC only (not PC+1)

Border color changes to violet when test is started.

When test is done, border color changes to green (success) or flashes (failure)
