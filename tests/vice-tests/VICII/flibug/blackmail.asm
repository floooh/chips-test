
;STABLEFIX = 0

codeptr = $4d

flibugcolors = $3b00    ; lo nibble are rasterbars, hi nibble the colorram colors
colramdata   = $3c00
screendata   = $4000
bitmapdata   = $6000

generatedcode = $2000

        * = $0801
        ; basic stub: "1 sys 2061"
        !byte $0b,$08,$01,$00,$9e,$32,$30,$36,$31,$00,$00,$00

        ; fill bitmap with test pattern
        lda #%00011011
        ldy #$20
--
        ldx #0
-
bmph:   sta bitmapdata,x
        inx
        bne -
        inc bmph+2
        dey
        bne --

        ; colorram pattern
        ldx #0
-
        txa
        sta colramdata,x
        sta colramdata+$100,x
        sta colramdata+$200,x
        sta colramdata+$300,x
        inx
        bne -

        ; rasterbar pattern
        ldx #0
-
        txa
;        eor #$f0
        lsr
        lsr
        lsr
        lsr
        sta rval+1

        txa
        asl
        asl
        asl
        asl
        and #$f0
rval:   ora #$0f
        sta flibugcolors,x
        inx
        cpx #200
        bne -

        ; vram pattern
        ldy #8
--
        tya
        pha

        ldx #0
-
        txa
        clc
scr1:   adc #1
scr2:   sta screendata,x
scr3:   sta screendata+$100,x
scr4:   sta screendata+$200,x
scr5:   sta screendata+$300,x
        inx
        bne -

        lda scr1+1
        clc
        adc #$44
        sta scr1+1

        lda scr2+2
        clc
        adc #4
        sta scr2+2
        tay
        iny
        sty scr3+2
        iny
        sty scr4+2
        iny
        sty scr5+2

        pla
        tay
        dey
        bne --

        jsr codegenerator
        jsr displayer
        jmp *

;-------------------------------------------------------------------------------
; disassembly of the display routine from blackmails fli designer 2.2
;-------------------------------------------------------------------------------

codegenerator:
        ldy #<generatedcode
        lda #>generatedcode
        sty codeptr
        sta codeptr+1

        ; generate helper tables
        
        ; 03b0  b0 b1 b2 b3  b4 b5 b6 b7  b8 b9 ba bb  bc bd be bf
        
        ; 0057  3f a3
        
        ; addresses for lda (zp,x)
        ; 0059  b0 03  b1 03 b2 03  b3 03 b4 03  b5 03 b6 03 b7 03
        ; 0069  b8 03  b9 03 ba 03  bb 03 bc 03  bd 03 be 03 bf 03
        
        ldx #$1e
        ldy #$bf
        lda #$3f
        sta $57
-
        lda #3
        sty $59,x
        sta $5a,x
        tya
        sta $300,y
        dey
        dex
        dex
        bpl -
        
        ; add padding to colortables and vram

        lda flibugcolors
        sta flibugcolors-1
        sta flibugcolors+$c8
        lsr
        lsr
        lsr
        lsr
        sta colramdata
        sta colramdata+1
        sta colramdata+2

        lda #$ff
        sta screendata
        sta screendata+$1
        sta screendata+$2

        ldx #0

