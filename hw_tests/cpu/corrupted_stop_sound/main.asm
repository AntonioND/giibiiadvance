
	INCLUDE	"hardware.inc"
	INCLUDE "header.inc"

	SECTION	"Main",HOME

;--------------------------------------------------------------------------
;- Main()                                                                 -
;--------------------------------------------------------------------------

Main:
	
	
	ld	a,$80
	ld	[rNR52],a
	ld	a,$FF
	ld	[rNR51],a
	ld	a,$77
	ld	[rNR50],a
	
	ld	a,$C0
	ld	[rNR11],a
	ld	a,$E0
	ld	[rNR12],a
	ld	a,$00
	ld	[rNR13],a
	ld	a,$87
	ld	[rNR14],a
	
	
	ld	a,$00
	ld	[rP1],a
	
	ld	a,$0A
	ld	[$0000],a
	
	ld	hl,$A000
	
VALUE SET 0
	REPT 256
	
	db $10 ; stop
	db VALUE
	
	ld	a,VALUE
	ld	[hl+],a
	
	push	hl
	ld	[hl],$12
	inc	hl
	ld	[hl],$12
	inc	hl
	ld	[hl],$12
	inc	hl
	ld	[hl],$12
	pop	hl
	
VALUE SET VALUE+1
	ENDR
	
	ld	a,$00
	ld	[$0000],a
	
	;--------------------------------
	
.endloop:
	halt
	jr	.endloop
