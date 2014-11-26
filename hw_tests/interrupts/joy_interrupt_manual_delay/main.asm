
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
	
	;--------------------------------
	
	ld	a,LCDCF_ON
	ld	[rLCDC],a
	
	REPT	100
	ld	b,140
	call	wait_ly
	ld	b,139
	call	wait_ly
	ENDR
	
	;--------------------------------
	
	ld	a,0
	ld	[rIF],a
	ld	a,IEF_HILO
	ld	[rIE],a

	;--------------------------------

	ei
	
.end:
	ld	b,1
	call	wait_ly
	ld	b,0
	call	wait_ly
	ld	a,$00
	ld	[rP1],a
	ld	a,$30
	ld	[rP1],a
	inc	a
	inc	a
	inc	a
	inc	a
	inc	a
	jr	.end
