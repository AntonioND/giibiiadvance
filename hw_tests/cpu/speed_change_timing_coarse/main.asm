
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
	
	ld	a,0
	ld	[rIF],a
	ld	a,IEF_TIMER
	ld	[rIE],a
	
	ld	a,0
	ld	[rTMA],a
	ld	[rTIMA],a
	ld	[rDIV],a
	
	ld	a,TACF_STOP|TACF_4KHZ
	ld	[rTAC],a
	ld	a,TACF_START|TACF_4KHZ
	ld	[rTAC],a
	
	ld	a,0
	ld	[rTIMA],a
	ld	[rDIV],a
	
	; -----------------------

	ld	a,[rIE]
	ld	b,a ; save IE
	xor	a,a
	ld	[rIE],a
	ld	a,$30
	ld	[rP1],a
	ld	a,$01
	ld	[rKEY1],a
	
	stop ; switch
	
	ld	a,b
	ld	[rIE],a ; restore IE
	
	ld	a,[rTIMA]
	ld	[hl+],a
	
	xor	a,a
	REPT 256
	inc	a
	ENDR
	
	REPT 1000
	nop
	ENDR

	; -----------------------
	
	ld	a,0
	ld	[rIF],a
	ld	a,IEF_TIMER
	ld	[rIE],a
	
	ld	a,0
	ld	[rTMA],a
	ld	[rTIMA],a
	ld	[rDIV],a
	
	ld	a,TACF_STOP|TACF_4KHZ
	ld	[rTAC],a
	ld	a,TACF_START|TACF_4KHZ
	ld	[rTAC],a
	
	ld	a,0
	ld	[rTIMA],a
	ld	[rDIV],a
	
	; -----------------------

	ld	a,[rIE]
	ld	b,a ; save IE
	xor	a,a
	ld	[rIE],a
	ld	a,$30
	ld	[rP1],a
	ld	a,$01
	ld	[rKEY1],a
	
	stop ; switch
	
	ld	a,b
	ld	[rIE],a ; restore IE
	
	ld	a,[rTIMA]
	ld	[hl+],a
	
	xor	a,a
	REPT 256
	inc	a
	ENDR
	
	REPT 1000
	nop
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
