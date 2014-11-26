
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
	
	ld	a,$30
	ld	[rP1],a
	
	stop
	
	xor	a,a
	ld	[rNR10],a
	ld	[rNR11],a
	ld	[rNR12],a
	ld	[rNR13],a
	ld	[rNR14],a
	ld	[rNR41],a
	ld	[rNR42],a
	ld	[rNR43],a
	ld	[rNR44],a
	
	ld	a,$C0
	ld	[rNR11],a
	ld	a,$E0
	ld	[rNR12],a
	ld	a,$00
	ld	[rNR13],a
	ld	a,$80
	ld	[rNR14],a
	
.loop:
	halt
	jr	.loop
	
	
	
	
	