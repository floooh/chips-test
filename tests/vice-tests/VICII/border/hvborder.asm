
        * = $0801
        !byte $0c, $08, $00, $00, $9e, $20, $32, $30, $36, $34
        !byte 0,0,0

        * = $0810

        sei

        lda #$15
        sta $d018
        lda #6
        sta $d021
        lda #14
        sta $d020
        
        ldx #0
-
        lda screen,x
        sta $0400,x
        lda screen+$100,x
        sta $0500,x
        lda screen+$200,x
        sta $0600,x
        lda #$20
        sta $0700,x
        lda #1
        sta $d800,x
        sta $d900,x
        sta $da00,x
        sta $db00,x
        inx
        bne -
 
        lda #$64
        ldx #39-20
-
        sta $0400+(24*40)+10,x
        dex
        bpl -
        
        ; init ghostbyte with some pattern to make border more visible
        ; and so we can see character boundaries in the border
        lda #%10000000
        sta $3fff

        ; setup textscreen Y position
        lda #$18
        sta $d011
        ; init X scroll
        lda #$c8
        sta $d016

        ; now setup the 8 sprites

        lda #$ff
        sta $d015 ;enable

        lda #%11111111
        sta $d01d ;xpand-x

        lda #$0
        sta $d017 ;xpand-y
        sta $d01c ;muco
        sta $d01b ;prio

        ldx #0
        ldy #0
ll
        txa
        clc
        adc #9
        sta $d027,x        ; sprite colors
        lda #spritedata/64
        sta $07f8,x        ; sprite pointers
!if MODE=0 {
        lda #$ff           ; $ff for the first variation of the main loop, $fa for the second variation
} else {
        lda #$fa
}
        sta $d001,y        ; y-pos
        iny
        iny
        inx
        cpx #8
        bne ll

        ; sprite data
        ldx #0
        lda #%10100000
l2
        sta spritedata,x
        inx
        cpx #63
        bne l2

        ; to display a sprite further left than regular position 0, set MSB in $d010
        ; and substract 8 extra pixels
        lda #$f8-8
        sta $d000

            ; set x pos for the other 7 sprites
        lda #$f8
        ldx #7
        ldy #2
spr1			
        clc
        adc #(2*24)
        sta $d000,y
        iny
        iny
        dex
        bpl spr1

        ; x-pos MSB
        lda #%11000001
        sta $d010
        
        jmp main_loop
        
*=$0900 ; align to some page so branches do not cross a page boundary and fuck up the timing        

!if MODE = 0 {

        ;-------------------mainloop---------
main_loop

        ; since we need cycle exact non jittering timing we must stabilize the raster

        ldx #$d1
        jsr rastersync

        ldy #$c8        ; value for 40 columns mode

        lda #$f8
rl1     cmp $d012
        bne rl1

        ; open the lower border
        lda #$18&$f7
        sta $d011

        lda #$ff
rl2     cmp $d012
        bne rl2

        ; delay so the first write to $d016 happens exactly at the
        ; beginning of the last character before the border

        ; 0: 4 cycles 1: 9 cycles 2: 14 cycles 3: 19 cycles etc
        ldx #6             ; 2
wl1     dex                ; 2
        bpl wl1            ; 3 (2)

        nop                ; 2

        ; the actual loop to open the sideborders

        ldx #21
ol1
        dec $d016       ; 6
        sty $d016       ; 4
        nop             ; 2
        nop             ; 2
        nop             ; 2
        nop             ; 2
        nop             ; 2
        nop             ; 2
        nop             ; 2
        nop             ; 2
        nop             ; 2
        nop             ; 2
        nop             ; 2
        nop             ; 2
        nop             ; 2
        clc             ; 2
        bcc sk1         ; 3
sk1
        dex             ; 2
        bpl ol1         ; 3
                        ; -> 46 ( 63-((8*2)+1) )

        ; reset $d011 for next round of upper/lower border opening
        lda #$18
        sta $d011

        dec framecount
        bne +
        lda #0
        sta $d7ff
+
        jmp main_loop
        
}

