
           *= $0801
           .byte $4c,$14,$08,$00,$97
turboass   = 780
           .text "780"
           .byte $2c,$30,$3a,$9e,$32,$30
           .byte $37,$33,$00,$00,$00
           lda #1
           sta turboass
           jmp main


print
           .block
           pla
           sta print0+1
           pla
           sta print0+2
           ldx #1
print0
           lda !*,x
           beq print1
           jsr $ffd2
           inx
           bne print0
print1
           sec
           txa
           adc print0+1
           sta print2+1
           lda #0
           adc print0+2
           sta print2+2
print2
           jmp !*
           .bend


printhb
           .block
           pha
           lsr a
           lsr a
           lsr a
           lsr a
           jsr printhn
           pla
           and #$0f
printhn
           ora #$30
           cmp #$3a
           bcc printhn0
           adc #6
printhn0
           jsr $ffd2
           rts
           .bend



delay
           .block
           sta $dd04
           stx $dd05
           lda #%00011001
           sta $dd0e
           lda #1
wait
           bit $dd0e
           bne wait
           rts
           .bend


gatimer    *= *+4
gatimerc   *= *+4


inittimer
           .block
           lda #0
           ldx #3
clear
           sta gatimerc,x
           dex
           bpl clear
           jsr starttimer
           jsr stoptimer
           ldx #3
setc
           lda gatimer,x
           sta gatimerc,x
           dex
           bpl setc
           rts
           .bend


starttimer
           .block
           sei
           lda $d011
           and #%11101111
          ; sta $d011
           bmi isborder
waitborder
           lda $d012
           cmp #30
           bcs waitborder
isborder
           lda #%01111111
           sta $dd0d
           lda #0
           sta $dd0e
           sta $dd0f
           lda #$ff
           sta $dd04
           sta $dd05
           sta $dd06
           sta $dd07
           lda #%01010001
           sta $dd0f
           lda #%00010001
           sta $dd0e
           rts
           .bend


stoptimer
           .block
           ldx #0
           stx $dd0e
           cld
           sec
           lda $dd04
           eor #$ff
           sbc gatimerc+0
           sta gatimer+0
           lda $dd05
           eor #$ff
           sbc gatimerc+1
           sta gatimer+1
           lda $dd06
           eor #$ff
           sbc gatimerc+2
           sta gatimer+2
           lda $dd07
           eor #$ff
           sbc gatimerc+3
           sta gatimer+3
           cli
           lda $d011
           ora #%00010000
           sta $d011
           rts
           .bend


printtimer
           .block
           lda gatimer+2
           sta $63
           lda gatimer+3
           sta $62
           ldx #$90
           sec
           jsr $bc49
           lda #<r65536
           ldy #>r65536
           jsr $ba28
           jsr $bc0c
           lda gatimer+0
           sta $63
           lda gatimer+1
           sta $62
           ldx #$90
           sec
           jsr $bc49
           jsr $b86a
           jsr $bddd
           ldx $0100
           cpx #32
           bne negative
           clc
           adc #1
negative
           jmp $ab1e
r65536     .byte $91,$00,$00,$00,$00
           .bend

gsavesp    .byte 0
gastack    *= *+256

savestack
           .block
           tsx
           stx gsavesp
           ldx #0
save
           lda $0100,x
           sta gastack,x
           inx
           bne save
           rts
           .bend


restorestack
           .block
           pla
           sta return+1
           pla
           sta return+2
           ldx gsavesp
           inx
           inx
           txs
           ldx #0
restore
           lda gastack,x
           sta $0100,x
           inx
           bne restore
return
           jmp $1111
           .bend

