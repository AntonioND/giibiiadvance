
	INCLUDE	"hardware.inc"
	INCLUDE "header.inc"

	SECTION	"Main",HOME

;--------------------------------------------------------------------------
;- Main()                                                                 -
;--------------------------------------------------------------------------

Main:
	
	ld	hl,$A000
	ld	a,$0A
	ld	[$0000],a ; enable ram
	
	ld	a,[rDIV]
	ld	[hl+],a
	
	ld	a,$FF
	ld	[rTIMA],a
	
	ld	a,IEF_TIMER
	ld	[rIE],a
	xor	a,a
	ld	[rIF],a
	ld	[rTMA],a
	
	ld	b,0
	ld	c,b
	ld	d,b
	ld	e,b
	
	ld	a,TACF_STOP|TACF_4KHZ
	ld	[rTAC],a
	
	ei
	
	ld	a,TACF_START|TACF_4KHZ
	ld	[rTAC],a	
	
	REPT	$255
	inc	b
	ENDR
	REPT	$255
	inc	c
	ENDR
	REPT	$255
	inc	d
	ENDR
	REPT	$255
	inc	e
	ENDR
	REPT	$255
	inc	b
	ENDR
	REPT	$255
	inc	c
	ENDR
	REPT	$255
	inc	d
	ENDR
	
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
	
.endloop:
	halt
	jr	.endloop
