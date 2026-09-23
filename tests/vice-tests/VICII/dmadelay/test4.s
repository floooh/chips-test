
                !to "test4.prg", cbm

*=$0801
basic:
; BASIC stub: "1 SYS 2061"
!by $0b,$08,$01,$00,$9e,$32,$30,$36,$31,$00,$00,$00

; ---------------------------------------------------------------------------

init:
                sei
                ldx #0
lp:
                lda #1
                sta $d800,x
                sta $d900,x
                sta $da00,x
                sta $db00,x
                lda #$20
                sta $0400,x
                sta $0500,x
                sta $0600,x
                sta $0700,x
                inx
                bne lp
                
                lda #$80
                sta $3fff

                LDX     #$2E
loc_1FE3:
                LDA     vicregtab,X
                STA     $D000,X
                DEX
                BPL     loc_1FE3

                LDX     #7
loc_1FEE:
                LDA     sprptrtab,X
                STA     $7F8,X
                DEX
                BPL     loc_1FEE

                LDX     #$C0
loc_1FF9:
                TXA
                STA     $380,X
                INX
                BNE     loc_1FF9

                jmp mainloop

; ---------------------------------------------------------------------------

sprptrtab:	!byte $11
		!byte $11
		!byte $11
		!byte $11
		!byte $11
		!byte $11
		!byte $11
		!byte $11

vicregtab:	!byte 0
		!byte $38
		!byte $40
		!byte $38
		!byte $80
		!byte $38
		!byte $C0
		!byte $38
		
		!byte 0
		!byte $38
		!byte $40
		!byte $38
		!byte $80
		!byte $38
		!byte $C0
		!byte $38

		!byte $F0
		!byte $1B
		!byte	0
		!byte	0
		!byte	0
		!byte $FF
		!byte $C8
		!byte $FF
		
		!byte $17
		!byte $70
		!byte $F1
		!byte	0
		!byte $FF
		!byte $FF
		!byte $C8
		!byte	0
		
		!byte $FD
		!byte $FB
		!byte $F1
		!byte $F2
		!byte $F3
		!byte $F0
		!byte $F1
		!byte $FF
		
		!byte $F3
		!byte $F3
		!byte $F3
		!byte $F3
		!byte $F3
		!byte $F3
		!byte $FF
		!byte $FF
; ---------------------------------------------------------------------------
                * = $0900
mainloop:

                ; wait for line $24
loc_2009:
		LDA	#$24
loc_200B:
		CMP	$D012
		BNE	loc_200B
		LDA	$D011
		BMI	loc_2009

		; stabilize (using lightpen)
		LDA	#$FF
		STA	$DC01
		STA	$DC03
		LDA	#$EF
		STA	$DC01
		LDA	#$FF
		STA	$DC01
		LDA	#0
		STA	$DC03
		LDA	$D013
		LSR
		LSR
		SEC
		SBC	#$C
		AND	#7
		STA	branchloc+1

branchloc:
		BPL	branchloc

		CMP	#$C9
		CMP	#$C9
		CMP	#$C9
		BIT	$EA
		BIT	$EA

                ; delay
		INC	$D021
		LDX	#$EC
loc_204A:
		DEX
		BNE	loc_204A

		DEC	$D021
		NOP

		LDX	#$36 ; num lines

loc_2053:
		DEC	$D016
		INC	$D016
		LDA	$D011
		ADC	#1
		AND	#7
		ORA	#$18
		STA	$D011
		BIT	$EA
		INC	$D021
		DEC	$D021
		DEX
		BNE	loc_2053

		LDA	#$1B
		STA	$D011

		JMP	mainloop