addressing
           .word brkn
           .word rix
           .word hltn
           .word mix
           .word rz
           .word rz
           .word mz
           .word mz
           .word phpn
           .word b
           .word n
           .word b
           .word ra
           .word ra
           .word ma
           .word ma
           .word r
           .word riy
           .word hltn
           .word miy
           .word rzx
           .word rzx
           .word mzx
           .word mzx
           .word n
           .word ray
           .word n
           .word may
           .word rax
           .word rax
           .word max
           .word max
           .word jsrw
           .word rix
           .word hltn
           .word mix
           .word rz
           .word rz
           .word mz
           .word mz
           .word plpn
           .word b
           .word n
           .word b
           .word ra
           .word ra
           .word ma
           .word ma
           .word r
           .word riy
           .word hltn
           .word miy
           .word rzx
           .word rzx
           .word mzx
           .word mzx
           .word n
           .word ray
           .word n
           .word may
           .word rax
           .word rax
           .word max
           .word max
           .word rtin
           .word rix
           .word hltn
           .word mix
           .word rz
           .word rz
           .word mz
           .word mz
           .word phan
           .word b
           .word n
           .word b
           .word jmpw
           .word ra
           .word ma
           .word ma
           .word r
           .word riy
           .word hltn
           .word miy
           .word rzx
           .word rzx
           .word mzx
           .word mzx
           .word n
           .word ray
           .word n
           .word may
           .word rax
           .word rax
           .word max
           .word max
           .word rtsn
           .word rix
           .word hltn
           .word mix
           .word rz
           .word rz
           .word mz
           .word mz
           .word plan
           .word b
           .word n
           .word b
           .word jmpi
           .word ra
           .word ma
           .word ma
           .word r
           .word riy
           .word hltn
           .word miy
           .word rzx
           .word rzx
           .word mzx
           .word mzx
           .word n
           .word ray
           .word n
           .word may
           .word rax
           .word rax
           .word max
           .word max
           .word b
           .word wix
           .word b
           .word rix
           .word wz
           .word wz
           .word wz
           .word rz
           .word n
           .word b
           .word n
           .word b
           .word wa
           .word wa
           .word wa
           .word ra
           .word r
           .word wiy
           .word hltn
           .word wiy
           .word wzx
           .word wzx
           .word wzy
           .word rzy
           .word n
           .word way
           .word n
           .word way
           .word wax
           .word wax
           .word way
           .word way
           .word b
           .word rix
           .word b
           .word rix
           .word rz
           .word rz
           .word rz
           .word rz
           .word n
           .word b
           .word n
           .word b
           .word ra
           .word ra
           .word ra
           .word ra
           .word r
           .word riy
           .word hltn
           .word riy
           .word rzx
           .word rzx
           .word rzy
           .word rzy
           .word n
           .word ray
           .word n
           .word ray
           .word rax
           .word rax
           .word ray
           .word ray
           .word b
           .word rix
           .word b
           .word mix
           .word rz
           .word rz
           .word mz
           .word mz
           .word n
           .word b
           .word n
           .word b
           .word ra
           .word ra
           .word ma
           .word ma
           .word r
           .word riy
           .word hltn
           .word miy
           .word rzx
           .word rzx
           .word mzx
           .word mzx
           .word n
           .word ray
           .word n
           .word may
           .word rax
           .word rax
           .word max
           .word max
           .word b
           .word rix
           .word b
           .word mix
           .word rz
           .word rz
           .word mz
           .word mz
           .word n
           .word b
           .word n
           .word b
           .word ra
           .word ra
           .word ma
           .word ma
           .word r
           .word riy
           .word hltn
           .word miy
           .word rzx
           .word rzx
           .word mzx
           .word mzx
           .word n
           .word ray
           .word n
           .word may
           .word rax
           .word rax
           .word max
           .word max

cmd        .byte 0

main
           jsr print
           .byte 13
           .text "(up)cputiming"
           .byte 0

           tsx
           stx error+1
           lda #0
           sta cmd
           jsr inittimer

loop
           lda #<addressing
           ldx #>addressing
           clc
           adc cmd
           bcc noinc1
           inx
noinc1
           clc
           adc cmd
           bcc noinc2
           inx
noinc2
           sta 172
           stx 173
           ldy #0
           lda (172),y
           sta jump+1
           iny
           lda (172),y
           sta jump+2
           lda #0
           sta gatimer+0
           sta gatimer+1
           sta gatimer+2
           sta gatimer+3
