.const outputline = $0400

// 10 SYS2061



*=$0801
	.byte $0B, $08, $0A, $00, $9E, $32, $30, $36, $31, $00, $00, $00

*=$080d
	// set to 25 line text mode and turn on the screen
	lda #$1B
	sta $D011

	// disable SHIFT-Commodore
	lda #$80
	sta $0291

	// set screen memory ($0400) and charset bitmap offset ($2000)
	lda #$17
	sta $D018

	// set border color
	lda #$00
	sta $D020
	
	// set background color
	lda #$00
	sta $D021

	jsr $e544

!loop:
	jsr $ffe4

	cmp #$41
	bne !+
	jmp fire1
!:	cmp #$42
	bne !+
	jmp fire2
!: cmp #$43
	bne !+
	jmp fire3
!:	cmp #$44
	bne !+
	jmp fire4
!:	cmp #$45
	bne !+
	jmp fire5
!:	cmp #$46
	bne !+
	jmp fire6
!:	cmp #$47
	bne !+
	jmp fire7
!: cmp #$48
	bne !+
	jmp fire8
!:	cmp #$49
	bne !+
	jmp fire9
!:	cmp #$4a
	bne !+
	jmp fire10
!:	cmp #$4b
	bne !+
	jmp fire11
!:	cmp #$4c
	bne !+
	jmp fire12
!: cmp #$4d
	bne !+
	jmp fire13
!:	cmp #$4e
	bne !+
	jmp fire14
!:	cmp #$4f
	bne !+
	jmp fire15
!:	cmp #$50
	bne !+
	jmp fire16
!:	cmp #$51
	bne !+
	jmp fire17
!: cmp #$52
	bne !+
	jmp fire18
!:	cmp #$53
	bne !+
	jmp fire19
!:	cmp #$54
	bne !+
	jmp fire20
!:	cmp #$55
	bne !+
	jmp fire21
!:	cmp #$56
	bne !+
	jmp fire22
!: cmp #$57
	bne !+
	jmp fire23
!:	cmp #$58
	bne !+
	jmp fire24
!:	cmp #$59
	bne !+
	jmp fire25
!:	cmp #$5a
	bne !+
	jmp fire26	
!:	cmp #$20
	bne !+
	jmp fire_space

!: jmp !loop-	


fire1:
	sta $d020
	ldx #$00
!print:	
	lda cue_1,x
	sta outputline,x
	inx
	cpx #11
	bne !print-
	jmp !loop-
fire2:
	sta $d020
	ldx #$00
!print:	
	lda cue_2,x
	sta outputline,x
	inx
	cpx #11
	bne !print-

	jmp !loop-
fire3:
	sta $d020
	ldx #$00
!print:	
	lda cue_3,x
	sta outputline,x
	inx
	cpx #11
	bne !print-
	jmp !loop-
fire4:
	sta $d020
	ldx #$00
!print:	
	lda cue_4,x
	sta outputline,x
	inx
	cpx #11
	bne !print-

	jmp !loop-
fire5:
	sta $d020
	ldx #$00
!print:	
	lda cue_5,x
	sta outputline,x
	inx
	cpx #11
	bne !print-

	jmp !loop-
fire6:
	sta $d020
	ldx #$00
!print:	
	lda cue_6,x
	sta outputline,x
	inx
	cpx #11
	bne !print-

	jmp !loop-
fire7:
	sta $d020
	ldx #$00
!print:	
	lda cue_7,x
	sta outputline,x
	inx
	cpx #11
	bne !print-

	jmp !loop-
fire8:
	sta $d020
	ldx #$00
!print:	
	lda cue_8,x
	sta outputline,x
	inx
	cpx #11
	bne !print-

	jmp !loop-
fire9:
	sta $d020
	ldx #$00
!print:	
	lda cue_9,x
	sta outputline,x
	inx
	cpx #11
	bne !print-

	jmp !loop-
