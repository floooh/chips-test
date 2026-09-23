
all: main.prg main2.prg main3.prg main4.prg bitmap.prg

main.prg: main.asm
	acme --cpu 6510 -f cbm -o main.prg main.asm
main2.prg: main2.asm
	acme --cpu 6510 -f cbm -o main2.prg main2.asm
main3.prg: main3.asm
	acme --cpu 6510 -f cbm -o main3.prg main3.asm
main4.prg: main4.asm
	acme --cpu 6510 -f cbm -o main4.prg main4.asm
bitmap.prg: bitmap.asm
	acme --cpu 6510 -f cbm -o bitmap.prg bitmap.asm
	
test: main.prg
	x64sc -default main.prg
test2: main2.prg
	x64sc -default main2.prg
test3: main3.prg
	x64sc -default main3.prg
test4: main4.prg
	x64sc -default main4.prg
test5: bitmap.prg
	x64sc -default bitmap.prg

clean:
	$(RM) *.prg
