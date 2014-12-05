
	INCLUDE	"hardware.inc"
	INCLUDE "header.inc"

	
	SECTION	"var",BSS
	
ram_ptr:	DS 2
repeat_loop:	DS 1

	SECTION	"Main",HOME

;--------------------------------------------------------------------------
;- Main()                                                                 -
;--------------------------------------------------------------------------

Main:
	
	ld	hl,$A000
	
	; -------------------------------------------------------
	
	ld	a,$0A
	ld	[$0000],a ; enable ram
	
	; -------------------------------------------------------
	
	ld	a,IEF_TIMER
	ld	[rIE],a
	
	xor	a,a
	ld	[rTMA],a

	ld	a,TACF_STOP|TACF_262KHZ
	ld	[rTAC],a
	ld	a,TACF_START|TACF_262KHZ
	ld	[rTAC],a
	
	di
	
;REPETITIONS_1 SET 0
;	REPT	16
	
REPETITIONS_2 SET 0
	REPT	32
	
	xor	a,a
	ld	[rTIMA],a
	ld	[rIF],a
	dec	a
	dec	a
	ld	[rDIV],a
	ld	[rTIMA],a
	ei
	
;	REPT	REPETITIONS_1
;	inc	a
;	ENDR
	halt
	REPT	REPETITIONS_2
	nop
	ENDR
	ld	a,[rTIMA]
	ld	[hl+],a

REPETITIONS_2 SET REPETITIONS_2+1
	ENDR
	
;REPETITIONS_1 SET REPETITIONS_1+1
;	ENDR
	
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

.endloop:
	halt
	jr	.endloop