genloop:
        ldy #0
        
        ; get color for colorram
        lda flibugcolors-1,x
        lsr
        lsr
        lsr
        lsr

        ora #$a0                
        sta (codeptr),y         ; a0-af opcode aX (colorram color in low nibble)
        
        asl                     ; this sets carry (1010xxxx -> 010xxxx0, C=1)
        tay                     ; 40-5e offset for jumptable
        lsr                     ; this clears carry (010xxxx0 -> 0010xxxx, C=0)
        and #3
        ora #$8c                
        sta $4f                 ; 8c-8f sty/sta/stx/sax abs
        
        ; the load comes after the store to $d011, so the lo-nibble of the load
        ; opcode (!) will be fetched as color. the store writes the loaded value 
        ; to $d021 and must be adjusted accordingly
        
        ; col   load                 store
        ;
        ; 0     a0 ??    ldy #       8c 21 d0 sty abs   entry_0_2_9_b
        ; 1     a1 ??    lda (zp,x)  8d 21 d0 sta abs   entry_1_3
        ; 2     a2 ??    ldx #       8e 21 d0 stx abs   entry_0_2_9_b
        ; 3     a3 ??    lax (zp,x)  8f 21 d0 sax abs   entry_1_3
        ; 4     a4 ??    ldy zp      8c 21 d0 sty abs   entry_4_5_6_7
        ; 5     a5 ??    lda zp      8d 21 d0 sta abs   entry_4_5_6_7
        ; 6     a6 ??    ldx zp      8e 21 d0 stx abs   entry_4_5_6_7
        ; 7     a7 ??    lax zp      8f 21 d0 sax abs   entry_4_5_6_7
        ; 8     a8       tay         8c 21 d0 sty abs   entry_8_a
        ; 9     a9 ??    lda #       8d 21 d0 sta abs   entry_0_2_9_b
        ; a     aa       tax         8e 21 d0 stx abs   entry_8_a
        ; b     ab ??    lax #       8f 21 d0 sax abs   entry_0_2_9_b  <- unstable LAX imm
        ; c     ac ?? ?? ldy abs     8c 21 d0 sty abs   entry_c_d_e_f
        ; d     ad ?? ?? lda abs     8d 21 d0 sta abs   entry_c_d_e_f
        ; e     ae ?? ?? ldx abs     8e 21 d0 stx abs   entry_c_d_e_f
        ; f     af ?? ?? lax abs     8f 21 d0 sax abs   entry_c_d_e_f

        lda jumptable - $40,y
        sta jumpaddr
        lda jumptable - $40 + 1,y
        sta jumpaddr + 1

        ; get color for rasterbar
        lda flibugcolors,x
        and #$f
        ldy #1
        ; carry is cleared
jumpaddr = * + 1     
        jsr $dead

        cpx #$c8
        bcs genend

        ; ldx $69/$6b/$6d/$6f/$71/$73/$75/$77
        lda #$a6            ; ldx zp
        sta (codeptr),y
        iny
        txa
        and #7
        ora #$b8
        sta $57             ; (line & 7) | $b8 = $b8...$bf
        
        and #7
        asl
        adc #$69            ; $69 + ((line & 7) << 1) = $69, $6b, $6d ... $77
        sta (codeptr),y
        iny
        
        ; lda #$08..$78
        lda #$a9            ; lda #
        sta (codeptr),y
        iny
        txa
        and #7
        asl
        asl
        asl
        asl
!if STABLEFIX = 1 {
        ora #$0f
} else {
        ora #$08
}
        sta (codeptr),y
        iny
        
        ; sta $d018
        lda #$8d            ; sta abs
        sta (codeptr),y
        iny
        lda #<$d018
        sta (codeptr),y
        iny
        lda #>$d018
        sta (codeptr),y
        iny
        
        ; stx $d011
        lda #$8e            ; stx abs
        sta (codeptr),y
        iny
        lda #<$d011
        sta (codeptr),y
        iny
        lda #>$d011
        sta (codeptr),y

        sec
        tya
        adc codeptr
        sta codeptr
        bcc +
        inc codeptr+1
+
        inx
        bne genloop

genend:
        lda #$60            ; rts
        sta (codeptr),y
        rts

;-------------------------------------------------------------------------------

displayer:
        sei
        lda #$7f
        sta $dd0d
        ldy #0
        sty $dd0f
        sty $d015
        lda #<$4cc7
        sta $dd06
        lda #>$4cc7
        sta $dd07
        lda #<nmihandler
        sta $318
        lda #>nmihandler
        sta $319

        ldx #$c
