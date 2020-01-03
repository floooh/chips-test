; ** Realtime sample & display of POTX or POTY on VIC-20,
;    written 2018-01-05 by Michael Kircher
;
;    - sample rate 512 us (^= paddle period), done in NMI,
;    - display height 256 pixels, *full* resolution,
;    - display width 208 pixels, equiv. ~= 100 ms,
;      i.e. ~5 x 20 ms in PAL, ~6 x 16,67 ms in NTSC.
;
;    Note: NTSC machines only show a cropped display.

zp_X    = $03
zp_X2   = $04
zp_scrn = $FB
zp_char = $FD

        * = $2000

Main:
        JSR $E518
        LDX #6
Main_00:
        CLC
        LDA $EDE4 - 1,X
        ADC offsets - 1,X
        STA $9000 - 1,X
        DEX
        BNE Main_00
        STX zp_X2

        STX $17F8
        STX $17F9
        STX $17FA
        STX $17FB
        STX $17FC
        STX $17FD
        STX $17FE
        STX $17FF

        LDA #$06
Main_01:
        STA $9400,X
        STA $9500,X
        STA $9600,X
        STA $9700,X
        INX
        BNE Main_01
        SEI
        LDA #$FF
        STA $9122
        LDA #$00
        STA $9123
        LDA #$F7
        STA $9120
        LDA #$7F
        STA $911E
        STA $911D
        STA $912E
        STA $912D
        LDA #$40
        STA $911B
        LDX #<NMI
        LDY #>NMI
        STX $0318
        STY $0319
        LDX #<(512-2)
        LDY #>(512-2)
        STX $9114
        STY $9115
        LDA #$C2
        STA $911E
Main_02:
        LDX #0
        STX zp_X
Main_03:
        LDX #8
Main_04:
        CPX zp_X2
        BNE Main_04
Main_05:
        LDA buffer1-1,X
        STA buffer2-1,X
        DEX
        BNE Main_05
        STX zp_X2
        JSR ClrScrn
        JSR ClrChar
        JSR Plot
        LDX zp_X
        INX
        CPX #26
        BEQ Main_02
        STX zp_X
        BNE Main_03

NMI:
        PHA
        TXA
        PHA
        BIT $911D
        BPL NMI_00
        BVC NMI_01
        LDX zp_X2
        CPX #8
        BEQ NMI_00
        LDA $9009
        EOR #$FF
        STA buffer1,X
        INX
        STX zp_X2
NMI_00:
        PLA
        TAX
        PLA
        BIT $9114
        BIT $9111
        RTI
NMI_01:
        BIT $9111
        LDA $9121
        AND #$01
        BNE NMI_00
        LDA #$7F
        STA $911E
        STA $911D
        LDA #$FE
        STA $9114
        LDA #$FF
        STA $9115
        JMP $FED2

ClrScrn:
        LDA #$FF
        LDX zp_X
        STA $1800+ 0*26,X
        STA $1800+ 1*26,X
        STA $1800+ 2*26,X
        STA $1800+ 3*26,X
        STA $1800+ 4*26,X
        STA $1800+ 5*26,X
        STA $1800+ 6*26,X
        STA $1800+ 7*26,X
        STA $1800+ 8*26,X
        STA $1800+ 9*26,X
        STA $1800+10*26,X
        STA $1800+11*26,X
        STA $1800+12*26,X
        STA $1800+13*26,X
        STA $1800+14*26,X
        STA $1800+15*26,X
        STA $1800+16*26,X
        STA $1800+17*26,X
        STA $1800+18*26,X
        STA $1800+19*26,X
        STA $1800+20*26,X
        STA $1800+21*26,X
        STA $1800+22*26,X
        STA $1800+23*26,X
        STA $1800+24*26,X
        STA $1800+25*26,X
        STA $1800+26*26,X
        STA $1800+27*26,X
        STA $1800+28*26,X
        STA $1800+29*26,X
        STA $1800+30*26,X
        STA $1800+31*26,X
        RTS

ClrChar:
        LDA #$02
        STA zp_char+1
        LDX zp_X
        LDA char_base,X
        ASL
        ROL zp_char+1
        ASL
        ROL zp_char+1
        ASL
        ROL zp_char+1
        STA zp_char
        LDA #0
        TAY
        LDX #8
