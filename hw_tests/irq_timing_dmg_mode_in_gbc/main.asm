
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
	ld	a,IEF_TIMER
	ld	[rIE],a
	
	ld	a,0
	ld	[rIF],a
	ld	a,TACF_START|TACF_262KHZ
	ld	[rTAC],a
	ld	a,0
	ld	[rDIV],a
	ld	[rTIMA],a
	ld	[rTMA],a
	ld	[rDIV],a
	ei
	
	REPT	24
	
	REPT	$200
	inc	a
	ENDR
	
	di
	ld	b,a
	ld	a,[$FFFE]
	ld	[hl+],a
	ld	a,b
	ei
	
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