jump
           jsr $1111
noerror
           inc cmd
           bne loop

           jmp ok

compare
           cmp gatimer+0
           bne error
           ldx gatimer+1
           bne error
           ldx gatimer+2
           bne error
           ldx gatimer+3
           bne error
           rts

error
           ldx #$11
           txs
           pha
           lda #13
           jsr $ffd2
           lda cmd
           jsr printhb
           jsr print
           .byte 13
           .text "clocks "
           .byte 0
           jsr printtimer
           jsr print
           .byte 13
           .text "right  "
           .byte 0
           pla
           sta gatimer+0
           lda #0
           sta gatimer+1
           sta gatimer+2
           sta gatimer+3
           jsr printtimer
waitk
           jsr $ffe4
           beq waitk
           cmp #3
           beq stop
           jmp noerror
stop
           lda turboass
           beq basic
           jmp $8000
basic
           jmp $a474


ok
           jsr print
           .text " - ok"
           .byte 13,0
           lda turboass
           beq load
waitkey    jsr $ffe4
           beq waitkey
           jmp $8000

load
           lda #47
           sta 0
           jsr print
name       .text "irq"
namelen    = *-name
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


n
           jsr savestack
           lda cmd
           sta nx
           lda #$7f
           sta $dc0d
           jsr starttimer
nx
           nop
           jsr stoptimer
           lda #$81
           sta $dc0d
           jsr restorestack
           lda #2
           jmp compare


b
           lda cmd
           sta bx
           jsr starttimer
bx
           lda #0
           jsr stoptimer
           lda #2
           jmp compare

rz
wz
           .block
           lda cmd
           sta command
           jsr starttimer
command
           lda 2
           jsr stoptimer
           lda #3
           jmp compare
           .bend


mz
           .block
           lda cmd
           sta command
           jsr starttimer
command
           lda 2
           jsr stoptimer
           lda #5
           jmp compare
           .bend


rzx
rzy
wzx
wzy
           .block
           lda cmd
           sta nowrap
           jsr starttimer
           ldx #2
           ldy #2
nowrap
           lda 0,x
           jsr stoptimer
           lda #8
           jsr compare
           lda cmd
           sta wrap
           jsr starttimer
           ldx #$ff
           ldy #$ff
wrap
           lda 3,x
           jsr stoptimer
           lda #8
           jmp compare
           .bend



mzx
mzy
           .block
           lda cmd
           sta nowrap
           jsr starttimer
           ldx #2
           ldy #2
nowrap
           lda 0
           jsr stoptimer
           lda #10
           jsr compare
           lda cmd
           sta wrap
           jsr starttimer
           ldx #$ff
           ldy #$ff
wrap
           lda 3
           jsr stoptimer
           lda #10
           jmp compare
           .bend


ra
wa
           .block
           lda cmd
           sta command
           jsr starttimer
command
           lda $ffff
           jsr stoptimer
           lda #4
           jmp compare
           .bend


ma
           .block
           lda cmd
           sta command
           jsr starttimer
command
           lda $ffff
           jsr stoptimer
           lda #6
           jmp compare
           .bend


rax
ray
           .block
           jsr savestack
           lda cmd
           sta nowrap
           jsr starttimer
           ldx #0
           ldy #0
nowrap
           lda $ffff
           jsr stoptimer
           jsr restorestack
           lda #8
           jsr compare
           jsr savestack
           lda cmd
           sta wrap
           jsr starttimer
           ldx #3
           ldy #3
wrap
           lda $ffff
           jsr stoptimer
           jsr restorestack
           lda #9
           jmp compare
           .bend


wax
way
           .block
           jsr savestack
           lda cmd
           sta nowrap
           jsr starttimer
           ldx #0
           ldy #0
nowrap
           lda $ffff
           jsr stoptimer
           jsr restorestack
           lda #9
           jsr compare
           jsr savestack
           lda cmd
           sta wrap
           jsr starttimer
           ldx #3
           ldy #3
