
	INCLUDE	"hardware.inc"
	INCLUDE "header.inc"

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


REG_TEST: MACRO
	ld	b,0
.loop\@:
	ld	a,b
	ld	[\1],a
	ld	a,[\1]
	ld	[hl+],a
	inc	b
	jr	nz,.loop\@
ENDM
	
	REG_TEST	rTMA
	REG_TEST	rTIMA
	REG_TEST	rTAC
	
	ld	a,0
	ld	[rTAC],a
	
	REG_TEST	rDIV
	REG_TEST	rIE
	
	ld	a,0
	ld	[rIE],a
	
	REG_TEST	rIF
	
	ld	a,0
	ld	[rIF],a
	
	REG_TEST	rLY ; screen OFF
	REG_TEST	rLYC

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
