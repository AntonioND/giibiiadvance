
	INCLUDE	"hardware.inc"
	INCLUDE "header.inc"

	SECTION	"Main",HOME

;--------------------------------------------------------------------------
;- Main()                                                                 -
;--------------------------------------------------------------------------

Main:
	
	ld	hl,$A000
	
	ld	a,LCDCF_ON
	ld	[rLCDC],a
	
	; -------------------------------------------------------
	
	ld	a,$0A
	ld	[$0000],a ; enable ram

	; -------------------------------------------------------
	
	di
	ld	a,0
	ld	[rIE],a
	ld	a,0
	ld	[rIF],a
	ld	a,$FF
	ld	[rTIMA],a
	ld	[rDIV],a
	ld	a,TACF_STOP|TACF_65KHZ
	ld	[rTAC],a
	ld	a,TACF_START|TACF_65KHZ
	ld	[rTAC],a
	
	REPT 16
	nop
	ENDR
	
	ld	a,[rIF]
	ld	[hl+],a
	
	di
	ld	a,0
	ld	[rIE],a
	ld	a,0
	ld	[rIF],a
	ld	a,$FF
	ld	[rTIMA],a
	ld	[rDIV],a
	ld	a,TACF_STOP|TACF_65KHZ
	ld	[rTAC],a
	ld	a,TACF_START|TACF_65KHZ
	ld	[rTAC],a
	ei
	
	REPT 16
	nop
	ENDR
	
	ld	a,[rIF]
	ld	[hl+],a
	
	di
	ld	a,IEF_TIMER
	ld	[rIE],a
	ld	a,0
	ld	[rIF],a
	ld	a,$FF
	ld	[rTIMA],a
	ld	[rDIV],a
	ld	a,TACF_STOP|TACF_65KHZ
	ld	[rTAC],a
	ld	a,TACF_START|TACF_65KHZ
	ld	[rTAC],a
	
	REPT 16
	nop
	ENDR
	
	ld	a,[rIF]
	ld	[hl+],a


	di
	ld	a,IEF_TIMER
	ld	[rIE],a
	ld	a,0
	ld	[rIF],a
	ld	a,$FF
	ld	[rTIMA],a
	ld	[rDIV],a
	ld	a,TACF_STOP|TACF_65KHZ
	ld	[rTAC],a
	ld	a,TACF_START|TACF_65KHZ
	ld	[rTAC],a
	ei
	
	REPT 16
	nop
	ENDR
	
	ld	a,[rIF]
	ld	[hl+],a
	
	; -------------------------------------------------------
	
	ld	b,130
	call	wait_ly
	
	di
	ld	a,IEF_VBLANK
	ld	[rIE],a
	ld	a,0
	ld	[rIF],a
	ld	a,$FF
	ld	[rTIMA],a
	ld	[rDIV],a
	ld	a,TACF_STOP|TACF_65KHZ
	ld	[rTAC],a
	ld	a,TACF_START|TACF_65KHZ
	ld	[rTAC],a
	
	halt
	
	ld	a,[rIF]
	ld	[hl+],a
	
	
	
	ld	b,130
	call	wait_ly
	
	di
	ld	a,IEF_VBLANK
	ld	[rIE],a
	ld	a,0
	ld	[rIF],a
	ld	a,$FF
	ld	[rTIMA],a
	ld	[rDIV],a
	ld	a,TACF_STOP|TACF_65KHZ
	ld	[rTAC],a
	ld	a,TACF_START|TACF_65KHZ
	ld	[rTAC],a
	ei
	
	halt
	
	ld	a,[rIF]
	ld	[hl+],a
	
	
	ld	b,130
	call	wait_ly
	
	di
	ld	a,IEF_VBLANK|IEF_TIMER
	ld	[rIE],a
	ld	a,0
	ld	[rIF],a
	ld	a,$FF
	ld	[rTIMA],a
	ld	[rDIV],a
	ld	a,TACF_STOP|TACF_65KHZ
	ld	[rTAC],a
	ld	a,TACF_START|TACF_65KHZ
	ld	[rTAC],a
	
	halt
	
	ld	a,[rIF]
	ld	[hl+],a
	
	
	ld	b,130
	call	wait_ly
	
	di
	ld	a,IEF_VBLANK|IEF_TIMER
	ld	[rIE],a
	ld	a,0
	ld	[rIF],a
	ld	a,$FF
	ld	[rTIMA],a
	ld	[rDIV],a
	ld	a,TACF_STOP|TACF_65KHZ
	ld	[rTAC],a
	ld	a,TACF_START|TACF_65KHZ
	ld	[rTAC],a
	ei
	
	halt
	
	ld	a,[rIF]
	ld	[hl+],a
	
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

.endloop:
	halt
	jr	.endloop
