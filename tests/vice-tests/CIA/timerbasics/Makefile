
all: test.prg timer.prg timer_test1.prg test_new.prg timer_new.prg timer_test1_new.prg

timer.prg: timer_old.s
	acme -f cbm -o timer.prg timer_old.s
timer_new.prg: timer_new.s
	acme -f cbm -o timer_new.prg  timer_new.s

timer_test1.prg: timer_test1.s timer_test1_old.s
	acme -f cbm -o timer_test1.prg timer_test1_old.s
timer_test1_new.prg:  timer_test1.s timer_test1_new.s
	acme -f cbm -o timer_test1_new.prg timer_test1_new.s

test.prg: test_old.s test.s
	acme -f cbm -o test.prg test_old.s
test_new.prg: test_new.s test.s
	acme -f cbm -o test_new.prg test_new.s

clean:
	rm -f *~
	rm -f *.prg