fire10:
	sta $d020
	ldx #$00
!print:	
	lda cue_10,x
	sta outputline,x
	inx
	cpx #11
	bne !print-

	jmp !loop-
fire11:
	sta $d020
	ldx #$00
!print:	
	lda cue_11,x
	sta outputline,x
	inx
	cpx #11
	bne !print-

	jmp !loop-
fire12:
	sta $d020
	ldx #$00
!print:	
	lda cue_12,x
	sta outputline,x
	inx
	cpx #11
	bne !print-

	jmp !loop-
fire13:
	sta $d020
	ldx #$00
!print:	
	lda cue_13,x
	sta outputline,x
	inx
	cpx #11
	bne !print-

	jmp !loop-
fire14:
	sta $d020
	ldx #$00
!print:	
	lda cue_14,x
	sta outputline,x
	inx
	cpx #11
	bne !print-

	jmp !loop-
fire15:
	sta $d020
	ldx #$00
!print:	
	lda cue_15,x
	sta outputline,x
	inx
	cpx #11
	bne !print-

	jmp !loop-
fire16:
	sta $d020
	ldx #$00
!print:	
	lda cue_16,x
	sta outputline,x
	inx
	cpx #11
	bne !print-

	jmp !loop-
fire17:
	sta $d020
	ldx #$00
!print:	
	lda cue_17,x
	sta outputline,x
	inx
	cpx #11
	bne !print-

	jmp !loop-
fire18:
	sta $d020
	ldx #$00
!print:	
	lda cue_18,x
	sta outputline,x
	inx
	cpx #11
	bne !print-

	jmp !loop-
fire19:
	sta $d020
	ldx #$00
!print:	
	lda cue_19,x
	sta outputline,x
	inx
	cpx #11
	bne !print-

	jmp !loop-
fire20:
	sta $d020
	ldx #$00
!print:	
	lda cue_20,x
	sta outputline,x
	inx
	cpx #11
	bne !print-

	jmp !loop-
fire21:
	sta $d020
	ldx #$00
!print:	
	lda cue_21,x
	sta outputline,x
	inx
	cpx #11
	bne !print-

	jmp !loop-
fire22:
	sta $d020
	ldx #$00
!print:	
	lda cue_22,x
	sta outputline,x
	inx
	cpx #11
	bne !print-

	jmp !loop-
fire23:
	sta $d020
	ldx #$00
!print:	
	lda cue_23,x
	sta outputline,x
	inx
	cpx #11
	bne !print-

	jmp !loop-
fire24:
	sta $d020
	ldx #$00
!print:	
	lda cue_24,x
	sta outputline,x
	inx
	cpx #11
	bne !print-

	jmp !loop-
fire25:
	sta $d020
	ldx #$00
!print:	
	lda cue_25,x
	sta outputline,x
	inx
	cpx #11
	bne !print-

	jmp !loop-
fire26:
	sta $d020
	ldx #$00
!print:	
	lda cue_26,x
	sta outputline,x
	inx
	cpx #11
	bne !print-

	jmp !loop-	
fire30:
	sta $d020
	jmp !loop-	
fire31:
	sta $d020
	jmp !loop-			
fire32:
	sta $d020
	jmp !loop-	
fire33:
	sta $d020
	jmp !loop-			
fire34:
	sta $d020
	jmp !loop-	
fire35:
	sta $d020
	jmp !loop-	
fire36:
	sta $d020
	jmp !loop-	
fire37:
	sta $d020
	jmp !loop-					
fire38:
	sta $d020
	jmp !loop-	
fire39:
	sta $d020
	jmp !loop-			
fire_space:
	sta $d020
	jmp !loop-


