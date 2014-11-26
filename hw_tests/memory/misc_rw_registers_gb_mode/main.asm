
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
	
	REG_TEST	rP1
	REG_TEST	rSB
	REG_TEST	rSC
	
	ld	a,0
	ld	[rSC],a
	
	REG_TEST	rSTAT ; lcd off
	REG_TEST	rHDMA1
	REG_TEST	rHDMA2
	REG_TEST	rHDMA3
	REG_TEST	rHDMA4
	
	REG_TEST	rSVBK
	REG_TEST	rVBK
	
	REG_TEST	rKEY1
	
	REG_TEST	rRP
	
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