wrap
           lda $ffff
           jsr stoptimer
           jsr restorestack
           lda #9
           jmp compare
           .bend


max
may
           .block
           jsr savestack
           lda cmd
           sta nowrap
           jsr starttimer
           ldx #0
           ldy #0
nowrap
           lda $ffff
           jsr stoptimer
           jsr restorestack
           lda #11
           jsr compare
           jsr savestack
           lda cmd
           sta wrap
           jsr starttimer
           ldx #3
           ldy #3
wrap
           lda $ffff
           jsr stoptimer
           jsr restorestack
           lda #11
           jmp compare
           .bend


rix
wix
           .block
           lda #$ff
           sta 172
           sta 173
           lda cmd
           sta nowrap
           jsr starttimer
           ldx #0
nowrap
           lda (172,x)
           jsr stoptimer
           lda #8
           jsr compare
           lda cmd
           sta wrap
           jsr starttimer
           ldx #$ff
wrap
           lda (173,x)
           jsr stoptimer
           lda #8
           jmp compare
           .bend


mix
           .block
           lda #$ff
           sta 172
           sta 173
           lda cmd
           sta nowrap
           jsr starttimer
           ldx #0
nowrap
           lda (172,x)
           jsr stoptimer
           lda #10
           jsr compare
           lda cmd
           sta wrap
           jsr starttimer
           ldx #$ff
wrap
           lda (173,x)
           jsr stoptimer
           lda #10
           jmp compare
           .bend


riy
           .block
           lda #$ff
           sta 172
           sta 173
           lda cmd
           sta nowrap
           jsr starttimer
           ldy #0
nowrap
           lda (172),y
           jsr stoptimer
           lda #7
           jsr compare
           lda cmd
           sta wrap
           jsr starttimer
           ldy #3
wrap
           lda (172),y
           jsr stoptimer
           lda #8
           jmp compare
           .bend


wiy
           .block
           lda #$ff
           sta 172
           sta 173
           lda cmd
           sta nowrap
           jsr starttimer
           ldy #0
nowrap
           lda (172),y
           jsr stoptimer
           lda #8
           jsr compare
           lda cmd
           sta wrap
           jsr starttimer
           ldy #3
wrap
           lda (172),y
           jsr stoptimer
           lda #8
           jmp compare
           .bend


miy
           .block
           lda #$ff
           sta 172
           sta 173
           lda cmd
           sta nowrap
           jsr starttimer
           ldy #0
nowrap
           lda (172),y
           jsr stoptimer
           lda #10
           jsr compare
           lda cmd
           sta wrap
           jsr starttimer
           ldy #3
wrap
           lda (172),y
           jsr stoptimer
           lda #10
           jmp compare
           .bend


r
           .block
from       = $2000
to         = $2000
           ldx #$f7
           lda cmd
           sta from-2
           and #$20
           beq clear
           ldx #$34
clear
           txa
           pha
           lda #to-from&$ff
           sta from-1
           lda #$60
           sta to
           jsr starttimer
           plp
           jsr from-2
           jsr stoptimer
           lda #18
           jsr compare
           .bend

           .block
from       = $2000
to         = $2000
           ldx #$34
           lda cmd
           sta from-2
           and #$20
           beq clear
           ldx #$f7
clear
           txa
           pha
           lda #to-from&$ff
           sta from-1
           lda #$60
           sta to
           jsr starttimer
           plp
           jsr from-2
           jsr stoptimer
           lda #19
           jsr compare
           .bend

           .block
from       = $2000
to         = $207f
           ldx #$34
           lda cmd
           sta from-2
           and #$20
           beq clear
           ldx #$f7
clear
           txa
           pha
           lda #to-from&$ff
           sta from-1
           lda #$60
           sta to
           jsr starttimer
           plp
           jsr from-2
           jsr stoptimer
           lda #19
           jsr compare
           .bend

           .block
from       = $1fff
to         = $2000
           ldx #$34
           lda cmd
           sta from-2
           and #$20
           beq clear
           ldx #$f7