cue_1:
.text "/c64/key/a"
.byte 128
cue_2:
.text "/c64/key/b"
.byte 128
cue_3:
.text "/c64/key/c"
.byte 128
cue_4:
.text "/c64/key/d"
.byte 128
cue_5:
.text "/c64/key/e"
.byte 128
cue_6:
.text "/c64/key/f"
.byte 128
cue_7:
.text "/c64/key/g"
.byte 128
cue_8:
.text "/c64/key/h"
.byte 128
cue_9:
.text "/c64/key/i"
.byte 128
cue_10:
.text "/c64/key/j"
.byte 128
cue_11:
.text "/c64/key/k"
.byte 128
cue_12:
.text "/c64/key/l"                              
.byte 128
cue_13:
.text "/c64/key/m"
.byte 128
cue_14:
.text "/c64/key/n"
.byte 128
cue_15:
.text "/c64/key/o"
.byte 128
cue_16:
.text "/c64/key/p"
.byte 128
cue_17:
.text "/c64/key/q"
.byte 128
cue_18:
.text "/c64/key/r"
.byte 128
cue_19:
.text "/c64/key/s"
.byte 128
cue_20:
.text "/c64/key/t"
.byte 128
cue_21:
.text "/c64/key/u"
.byte 128
cue_22:
.text "/c64/key/v"
.byte 128
cue_23:
.text "/c64/key/w"
.byte 128
cue_24:
.text "/c64/key/x"
.byte 128
cue_25:
.text "/c64/key/y"
.byte 128
cue_26:
.text "/c64/key/z"
.byte 128

cue_31:
.text "/c64/key/1"
.byte 128
cue_32:
.text "/c64/key/2"
.byte 128
cue_33:
.text "/c64/key/3"
.byte 128
cue_34:
.text "/c64/key/4"
.byte 128
cue_35:
.text "/c64/key/5"
.byte 128
cue_36:
.text "/c64/key/6"
.byte 128
cue_37:
.text "/c64/key/7"
.byte 128
cue_38:
.text "/c64/key/8"
.byte 128
cue_39:
.text "/c64/key/9"
.byte 128
cue_30:
.text "/c64/key/0"
.byte 128







	// draw screen
	ldx #$00
!:        
	lda screen_layout,x
	sta $0400,x
        lda screen_layout+$100,x
        sta $0500,x
        lda screen_layout+$200,x
        sta $0600,x
        lda screen_layout+$300,x
        sta $0700,x
        lda color_layout,x
        sta $d800,x
        lda color_layout+$100,x
        sta $d900,x
        lda color_layout+$200,x
        sta $da00,x
        lda color_layout+$300,x
        sta $db00,x
        inx
        bne !-

	// wait for keypress
	lda $c6
	beq *-2

	rts