!if MODE = 1 {
        
*=$0900 ; align to some page so branches do not cross a page boundary and fuck up the timing        

        ;-------------------mainloop---------
main_loop

        ; since we need cycle exact non jittering timing we must stabilize the raster

        ldx #$d1
        jsr rastersync

        ldy #$c8        ; value for 40 columns mode

        lda #$fa
rl2     cmp $d012
        bne rl2

        ; delay so the first write to $d016 happens exactly at the
        ; beginning of the last character before the border

        ; 0: 4 cycles 1: 9 cycles 2: 14 cycles 3: 19 cycles etc
        ldx #5             ; 2
wl1     dex                ; 2
        bpl wl1            ; 3 (2)

        nop                ; 2
        nop                ; 2
        nop                ; 2
        nop                ; 2

        ; the actual loop to open the sideborders

        ldx #21
ol1
        dec $d016       ; 6
        sty $d016       ; 4
        nop             ; 2
        nop             ; 2
        nop             ; 2
        nop             ; 2
        nop             ; 2
        nop             ; 2
        nop             ; 2
        nop             ; 2
        nop             ; 2
        nop             ; 2
        nop             ; 2
        nop             ; 2
        nop             ; 2
        clc             ; 2
        bcc sk1         ; 3
sk1
        dex             ; 2
        bpl ol1         ; 3
                        ; -> 46 ( 63-((8*2)+1) )

        dec framecount
        bne +
        lda #0
        sta $d7ff
+
        jmp main_loop
}

;--------------------------------------------------
; simple polling rastersync routine

        *=$0d00 ; align to some page so branches do not cross a page boundary and fuck up the timing

rastersync:

lp1:
          cpx $d012
          bne lp1
          jsr cycles
          bit $ea
          nop
          cpx $d012
          beq skip1
          nop
          nop
skip1:    jsr cycles
          bit $ea
          nop
          cpx $d012
          beq skip2
          bit $ea
skip2:    jsr cycles
          nop
          nop
          nop
          cpx $d012
          bne onecycle
onecycle: rts

cycles:
         ldy #$06
lp2:     dey
         bne lp2
         inx
         nop
         nop
         rts
         
framecount: !byte 5

screen:
!if MODE=0 {
    !byte $20,$20,$20,$20,$20,$20,$20,$20,$20,$20
    !byte $63,$63,$63,$63,$63,$63,$63,$63,$63,$63
    !byte $63,$63,$63,$63,$63,$63,$63,$63,$63,$63
    !byte $20,$20,$20,$20,$20,$20,$20,$20,$20,$20
    !scr "                                        "
    !scr " open vertical border first, then open  "
    !scr " sideborder.                            "
    !scr "                                        "
    !scr " idle byte behind area with open        "
    !scr " sideborder.                            "
    !scr "                                        "
    !scr " no gap between top white line and idle "
    !scr " gfx.                                   "
    !scr "                                        "
    !scr " no gap between bottom white line and   "
    !scr " idle gfx.                              "
    !scr "                                        "
    !scr " both white lines are one pixel high.   "
    !scr "                                        "
}
!if MODE=1 {
    !byte $20,$20,$20,$20,$20,$20,$20,$20,$20,$20
    !byte $46,$46,$46,$46,$46,$46,$46,$46,$46,$46
    !byte $46,$46,$46,$46,$46,$46,$46,$46,$46,$46
    !byte $20,$20,$20,$20,$20,$20,$20,$20,$20,$20
    !scr "                                        "
    !scr " open sideborder, keep opening it in    "
    !scr " lower border.                          "
    !scr "                                        "
    !scr " no idle byte behind area with open     "
    !scr " sideborder.                            "
    !scr "                                        "
    !scr " one line gap (background) between top  "
    !scr " white line and top border.             "
    !scr "                                        "
    !scr " no gap between bottom white line and   "
    !scr " idle gfx.                              "
    !scr "                                        "
    !scr " top white line is two pixel high.      "
    !scr " bottom white line is one pixel high.   "
}
    !scr "                                        "
    !scr "                                        "
    !scr "                                        "
    !scr "                                        "
         
spritedata=$0fc0        