-
        bit $d011
        bpl -
-
        bit $d011
        bmi -

loc_2a29:
        lda #$81
-
        dex
        bne -

        cpy $d012
        iny
        ldx #$a
        bcc loc_2a29
        dex
        cpy #$2e
        bcc loc_2a29

        sta $dd0f
        lda #$82
        sta $dd0d
        lda #$18
        sta $d016

        ; copy colorram
        ldy #0
-
        lda colramdata,y
        sta $d800,y
        lda colramdata+$100,y
        sta $d900,y
        lda colramdata+$200,y
        sta $da00,y
        lda colramdata+$300,y
        sta $db00,y
        dey
        bne -
        ; use videobank $4000-$7fff
        lda #2
        sta $dd00
        rts

; ---------------------------------------------------------------------------

        ; colorram colors:
        ; 0: black
        ; 2: red
        ; 9: brown
        ; b: d.grey
        ;
        ; y=1, a=rastercolor
        
        ; 0: black
        ;
        ; a0 xx     ldy # <color>
        ; 8c 21 d0  sty $d021
        ; ea        nop
        ; ea        nop
        
        ; 2: red
        ;
        ; a2 xx     ldx # <color>
        ; 8e 21 d0  stx $d021
        ; ea        nop
        ; ea        nop
        
        ; 9: brown
        ;
        ; a9 xx     lda # <color>
        ; 8d 21 d0  sta $d021
        ; ea        nop
        ; ea        nop
        
        ; BUG! the following requires bits 0-2 of the "magic constant" to be set
        
        ; b: d.grey
        ;
        ; ab xx     lax # <color>       <- unstable! (A=$08,$18...$78)
        ; 8f 21 d0  sax $d021
        ; ea        nop
        ; ea        nop
entry_0_2_9_b:
        jsr stored021write
        lda #$ea ; nop
        jmp store2bytesa

        ; colorram colors:
        ; 1: white
        ; 3: cyan
        ;
        ; y=1, a=rastercolor

        ; 1: white
        ;
        ; a1 xx     lda (zp,x)  ; ((color << 1) + $59) - ((line & 7) | $b8)
        ; 8d 21 d0  sta $d021

        ; 3: cyan
        ;
        ; a3 xx     lax (zp,x)  ; ((color << 1) + $59) - ((line & 7) | $b8)
        ; 8f 21 d0  sax $d021
entry_1_3:
        asl
        adc #$59            ; (color << 1) + $59
        sec
        sbc $57             ; (line & 7) | $b8 = $b8...$bf

stored021write:
        sta (codeptr),y
        ldy #2

sub_2a7d:
        lda $4f             ; sty/sta/stx/sax abs
        sta (codeptr),y
        iny
        lda #<$d021
        sta (codeptr),y
        iny
        lda #>$d021
        sta (codeptr),y
        iny
        rts

        ; colorram colors:
        ; 4: violet
        ; 5: green
        ; 6: blue
        ; 7: yellow
        ;
        ; y=1, a=rastercolor

        ; 4: violet
        ;
        ; a4 xx     ldy <(color << 1) + $59>
        ; 8c 21 d0  sty $d021
        ; 24 24     bit $24

        ; 5: green
        ;
        ; a5 xx     lda <(color << 1) + $59>
        ; 8d 21 d0  sta $d021
        ; 24 24     bit $24

        ; 6: blue
        ;
        ; a6 xx     ldx <(color << 1) + $59>
        ; 8e 21 d0  stx $d021
        ; 24 24     bit $24

        ; 7: yellow
        ;
        ; a7 xx     lax <(color << 1) + $59>
        ; 8f 21 d0  sax $d021
        ; 24 24     bit $24
entry_4_5_6_7:
        asl
        adc #$59
        jsr stored021write
        lda #$24            ; bit zp

store2bytesa:
        sta (codeptr),y
        iny

