
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
	
	ld	a,LCDCF_ON
	ld	[rLCDC],a
	
	stop
	
	ld	a,$C0
	ld	[rNR11],a
	ld	a,$E0
	ld	[rNR12],a
	ld	a,$00
	ld	[rNR13],a
	ld	a,$81
	ld	[rNR14],a
	
	ld	a,IEF_VBLANK
	ld	[rIE],a
	ei
	
	ld	b,240
	call	wait_frames
	
	xor	a,a
	ld	[rIE],a
	di
	
	call	screen_off
	
	ld	a,$00
	ld	[rP1],a
	
	stop
	
	ld	a,$C0
	ld	[rNR11],a
	ld	a,$E0
	ld	[rNR12],a
	ld	a,$00
	ld	[rNR13],a
	ld	a,$83
	ld	[rNR14],a
	
	;--------------------------------
	
.endloop:
	halt
	jr	.endloop
