
	INCLUDE	"hardware.inc"
	INCLUDE "header.inc"

	SECTION	"Main",HOME

;--------------------------------------------------------------------------
;- Main()                                                                 -
;--------------------------------------------------------------------------

Main:

	ld	a,$84
	ld	[$FFFE],a
	;--------------------------------
	
	ld	a,$80
	ld	[rNR52],a
	ld	a,$FF
	ld	[rNR51],a
	ld	a,$77
	ld	[rNR50],a
	
	;--------------------------------
	
	ld	a,0
	ld	[rIF],a
	ld	a,IEF_HILO
	ld	[rIE],a

	;--------------------------------
	
	ld	a,$C0
	ld	[rNR11],a
	ld	a,$E0
	ld	[rNR12],a
	ld	a,$00
	ld	[rNR13],a
	ld	a,$87
	ld	[rNR14],a
	
	;--------------------------------
	
	ld	a,$00
	ld	[rP1],a
	
	ei
	
.end:
	halt
	jr	.end
