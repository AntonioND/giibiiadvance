
	INCLUDE	"hardware.inc"
	INCLUDE "header.inc"

	
	SECTION	"var",BSS
	
repeat_loop:	DS 1

	SECTION	"Main",ROM0

;--------------------------------------------------------------------------
;- Main()                                                                 -
;--------------------------------------------------------------------------

Main:
	
	ld	hl,$A000
	
	ld	a,[Init_Reg_A]
	cp	a,$11
	jr	nz,.skip1
	ld	a,0
	ld	[repeat_loop],a
	call	CPU_slow
.skip1:

repeat_all:
	
	; -------------------------------------------------------
	
	ld	a,$0A
	ld	[$0000],a ; enable ram
	
	; -------------------------------------------------------

;	di
;	ld	a,IEF_TIMER
;	ld	[rIE],a
;	ld	[rIF],a
;	xor	a,a
;	ei
	
;	REPT	256
;	inc	a
;	ENDR

	; -------------------------------------------------------
	
REPETITIONS_1 SET 0
	REPT	16

REPETITIONS_2 SET 0
	REPT	16
	
	ld	a,(REPETITIONS_1<<4)|REPETITIONS_2
	ld	[hl+],a
	
	xor	a,a
	ld	[rTAC],a
	ld	[rDIV],a
	ld	[rTIMA],a
	ld	[rTMA],a
	ld	[rDIV],a
	ld	c,rDIV & $FF
	ld	de,rTIMA
	
	ld	a,TACF_STOP|TACF_65KHZ
	ld	[rTAC],a
	ld	a,TACF_START|TACF_65KHZ
	ld	[rTAC],a
	
	xor	a,a
	ld	[$FF00+c],a
	ld	[de],a
	ld	[$FF00+c],a
	
	ld	a,[de]
	ld	[hl+],a
	
	REPT REPETITIONS_1
	nop
	ENDR
	
	ld	[$FF00+c],a
	
	REPT REPETITIONS_2
	nop
	ENDR
	
	ld	a,[de]
	ld	[hl+],a
	
	ld	[$FF00+c],a
	
	ld	a,[de]
	ld	[hl+],a


REPETITIONS_2 SET REPETITIONS_2+1
	ENDR

REPETITIONS_1 SET REPETITIONS_1+1
	ENDR
	
	jp	Main2

	; -------------------------------------------------------
	
	SECTION	"Main2",ROMX
	
Main2:

	; -------------------------------------------------------
	
REPETITIONS_1 SET 0
	REPT	16

REPETITIONS_2 SET 0
	REPT	16
	
	ld	a,(REPETITIONS_1<<4)|REPETITIONS_2
	ld	[hl+],a
	
	xor	a,a
	ld	[rTAC],a
	ld	[rDIV],a
	ld	[rTIMA],a
	ld	[rTMA],a
	ld	[rDIV],a
	ld	c,rDIV & $FF
	ld	de,rTIMA
	
	ld	a,TACF_STOP|TACF_65KHZ
	ld	[rTAC],a
	ld	a,TACF_START|TACF_65KHZ
	ld	[rTAC],a
	
	xor	a,a
	ld	[$FF00+c],a
	ld	[de],a
	ld	[$FF00+c],a
	
	REPT REPETITIONS_1
	nop
	ENDR
	
	ld	a,[de]
	ld	[hl+],a
	
	ld	[$FF00+c],a
	
	REPT REPETITIONS_2
	nop
	ENDR
	
	ld	a,[de]
	ld	[hl+],a
	
	ld	[$FF00+c],a
	
	ld	a,[de]
	ld	[hl+],a


REPETITIONS_2 SET REPETITIONS_2+1
	ENDR

REPETITIONS_1 SET REPETITIONS_1+1
	ENDR
	
	; -------------------------------------------------------
	
	push	hl ; magic number
	ld	[hl],$12
	inc hl
	ld	[hl],$34
	inc hl
	ld	[hl],$56
	inc hl
	ld	[hl],$78
	pop	hl
	
	ld	a,$00
	ld	[$0000],a ; disable ram
	
	; -------------------------------------------------------
	
	ld	a,[Init_Reg_A]
	cp	a,$11
	jr	nz,.dontchange
	ld	a,[repeat_loop]
	and	a,a
	jr	nz,.endloop
	; -------------------------------------------------------

	call	CPU_fast
	ld	a,1
	ld	[repeat_loop],a
	jp	repeat_all
.dontchange:
	
.endloop:
	halt
	jr	.endloop
