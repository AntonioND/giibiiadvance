
	INCLUDE	"hardware.inc"
	INCLUDE "header.inc"

	
	SECTION	"var",BSS
	
repeat_loop:	DS 1

	SECTION	"Main",HOME

;--------------------------------------------------------------------------
;- Main()                                                                 -
;--------------------------------------------------------------------------

Main:
	
	xor	a,a
	ld	[rIF],a

	ld	a,IEF_TIMER
	ld	[rIE],a
	
	ei
	
	ld	hl,$A000

	ld	a,$0A
	ld	[$0000],a ; enable ram
	
	ld	a,$FD
	ld	[rDIV],a
	ld	[rTIMA],a
	xor	a,a
	ld	[rTMA],a
	ld	a,TACF_STOP|TACF_65KHZ
	ld	[rTAC],a
	ld	a,TACF_START|TACF_65KHZ
	ld	[rTAC],a
	
	DMA_COPY	0,$8000,64*16,0	

	ld	a,[rTIMA]
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
	
.endloop:
	halt
	jr	.endloop