// screen character data
*=$2800
screen_layout:
	.byte	$A0, $A0, $A0, $A0, $A0, $A0, $A0, $A0, $A0, $A0, $C6, $BA, $B0, $B0, $B0, $A0, $A0, $A0, $A0, $A0, $A0, $A0, $A0, $A0, $A0, $A0, $A0, $D4, $89, $8D, $85, $A0, $B1, $B9, $BA, $B1, $B2, $BA, $B2, $B3
	.byte	$70, $31, $40, $40, $40, $40, $40, $40, $40, $40, $40, $40, $40, $40, $40, $40, $40, $40, $40, $40, $40, $40, $40, $40, $40, $40, $40, $40, $40, $40, $40, $40, $40, $40, $40, $40, $40, $40, $40, $6E
	.byte	$20, $2F, $07, $0F, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20
	.byte	$20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20
	.byte	$6D, $40, $54, $4F, $44, $3A, $30, $30, $3A, $30, $30, $3A, $30, $30, $40, $40, $46, $3A, $30, $30, $30, $40, $40, $4E, $05, $18, $14, $20, $43, $15, $05, $3A, $30, $32, $40, $40, $40, $40, $40, $7D
	.byte	$70, $32, $40, $40, $40, $40, $40, $40, $40, $40, $40, $40, $40, $40, $40, $40, $40, $40, $40, $40, $40, $40, $40, $40, $40, $40, $40, $40, $40, $40, $40, $40, $40, $40, $40, $40, $40, $40, $40, $6E
	.byte	$20, $2F, $16, $0F, $0C, $20, $30, $2E, $35, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20
	.byte	$20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20
	.byte	$6D, $40, $54, $4F, $44, $3A, $30, $30, $3A, $30, $30, $3A, $30, $30, $40, $40, $46, $3A, $30, $30, $30, $40, $40, $4E, $05, $18, $14, $20, $43, $15, $05, $3A, $30, $33, $40, $40, $40, $40, $40, $7D
	.byte	$70, $33, $40, $40, $40, $40, $40, $40, $40, $40, $40, $40, $40, $40, $40, $40, $40, $40, $40, $40, $40, $40, $40, $40, $40, $40, $40, $40, $40, $40, $40, $40, $40, $40, $40, $40, $40, $40, $40, $6E
	.byte	$20, $2F, $03, $0F, $0D, $10, $0F, $13, $09, $14, $09, $0F, $0E, $13, $2F, $0C, $01, $19, $05, $12, $13, $2F, $31, $2F, $03, $0C, $09, $10, $13, $2F, $33, $2F, $03, $0F, $0E, $0E, $05, $03, $14, $20
	.byte	$20, $20, $31, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20
	.byte	$6D, $40, $54, $4F, $44, $3A, $30, $30, $3A, $30, $30, $3A, $30, $30, $40, $40, $46, $3A, $30, $30, $30, $40, $40, $4E, $05, $18, $14, $20, $43, $15, $05, $3A, $30, $34, $40, $40, $40, $40, $40, $7D
	.byte	$70, $34, $40, $40, $40, $40, $40, $40, $40, $40, $40, $40, $40, $40, $40, $40, $40, $40, $40, $40, $40, $40, $40, $40, $40, $40, $40, $40, $40, $40, $40, $40, $40, $40, $40, $40, $40, $40, $40, $6E
	.byte	$20, $2F, $03, $0F, $0D, $10, $0F, $13, $09, $14, $09, $0F, $0E, $13, $2F, $0C, $01, $19, $05, $12, $13, $2F, $32, $2F, $03, $0C, $09, $10, $13, $2F, $32, $2F, $03, $0F, $0E, $0E, $05, $03, $14, $20
	.byte	$20, $20, $31, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20
	.byte	$6D, $40, $54, $4F, $44, $3A, $30, $30, $3A, $30, $30, $3A, $30, $30, $40, $40, $46, $3A, $30, $30, $30, $40, $40, $4E, $05, $18, $14, $20, $43, $15, $05, $3A, $30, $35, $40, $40, $40, $40, $40, $7D
	.byte	$70, $35, $40, $40, $40, $40, $40, $40, $40, $40, $40, $40, $40, $40, $40, $40, $40, $40, $40, $40, $40, $40, $40, $40, $40, $40, $40, $40, $40, $40, $40, $40, $40, $40, $40, $40, $40, $40, $40, $6E
	.byte	$20, $2F, $03, $0F, $0D, $10, $0F, $13, $09, $14, $09, $0F, $0E, $13, $2F, $0C, $01, $19, $05, $12, $13, $2F, $33, $2F, $03, $0C, $09, $10, $13, $2F, $31, $2F, $03, $0F, $0E, $0E, $05, $03, $14, $20
	.byte	$20, $20, $31, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20
	.byte	$6D, $40, $54, $4F, $44, $3A, $30, $30, $3A, $30, $30, $3A, $30, $30, $40, $40, $46, $3A, $30, $30, $30, $40, $40, $4E, $05, $18, $14, $20, $43, $15, $05, $3A, $30, $36, $40, $40, $40, $40, $40, $7D
	.byte	$70, $36, $40, $40, $40, $40, $40, $40, $40, $40, $40, $40, $40, $40, $40, $40, $40, $40, $40, $40, $40, $40, $40, $40, $40, $40, $40, $40, $40, $40, $40, $40, $40, $40, $40, $40, $40, $40, $40, $6E
	.byte	$20, $2F, $03, $0F, $0D, $10, $0F, $13, $09, $14, $09, $0F, $0E, $13, $2F, $0C, $01, $19, $05, $12, $13, $2F, $32, $2F, $03, $0C, $09, $10, $13, $2F, $31, $2F, $03, $0F, $0E, $0E, $05, $03, $14, $20
	.byte	$20, $20, $31, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20, $20
	.byte	$6D, $40, $54, $4F, $44, $3A, $30, $30, $3A, $30, $30, $3A, $30, $30, $40, $40, $46, $3A, $30, $30, $30, $40, $40, $4E, $05, $18, $14, $20, $43, $15, $05, $3A, $30, $37, $40, $40, $40, $40, $40, $7D

