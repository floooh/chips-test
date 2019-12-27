all: joystick.prg joystick-8k.prg joycheck.prg

joystick.prg: main.asm
	acme -Dexpanded=0 -f cbm -o joystick.prg main.asm
joystick-8k.prg: main.asm
	acme -Dexpanded=1 -f cbm -o joystick-8k.prg main.asm
	
joycheck.prg: joycheck.bas
	petcat -w2 -o joycheck.prg < joycheck.bas