clear
           txa
           pha
           lda #to-from&$ff
           sta from-1
           lda #$60
           sta to
           jsr starttimer
           plp
           jsr from-2
           jsr stoptimer
           lda #20
           jsr compare
           .bend

           .block
from       = $1fff
to         = $207e
           ldx #$34
           lda cmd
           sta from-2
           and #$20
           beq clear
           ldx #$f7
clear
           txa
           pha
           lda #to-from&$ff
           sta from-1
           lda #$60
           sta to
           jsr starttimer
           plp
           jsr from-2
           jsr stoptimer
           lda #20
           jsr compare
           .bend

           .block
from       = $1fff
to         = $1ffc
           ldx #$34
           lda cmd
           sta from-2
           and #$20
           beq clear
           ldx #$f7
clear
           txa
           pha
           lda #to-from&$ff
           sta from-1
           lda #$60
           sta to
           jsr starttimer
           plp
           jsr from-2
           jsr stoptimer
           lda #19
           jsr compare
           .bend

           .block
from       = $1fff
to         = $1f7f
           ldx #$34
           lda cmd
           sta from-2
           and #$20
           beq clear
           ldx #$f7
clear
           txa
           pha
           lda #to-from&$ff
           sta from-1
           lda #$60
           sta to
           jsr starttimer
           plp
           jsr from-2
           jsr stoptimer
           lda #19
           jsr compare
           .bend

           .block
from       = $2000
to         = $1ffd
           ldx #$34
           lda cmd
           sta from-2
           and #$20
           beq clear
           ldx #$f7
clear
           txa
           pha
           lda #to-from&$ff
           sta from-1
           lda #$60
           sta to
           jsr starttimer
           plp
           jsr from-2
           jsr stoptimer
           lda #20
           jsr compare
           .bend

           .block
from       = $2000
to         = $1f80
           ldx #$34
           lda cmd
           sta from-2
           and #$20
           beq clear
           ldx #$f7
clear
           txa
           pha
           lda #to-from&$ff
           sta from-1
           lda #$60
           sta to
           jsr starttimer
           plp
           jsr from-2
           jsr stoptimer
           lda #20
           jsr compare
           .bend

           rts


brkn
           lda #$7f
           sta $dc0d
           lda #$35
           sta 1
           lda #<break
           sta $fffe
           lda #>break
           sta $ffff
           jsr starttimer
           brk
break
           jsr stoptimer
           pla
           pla
           pla
           lda #$37
           sta 1
           lda #$81
           sta $dc0d
           lda #7
           jmp compare


phan
phpn
           .block
           lda cmd
           sta command
           jsr starttimer
command
           pha
           jsr stoptimer
           pla
           lda #3
           jmp compare
           .bend


plan
plpn
           .block
           lda cmd
           sta command
           lda #$f7
           pha
           jsr starttimer
command
           pla
           jsr stoptimer
           lda #4
           jmp compare
           .bend


jmpw
           .block
           jsr starttimer
           jmp continue
continue
           jsr stoptimer
           lda #3
           jmp compare
           .bend


jmpi
           .block
           lda #<nowrap
           sta $2000
           lda #>nowrap
           sta $2001
           jsr starttimer
           jmp ($2000)
nowrap
           jsr stoptimer
           lda #5
           jsr compare
           lda #<wrap
           sta $1fff
           lda #>wrap
           sta $1f00
           jsr starttimer
           jmp ($1fff)
wrap
           jsr stoptimer
           lda #5
           jmp compare
           .bend


jsrw
           .block
           jsr starttimer
           jsr continue
continue
           jsr stoptimer
           pla
           pla
           lda #6
           jmp compare
           .bend


rtsn
           .block
           lda #>continue-1
           pha
           lda #<continue-1
           pha
           jsr starttimer
           rts
continue
           jsr stoptimer
           lda #6
           jmp compare
           .bend


rtin
           .block
           lda #>continue
           pha
           lda #<continue
           pha
           sei
           php
           jsr starttimer
           rti
continue
           jsr stoptimer
           lda #6
           jmp compare
           .bend


hltn
           rts


