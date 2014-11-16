
	INCLUDE	"hardware.inc"
	INCLUDE "header.inc"

	
	SECTION	"var",BSS
	
repeat_loop:	DS 1

	SECTION	"Main",HOME

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

.repeat_all:
	
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
	
REPETITIONS SET 0
	REPT	16
	
	xor	a,a
	ld	[rTAC],a
	ld	[rDIV],a
	ld	[rTIMA],a
	ld	[rTMA],a
	ld	[rDIV],a
	
	ld	a,TACF_STOP|TACF_65KHZ
	ld	[rTAC],a
	ld	a,TACF_START|TACF_65KHZ
	ld	[rTAC],a
	
	ld	a,[rTIMA]
	ld	[hl+],a
	ld	[rDIV],a
	
	REPT	15
	
	REPT REPETITIONS
	nop
	ENDR
	
	ld	[rDIV],a
	
	ENDR
	
	ld	a,[rTIMA]
	ld	[hl+],a

REPETITIONS SET REPETITIONS+1
	ENDR
	
	; -------------------------------------------------------
	
REPETITIONS SET 0
	REPT	32
	
	xor	a,a
	ld	[rTAC],a
	ld	[rDIV],a
	ld	[rTIMA],a
	ld	[rTMA],a
	ld	[rDIV],a
	
	ld	a,TACF_STOP|TACF_65KHZ
	ld	[rTAC],a
	ld	a,TACF_START|TACF_65KHZ
	ld	[rTAC],a
	
	ld	a,[rTIMA]
	ld	[hl+],a
	ld	[rDIV],a
	
	REPT	15
	
	REPT REPETITIONS
	nop
	ENDR
	
	ld	a,[rTIMA]
	ld	[hl+],a
	ld	[rDIV],a
	
	ENDR

REPETITIONS SET REPETITIONS+1
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
	jp	.repeat_all
.dontchange:
	
.endloop:
	halt
	jr	.endloop