loc_2a98:
        sta (codeptr),y
        iny
        rts

        ; colorram colors:
        ; c: m.grey
        ; d: l.green
        ; e: l.blue
        ; f: l.grey
        ;
        ; y=1, a=rastercolor

        ; c: m.grey
        ;
        ; ac bd xx  ldy $03b<color>
        ; 8c 21 d0  sty $d021
        ; ea        nop
        
        ; d: l.green
        ;
        ; ad be xx  lda $03b<color>
        ; 8d 21 d0  sta $d021
        ; ea        nop
        
        ; e: l.blue
        ;
        ; ae bf xx  ldx $03b<color>
        ; 8e 21 d0  stx $d021
        ; ea        nop
        
        ; f: l.grey
        ;
        ; af c0 xx  lax $03b<color>
        ; 8f 21 d0  sax $d021
        ; ea        nop
entry_c_d_e_f:
        ; carry is cleared
        adc #$b0
        sta (codeptr),y
        iny
        lda #3

loc_2aa3:
        sta (codeptr),y
        ldy #3
        jsr sub_2a7d
        lda #$ea            ; nop
        bne loc_2a98

        ; colorram colors:
        ; 8: orange
        ; a: l.red
        ;
        ; y=1, a=rastercolor
        
        ; 8: orange
        ;
        ; a8        tay
        ; a0 xx     ldy # <color>
        ; 8c 21 d0  sty $d021
        ; ea        nop
        
        ; a: l.red
        ;
        ; aa        tax
        ; a2 xx     ldx # <color>
        ; 8e 21 d0  stx $d021
        ; ea        nop
entry_8_a:
        iny
        sta (codeptr),y
        dey
        lda $4f             ; 8c/8e sty/stx abs
        ; carry is cleared
        adc #$14            ; a0/a2 ldy/ldx imm
        bne loc_2aa3

;-------------------------------------------------------------------------------

nmihandler:
        pha
        stx $57
        lda $dd06
        sty $58
        clc
        adc #1
        and #$f
        eor #$f
        sta loc_2aca+1

loc_2aca:
        bcc loc_2acc

loc_2acc:
        cmp #$c9
        cmp #$c9
        cmp #$c9
        cmp #$c9
        cmp #$c9

loc_2ad6:
        cmp #$c9
        cmp #$c9
        bit $ea
        cmp (0,x)
        cmp (0,x)
        cmp (0,x)
        bit 0
        nop

        ldx #$7f
        stx $d011

        ldx #$3f
        jsr generatedcode

        ldx #$7f
        sta $d011

        lda #$fa
-
        cmp $d012
        bne -

        lda #$77
        sta $d011

        jsr $ff9f        ; jmp $ea87

        lda #$7f
-
        bit $d011
        bpl -

        sta $d011
        
        jsr docount
        
        bit $dd0d
        ldx $57
        ldy $58
        pla
        rti

; ---------------------------------------------------------------------------
jumptable:      
        !word entry_0_2_9_b  ; 0: black
        !word entry_1_3      ; 1: white
        !word entry_0_2_9_b  ; 2: red
        !word entry_1_3      ; 3: cyan
        !word entry_4_5_6_7  ; 4: violet
        !word entry_4_5_6_7  ; 5: green
        !word entry_4_5_6_7  ; 6: blue
        !word entry_4_5_6_7  ; 7: yellow
        !word entry_8_a      ; 8: orange
        !word entry_0_2_9_b  ; 9: brown
        !word entry_8_a      ; a: l.red
        !word entry_0_2_9_b  ; b: d.grey
        !word entry_c_d_e_f  ; c: m.grey
        !word entry_c_d_e_f  ; d: l.green
        !word entry_c_d_e_f  ; e: l.blue
        !word entry_c_d_e_f  ; f: l.grey
; ---------------------------------------------------------------------------

framecount: !byte 5

docount:
        dec framecount
        bne +
        lda #0
        sta $d7ff
+
        rts