// screen color data
*=$2be8
color_layout:
	.byte	$0B, $0B, $0B, $0B, $0B, $0B, $0B, $0B, $0B, $0B, $0B, $0B, $0B, $0B, $0B, $0B, $0B, $0B, $0B, $0B, $0B, $0B, $0B, $0B, $0B, $0B, $0B, $0B, $0B, $0B, $0B, $0B, $0B, $0B, $0B, $0B, $0B, $0B, $0B, $0B
	.byte	$0B, $03, $0B, $0B, $0B, $0B, $0B, $0B, $0B, $0B, $0B, $0B, $0B, $0B, $0B, $0B, $0B, $0B, $0B, $0B, $0B, $0B, $0B, $0B, $0B, $0B, $0B, $0B, $0B, $0B, $0B, $0B, $0B, $0B, $0B, $0B, $0B, $0B, $0B, $0B
	.byte	$0B, $0F, $0F, $0F, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0B
	.byte	$0B, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0B
	.byte	$0B, $0B, $05, $05, $05, $05, $07, $07, $07, $07, $07, $07, $07, $07, $0B, $0B, $05, $05, $07, $07, $07, $0B, $0B, $05, $05, $05, $05, $0E, $05, $05, $05, $05, $07, $07, $0B, $0B, $0B, $0B, $0B, $0B
	.byte	$0B, $03, $0B, $0B, $0B, $0B, $0B, $0B, $0B, $0B, $0B, $0B, $0B, $0B, $0B, $0B, $0B, $0B, $0B, $0B, $0B, $0B, $0B, $0B, $0B, $0B, $0B, $0B, $0B, $0B, $0B, $0B, $0B, $0B, $0B, $0B, $0B, $0B, $0B, $0B
	.byte	$0B, $0F, $0F, $0F, $0F, $0E, $0F, $0F, $0F, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0B
	.byte	$0B, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0B
	.byte	$0B, $0B, $05, $05, $05, $05, $07, $07, $07, $07, $07, $07, $07, $07, $0B, $0B, $05, $05, $07, $07, $07, $0B, $0B, $05, $05, $05, $05, $0E, $05, $05, $05, $05, $07, $07, $0B, $0B, $0B, $0B, $0B, $0B
	.byte	$02, $03, $02, $02, $02, $02, $02, $02, $02, $02, $02, $02, $02, $02, $02, $02, $02, $02, $02, $02, $02, $02, $02, $02, $02, $02, $02, $02, $02, $02, $02, $02, $02, $02, $02, $02, $02, $02, $02, $02
	.byte	$0F, $01, $01, $01, $01, $01, $01, $01, $01, $01, $01, $01, $01, $01, $01, $01, $01, $01, $01, $01, $01, $01, $01, $01, $01, $01, $01, $01, $01, $01, $01, $01, $01, $01, $01, $01, $01, $01, $01, $0F
	.byte	$0F, $01, $01, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0F
	.byte	$02, $02, $05, $05, $05, $05, $07, $07, $07, $07, $07, $07, $07, $07, $02, $02, $05, $05, $07, $07, $07, $02, $02, $05, $05, $05, $05, $0E, $05, $05, $05, $05, $07, $07, $02, $02, $02, $02, $02, $02
	.byte	$0B, $03, $0B, $0B, $0B, $0B, $0B, $0B, $0B, $0B, $0B, $0B, $0B, $0B, $0B, $0B, $0B, $0B, $0B, $0B, $0B, $0B, $0B, $0B, $0B, $0B, $0B, $0B, $0B, $0B, $0B, $0B, $0B, $0B, $0B, $0B, $0B, $0B, $0B, $0B
	.byte	$0B, $0F, $0F, $0F, $0F, $0F, $0F, $0F, $0F, $0F, $0F, $0F, $0F, $0F, $0F, $0F, $0F, $0F, $0F, $0F, $0F, $0F, $0F, $0F, $0F, $0F, $0F, $0F, $0F, $0F, $0F, $0F, $0F, $0F, $0F, $0F, $0F, $0F, $0F, $0B
	.byte	$0B, $0E, $0F, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0B
	.byte	$0B, $0B, $05, $05, $05, $05, $07, $07, $07, $07, $07, $07, $07, $07, $0B, $0B, $05, $05, $07, $07, $07, $0B, $0B, $05, $05, $05, $05, $0E, $05, $05, $05, $05, $07, $07, $0B, $0B, $0B, $0B, $0B, $0B
	.byte	$0B, $03, $0B, $0B, $0B, $0B, $0B, $0B, $0B, $0B, $0B, $0B, $0B, $0B, $0B, $0B, $0B, $0B, $0B, $0B, $0B, $0B, $0B, $0B, $0B, $0B, $0B, $0B, $0B, $0B, $0B, $0B, $0B, $0B, $0B, $0B, $0B, $0B, $0B, $0B
	.byte	$0B, $0F, $0F, $0F, $0F, $0F, $0F, $0F, $0F, $0F, $0F, $0F, $0F, $0F, $0F, $0F, $0F, $0F, $0F, $0F, $0F, $0F, $0F, $0F, $0F, $0F, $0F, $0F, $0F, $0F, $0F, $0F, $0F, $0F, $0F, $0F, $0F, $0F, $0F, $0B
	.byte	$0B, $0E, $0F, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0B
	.byte	$0B, $0B, $05, $05, $05, $05, $07, $07, $07, $07, $07, $07, $07, $07, $0B, $0B, $05, $05, $07, $07, $07, $0B, $0B, $05, $05, $05, $05, $0E, $05, $05, $05, $05, $07, $07, $0B, $0B, $0B, $0B, $0B, $0B
	.byte	$0B, $03, $0B, $0B, $0B, $0B, $0B, $0B, $0B, $0B, $0B, $0B, $0B, $0B, $0B, $0B, $0B, $0B, $0B, $0B, $0B, $0B, $0B, $0B, $0B, $0B, $0B, $0B, $0B, $0B, $0B, $0B, $0B, $0B, $0B, $0B, $0B, $0B, $0B, $0B
	.byte	$0B, $0F, $0F, $0F, $0F, $0F, $0F, $0F, $0F, $0F, $0F, $0F, $0F, $0F, $0F, $0F, $0F, $0F, $0F, $0F, $0F, $0F, $0F, $0F, $0F, $0F, $0F, $0F, $0F, $0F, $0F, $0F, $0F, $0F, $0F, $0F, $0F, $0F, $0F, $0B
	.byte	$0B, $0E, $0F, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0E, $0B
	.byte	$0B, $0B, $05, $05, $05, $05, $07, $07, $07, $07, $07, $07, $07, $07, $0B, $0B, $05, $05, $07, $07, $07, $0B, $0B, $05, $05, $05, $05, $0E, $05, $05, $05, $05, $07, $07, $0B, $0B, $0B, $0B, $0B, $0B