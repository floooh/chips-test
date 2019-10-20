basicstart = $0801

        * = basicstart
        !word next
        !byte $0a, $00
        !byte $9e

        !byte $32, $30, $36, $31
next:
        !byte 0,0,0

        jmp start
        
irq:
        rti
        
start:
        sei
        lda #>irq
        sta $ffff
        sta $fffb
        lda #<irq
        sta $fffe
        sta $fffa

        ldx #0
-
        lda #$20
        sta $0400,x
        sta $0500,x
        sta $0600,x
        sta $0700,x
        lda #$01
        sta $d800,x
        sta $d900,x
        sta $da00,x
        sta $db00,x
        dex
        bne -
        
        lda #0
        sta $d020
        sta $d021
        
        ;--------------------------------------------------------------------
        ; put some known values into the CIAs

        ;--- CIA1 ---
        
        lda #%11111111  ; set all to output
        sta $dc02
        sta $dc03
        
        ldx #$34
        stx $dc00
        inx
        stx $dc01
        
        lda #%00000000  ; stop timers
        sta $dc0e
        sta $dc0f

        ldx #$12
        stx $dc04
        inx
        stx $dc05
        inx
        stx $dc06
        inx
        stx $dc07
        
        lda #%00010000  ; force load timers, stop timers
        sta $dc0e
        sta $dc0f
        
        lda #%10101010
        sta $dc0c       ; shift register
        
        lda #$7f
        sta $dc0d
        lda $dc0d
        
        ;--- CIA2 ---

        lda #%11111111  ; set all to output
        sta $dd02
        sta $dd03
        
        ldx #$43
        stx $dd00
        inx
        stx $dd01
        
        lda #%00000000  ; stop timers
        sta $dd0e
        sta $dd0f

        ldx #$21
        stx $dd04
        inx
        stx $dd05
        inx
        stx $dd06
        inx
        stx $dd07
        
        lda #%00010000  ; force load timers, stop timers
        sta $dd0e
        sta $dd0f
        
        lda #%01010101
        sta $dd0c       ; shift register
        
        lda #$7f
        sta $dd0d
        lda $dd0d
        
        ;--------------------------------------------------------------------
        
mlp:
        ; read back the I/O and show it on screen
        ldx #0
-
        lda $dc00,x
        sta $0400,x
        lda $dd00,x
        sta $0400+(10*40),x

        inx
        bne -

        ; create mirror tables
        ldx #0
-
        txa
        and #$0f
        tay
        lda $0400,y
        sta expected1,x
        lda $0400+(10*40),y
        sta expected2,x

        inx
        bne -

        ; compare against mirror table
        lda #0
        sta testfailed
        
        ldx #0
-

        ldy #5
        lda $0400,x
        cmp expected1,x
        beq +
        ldy #10
        sty testfailed
+
        tya
        sta $d800,x

        ldy #5
        lda $0400+(10*40),x
        cmp expected2,x
        beq +
        ldy #10
        sty testfailed
+
        tya
        sta $d800+(10*40),x

        inx
        bne -

        ldy #5
        ldx #0
testfailed = * + 1
        lda #0
        beq +
        ldy #10
        ldx #$ff
+
        sty $d020
        stx $d7ff
        
        jmp mlp
        
    !align 255,0,1
expected1:
    !for i,0,7 {
    !byte $34, $35, $ff, $ff, $12, $13, $14, $15, $00, $00, $00, $00, $aa, $00, $40, $40
    }
    !align 255,0,2
expected2:
    !for i,0,7 {
    !byte $43, $44, $ff, $ff, $21, $22, $23, $24, $00, $00, $00, $00, $55, $00, $40, $40
    }