ClrChar_00:
        STA (zp_char),Y
        INY
        STA (zp_char),Y
        INY
        STA (zp_char),Y
        INY
        STA (zp_char),Y
        INY
        STA (zp_char),Y
        INY
        STA (zp_char),Y
        INY
        STA (zp_char),Y
        INY
        STA (zp_char),Y
        INY
        DEX
        BNE ClrChar_00
        RTS

Plot:
        LDX #0
Plot_00:
        LDA buffer2,X
        LSR
        LSR
        LSR
        TAY
        LDA line_low,Y 
        STA zp_scrn
        LDA line_high,Y
        STA zp_scrn+1
        LDA #$02
        STA zp_char+1
        LDY zp_X
        LDA (zp_scrn),Y
        CMP #$FF
        BNE Plot_01
        TXA
        ORA char_base,Y
        STA (zp_scrn),Y
Plot_01:
        ASL
        ROL zp_char+1
        ASL
        ROL zp_char+1
        ASL
        ROL zp_char+1
        STA zp_char
        LDA buffer2,X
        AND #$07
        TAY
        LDA (zp_char),Y
        ORA $8268,X
        STA (zp_char),Y
        INX
        CPX #8
        BNE Plot_00
        RTS

line_low:
        !byte <($1800+ 0*26)
        !byte <($1800+ 1*26)
        !byte <($1800+ 2*26)
        !byte <($1800+ 3*26)
        !byte <($1800+ 4*26)
        !byte <($1800+ 5*26)
        !byte <($1800+ 6*26)
        !byte <($1800+ 7*26)
        !byte <($1800+ 8*26)
        !byte <($1800+ 9*26)
        !byte <($1800+10*26)
        !byte <($1800+11*26)
        !byte <($1800+12*26)
        !byte <($1800+13*26)
        !byte <($1800+14*26)
        !byte <($1800+15*26)
        !byte <($1800+16*26)
        !byte <($1800+17*26)
        !byte <($1800+18*26)
        !byte <($1800+19*26)
        !byte <($1800+20*26)
        !byte <($1800+21*26)
        !byte <($1800+22*26)
        !byte <($1800+23*26)
        !byte <($1800+24*26)
        !byte <($1800+25*26)
        !byte <($1800+26*26)
        !byte <($1800+27*26)
        !byte <($1800+28*26)
        !byte <($1800+29*26)
        !byte <($1800+30*26)
        !byte <($1800+31*26)

line_high:
        !byte >($1800+ 0*26)
        !byte >($1800+ 1*26)
        !byte >($1800+ 2*26)
        !byte >($1800+ 3*26)
        !byte >($1800+ 4*26)
        !byte >($1800+ 5*26)
        !byte >($1800+ 6*26)
        !byte >($1800+ 7*26)
        !byte >($1800+ 8*26)
        !byte >($1800+ 9*26)
        !byte >($1800+10*26)
        !byte >($1800+11*26)
        !byte >($1800+12*26)
        !byte >($1800+13*26)
        !byte >($1800+14*26)
        !byte >($1800+15*26)
        !byte >($1800+16*26)
        !byte >($1800+17*26)
        !byte >($1800+18*26)
        !byte >($1800+19*26)
        !byte >($1800+20*26)
        !byte >($1800+21*26)
        !byte >($1800+22*26)
        !byte >($1800+23*26)
        !byte >($1800+24*26)
        !byte >($1800+25*26)
        !byte >($1800+26*26)
        !byte >($1800+27*26)
        !byte >($1800+28*26)
        !byte >($1800+29*26)
        !byte >($1800+30*26)
        !byte >($1800+31*26)

char_base:
        !byte  0*8
        !byte  1*8
        !byte  2*8
        !byte  3*8
        !byte  4*8
        !byte  5*8
        !byte  6*8
        !byte  7*8
        !byte  8*8
        !byte  9*8
        !byte 10*8
        !byte 11*8
        !byte 12*8
        !byte 13*8
        !byte 14*8
        !byte 15*8
        !byte 16*8
        !byte 17*8
        !byte 18*8
        !byte 19*8
        !byte 20*8
        !byte 21*8
        !byte 22*8
        !byte 23*8
        !byte 24*8
        !byte 25*8

buffer1:
        !byte 0
        !byte 0
        !byte 0
        !byte 0
        !byte 0
        !byte 0
        !byte 0
        !byte 0

buffer2:
        !byte 0
        !byte 0
        !byte 0
        !byte 0
        !byte 0
        !byte 0
        !byte 0
        !byte 0

offsets:
        !byte $FC
        !byte $EE
        !byte $04
        !byte $12
        !byte $00
        !byte $2C
