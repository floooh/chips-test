         *= $0801

         .byte $4c,$16,$08,$00,$97,$32
         .byte $2c,$30,$3a,$9e,$32,$30
         .byte $37,$30,$00,$00,$00,$a9
         .byte $01,$85,$02

         jsr print
         .byte 13
         .text "(up)rora"
         .byte 0

         lda #%00011011
         sta db
         lda #%11000110
         sta ab
         lda #%10110001
         sta xb
         lda #%01101100
         sta yb
         lda #0
         sta pb
         tsx
         stx sb

         lda #0
         sta db

next     lda db
         sta da
         lsr a
         sta dr
         lda pb
         and #1
         beq noc
         lda dr
         ora #128
         sta dr
noc
         lda ab
         sta ar

         lda xb
         sta xr

         lda yb
         sta yr

         lda pb
         ora #%00110000
         and #%01111100
         tax
         lda dr
         cmp #0
         bne nozero
         txa
         ora #%00000010
         tax
nozero   lda dr
         bpl noneg
         txa
         ora #%10000000
         tax
noneg    lda db
         and #1
         beq nocarry
         txa
         ora #%00000001
         tax
nocarry  stx pr

         lda sb
         sta sr

         ldx sb
         txs
         lda pb
         pha
         lda ab
         ldx xb
         ldy yb
         plp

cmd      ror da

         php
         cld
         sta aa
         stx xa
         sty ya
         pla
         sta pa
         tsx
         stx sa
         jsr check

         inc db
         bne jmpnext
         inc pb
         beq nonext
jmpnext  jmp next
nonext

         jsr print
         .text " - ok"
         .byte 13,0
         lda 2
         beq load
wait     jsr $ffe4
         beq wait
         jmp $8000

load     jsr print
name     .text "rorax"
namelen  = *-name
         .byte 0
         lda #0
         sta $0a
         sta $b9
         lda #namelen
         sta $b7
         lda #<name
         sta $bb
         lda #>name
         sta $bc
         pla
         pla
         jmp $e16f

db       .byte 0
ab       .byte 0
xb       .byte 0
yb       .byte 0
pb       .byte 0
sb       .byte 0
da       .byte 0
aa       .byte 0
xa       .byte 0
ya       .byte 0
pa       .byte 0
sa       .byte 0
dr       .byte 0
ar       .byte 0
xr       .byte 0
yr       .byte 0
pr       .byte 0
sr       .byte 0

check
         .block
         lda da
         cmp dr
         bne error
         lda aa
         cmp ar
         bne error
         lda xa
         cmp xr
         bne error
         lda ya
         cmp yr
         bne error
         lda pa
         cmp pr
         bne error
         lda sa
         cmp sr
         bne error
         rts

error    jsr print
         .byte 13
         .null "before  "
         ldx #<db
         ldy #>db
         jsr showregs
         jsr print
         .byte 13
         .null "after   "
         ldx #<da
         ldy #>da
         jsr showregs
         jsr print
         .byte 13
         .null "right   "
         ldx #<dr
         ldy #>dr
         jsr showregs
         lda #13
         jsr $ffd2
wait     jsr $ffe4
         beq wait
         cmp #3
         beq stop
         rts
stop     lda 2
         beq basic
         jmp $8000
basic    jmp ($a002)

showregs stx 172
         sty 173
         ldy #0
         lda (172),y
         jsr hexb
         lda #32
         jsr $ffd2
         lda #32
         jsr $ffd2
         iny
         lda (172),y
         jsr hexb
         lda #32
         jsr $ffd2
         iny
         lda (172),y
         jsr hexb
         lda #32
         jsr $ffd2
         iny
         lda (172),y
         jsr hexb
         lda #32
         jsr $ffd2
         iny
         lda (172),y
         ldx #"n"
         asl a
         bcc ok7
         ldx #"N"
ok7      pha
         txa
         jsr $ffd2
         pla
         ldx #"v"
         asl a
         bcc ok6
         ldx #"V"
ok6      pha
         txa
         jsr $ffd2
         pla
         ldx #"0"
         asl a
         bcc ok5
         ldx #"1"
ok5      pha
         txa
         jsr $ffd2
         pla
         ldx #"b"
         asl a
         bcc ok4
         ldx #"B"
ok4      pha
         txa
         jsr $ffd2
         pla
         ldx #"d"
         asl a
         bcc ok3
         ldx #"D"
ok3      pha
         txa
         jsr $ffd2
         pla
         ldx #"i"
         asl a
         bcc ok2
         ldx #"I"
ok2      pha
         txa
         jsr $ffd2
         pla
         ldx #"z"
         asl a
         bcc ok1
         ldx #"Z"
ok1      pha
         txa
         jsr $ffd2
         pla
         ldx #"c"
         asl a
         bcc ok0
         ldx #"C"
ok0      pha
         txa
         jsr $ffd2
         pla
         lda #32
         jsr $ffd2
         iny
         lda (172),y
         .bend
hexb     pha
         lsr a
         lsr a
         lsr a
         lsr a
         jsr hexn
         pla
         and #$0f
hexn     ora #$30
         cmp #$3a
         bcc hexn0
         adc #6
hexn0    jmp $ffd2

print    pla
         .block
         sta print0+1
         pla
         sta print0+2
         ldx #1
print0   lda !*,x
         beq print1
         jsr $ffd2
         inx
         bne print0
print1   sec
         txa
         adc print0+1
         sta print2+1
         lda #0
         adc print0+2
         sta print2+2
print2   jmp !*
         .bend


