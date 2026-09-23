
        *=$0801
        ; BASIC stub: "1 SYS 2061"
        !byte $0b,$08,$01,$00,$9e,$32,$30,$36,$31,$00,$00,$00
        jmp entrypoint

        *=$0900
entrypoint:

        SEI
        LDA $DC0D
        AND #$7F
        ORA #$01
        STA $DC0D
        LDA #$F8
        STA $D012
        LDA $D011
        AND #$7F
        STA $D011
        LDA #$81
        STA $D01A
        LDA #<irq
        STA $0314
        LDA #>irq
        STA $0315
        CLI
        
        lda #$08
        sta $3fff
        
        lda #$06
        sta $d021
        lda #$0e
        sta $d020
        
        ldx #0
-
        lda screen,x
        sta $0400,x
        lda screen+$100,x
        sta $0500,x
        lda screen+$200,x
        sta $0600,x
        lda screen+$300,x
        sta $0700,x
        lda #1
        sta $d800,x
        sta $d900,x
        sta $da00,x
        sta $db00,x
        inx
        bne -
        
        ldx #39
-
        lda textline,x
        sta $0800+(40*24),x
        lda #$03
        sta $d800+(40*24),x
        dex
        bpl -
        
        jmp *

screen:
        !scr " 1                                     1"
        !scr " 2 this test displays (almost)         2"
        !scr " 3                                     3"
        !scr " 4 26 lines of text.                   4"
        !scr " 5                                     5"
        !scr " 6                                     6"
        !scr " 7                                     7"
        !scr " 8                                     8"
        !scr " 9                                     9"
        !scr "10                                    10"
        !scr "11                                    11"
        !scr "12                                    12"
        !scr "13                                    13"
        !scr "14                                    14"
        !scr "15                                    15"
        !scr "16                                    16"
        !scr "17                                    17"
        !scr "18                                    18"
        !scr "19                                    19"
        !scr "20                                    20"
        !scr "21                                    21"
        !scr "22                                    22"
        !scr "23                                    23"
        !scr "24                                    24"
        !scr "25 - last normal textline           - 25"
        
textline:
        !scr "26 - extra line of text             - 26"
        
irq:
        LDA $D019
        STA $D019
        LDA $D012
        CMP #$F8
        BEQ lastline
        CMP #$F3
        BEQ firstline
        LDA #$18
        STA $D011
        LDA #$14        ; normal screen at $400
        STA $D018
        LDA #$F3
        BNE skp
firstline:
        LDA #$1F
        STA $D011
        LDA #$24        ; second screen at $800
        STA $D018
        LDA #$F8
skp:
        STA $D012
        JMP $FEBC
lastline:
        LDA #$10
        STA $D011
        LDA #$28
        STA $D012
        
framecount = * + 1
        lda #5
        bne +
        lda #0
        sta $d7ff
+        
        dec framecount
        JMP $EA31
 
