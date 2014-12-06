
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

	; -------------------------------------------------------
	
	ld	a,$0A
	ld	[$0000],a ; enable ram

	; -------------------------------------------------------
	
	di
	
	ld	a,$80
	ld	[rTMA],a
	
	; -------------------------------------------------------

PERFORM_TEST : MACRO
	
	ld	a,TACF_STOP|TACF_65KHZ
	ld	[rTAC],a
	ld	a,TACF_START|TACF_65KHZ
	ld	[rTAC],a
	
	ld	[rDIV],a
	ld	a,$F8
	ld	[rTIMA],a
	ld	[rDIV],a

	ld	a,IEF_TIMER
	ld	[rIE],a
	xor	a,a
	ld	[rIF],a
	ei
	halt
	REPT \1
	NOP
	ENDR
	ld	a,[rTIMA]
	ld	[hl+],a
	di
ENDM

REPETITIONS SET 0
	REPT 64
	PERFORM_TEST REPETITIONS
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

.endloop:
	halt
	jr	.endloop
