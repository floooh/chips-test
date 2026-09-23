; test posted by konsolero in forum64
; https://www.forum64.de/index.php?thread/72867-vsp-bug-checker/

; Used illegal opcodes
; $80,$??        nop #$??        immediate

        * = $0801
        ; basic stub: "1 sys 2061"
        !byte $0b,$08,$01,$00,$9e,$32,$30,$36,$31,$00,$00,$00
        jmp start
        
start:

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
        lda payload,x
        sta $c000,x
        lda payload+$100,x
        sta $c100,x
        inx
        bne -
        stx $d020
        stx $d021
        jmp $c000

payload:        
        
!pseudopc $c000 {

.main
        sei
        lda #$7f
        sta $dc0d       ; disable CIA1 IRQ
        !byte $80,$a5
        ;-----------------
.s1
        sta $dd0d       ; disable CIA2 NMI
        lda $dc0d       ; ack CIA1
        !byte $80,$a5
        ;-----------------
.s2
        lda $dd0d       ; ack CIA2
        ldx #$fe
        txs             ; set stack-pointer to stable address
        !byte $80,$a5
        ;-----------------
.s3
        lda #<.nmi
        sta $fffa
        nop
        !byte $80,$a5
        ;-----------------
.s4
        lda #>.nmi
        sta $fffb       ; nmi to resume after VSP-loop
        nop
        !byte $80,$a5
        ;-----------------
.s5
        lda #$00
        sta $d020
        nop
        !byte $80,$a5
        ;-----------------
.s6
        lda #$34
        sta $01         ; all RAM
        lda #$00
        !byte $80,$a5
        ;-----------------
.s7
        sta $fb
        sta $fc
        ldy #$07
        !byte $80,$a5
        ;-----------------
.s8
        lda #$a5
.l4
        sta ($fb),y     ; write %10100101 / %01011010 to all fragile addresses ($xxx7 / $xxxf)
        iny
        iny
        !byte $80,$a5
        ;-----------------
.s85
        eor #$ff        ; invert value
        bit $eaea
        nop
        !byte $80,$a5
        ;-----------------
.s9
        iny
        iny
        iny
        iny
        iny
        iny             ; set y to next fragile address
        !byte $80,$a5
        ;-----------------
.s10
        cpy #$08        ; leaving page?
        bcs .l4
        inc $fc         ; next page
        !byte $80,$a5
        ;-----------------
.s11
        bne .l4         ; up to $ffff
        lda #$40
        sta $02         ; loopcounter
        !byte $80,$a5
        ;-----------------
.s12
        lda #$4c
        sta .mod1       ; jmp$ opcode for VSP-loop
        nop
        !byte $80,$a5
        ;-----------------
.s13
        inc $01         ; only I/O
        lda #$00
        nop
        nop
        !byte $80,$a5
        ;-----------------
.s14
        sta $dd0e       ; stop timer A CIA2
        lda #$ff
        nop
        !byte $80,$a5
        ;-----------------
.s15
        sta $dd04
        sta $dd05       ; latch timer A CIA2
        !byte $80,$a5
        ;-----------------
.s16
        lda #$81
        sta $dd0d       ; enable CIA2 nmi
        nop
        !byte $80,$a5
        ;-----------------
.s17
        lda #$01
        sta $dd0e       ; start timer A CIA2
        nop
        !byte $80,$a5
        ;-----------------
.s18
.l5
        inc $d011
.mod1
        jmp .l5         ; VSP-bug loop
        !byte $80,$a5
        ;-----------------
.s19
        lda #$1b
        sta $d011       ; restore $d011
        nop
        !byte $80,$a5
        ;-----------------
.s195
        dec $01         ; all RAM
        bit $eaea
        nop
        !byte $80,$a5
        ;-----------------
.s20
        ldx #$01
        stx $02         ; VSP-bug flag
        dex
        nop
        !byte $80,$a5
        ;-----------------
.s21
        stx $fb
        stx $fc
        ldy #$07
        !byte $80,$a5
        ;-----------------
.s22
        lda #$a5
.l6
        cmp ($fb),y     ; check fragile memory
        bne .l7
        !byte $80,$a5
        ;-----------------
.s23
        iny
        iny
        iny
        iny
        iny
        iny
        !byte $80,$a5
        ;-----------------
.s235
        eor #$ff
        bit $eaea
        nop
        !byte $80,$a5
        ;-----------------
.s24
        iny
        iny
        cpy #$08
        bcs .l6
        !byte $80,$a5
        ;-----------------
.s25
        inc $fc         ; next page
        bne .l6
        dec $02         ; clear VSP-bug flag
        !byte $80,$a5
        ;-----------------
.s26
.l7
        inc $01         ; only I/O
        bit $eaea
        nop
        !byte $80,$a5
        ;-----------------
.s265
        lda #<.nmi2
        sta $fffa
        nop
        !byte $80,$a5
        ;-----------------
.s27
        lda #>.nmi2
        sta $fffb       ; nmi2 to reset by hitting restore
        nop
        !byte $80,$a5
        ;-----------------
.s28
        ldx #$02        ; color red (VSP-bug)
        lda $02
        bne .l8
        !byte $80,$a5
        ;-----------------
.s29
        ldx #$0d        ; color lightgreen (VSP-ok)
.l8
        stx $d020       ; set bordercolor
        nop
        !byte $80,$a5
        ;-----------------
.s30
        ldx #$02
.l9
        lda .msg1,x
        nop
        !byte $80,$a5
        ;-----------------
.s301
        sta $05f2,x     ; screen text VSP
        dex
        bpl .l9
        !byte $80,$a5
        ;-----------------
.s302
        ldx #$02
        lda $02
        bne .l10
        !byte $80,$a5
        ;-----------------
.s303
        ldx #$05
.l10
        ldy #$02
        nop
        nop
        !byte $80,$a5
        ;-----------------
.s304
.l11
        lda .msg2,x
        sta $061a,y     ; screen text BUG/OK
        !byte $80,$a5
        ;-----------------
.s305
        dex
        dey
        bpl .l11
        nop
        nop
        !byte $80,$a5
        ;-----------------
.s309
        jmp .s28a       ; to end

.nmi
        lda $dd0d       ; ack NMI
        !byte $80,$a5
        ;-----------------
.s31
        and #$01
        beq .goback     ; no timer interrupt
        dec $02         ; dec loopcounter
        !byte $80,$a5
        ;-----------------
.s32
        bne .goback     ; until zero
        lda #$00
        nop
        nop
        !byte $80,$a5
        ;-----------------
.s33
        sta $dd0e       ; stop timer A CIA2
        lda #$7f
        nop
        !byte $80,$a5
        ;-----------------
.s34
        sta $dd0d       ; disable CIA2 NMI
        lda #$2c
        nop
        !byte $80,$a5
        ;-----------------
.s35
        sta .mod1       ; bit$ opcode for resume
.goback
        rti
        nop
        nop
        !byte $80,$a5
        ;-----------------
.s36
.nmi2
        lda $dd0d       ; ack NMI
        lda #$37
        nop
        !byte $80,$a5
        ;-----------------
.s37
        sta $01         ; normal RAM
        jmp ($fffc)     ; reset
        nop
        !byte $80,$a5
        ;-----------------
.msg1
        !ct scr
        !text "vsp"
        !byte $a5,$a5,$a5,$a5,$a5
        ;-----------------
.msg2
        !text "bugok "
        !byte $a5,$a5
        ;-----------------

.s28a
        ldx #$ff        ; (VSP-bug)
        lda $02
        bne .l8a
        !byte $80,$a5
        ;-----------------
.s29a
        ldx #$00        ; (VSP-ok)
.l8a
        stx $d7ff       ; set debug register
        nop
        !byte $80,$a5
        ;-----------------

        jmp *
}
