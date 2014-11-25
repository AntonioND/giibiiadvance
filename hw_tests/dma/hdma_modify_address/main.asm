
	INCLUDE	"hardware.inc"
	INCLUDE "header.inc"

	SECTION	"Main",HOME

;--------------------------------------------------------------------------
;- Main()                                                                 -
;--------------------------------------------------------------------------

Main:

	; -------------------------------------------------------
	
	ld	a,$0A
	ld	[$0000],a ; enable ram
	
	ld	a,LCDCF_ON
	ld	[rLCDC],a
	
	ld	hl,$A000

	; -------------------------------------------------------
	
	ld	b,10
	call	wait_ly
	
	DMA_COPY	$0100,$8000,32,1
	
	ld	b,11
	call	wait_ly
	
	
	ld	a, ( $0140 >> 8 )& $FF
	ld	[rHDMA1],a
	ld	a, $0140 & $F0 ; Lower 4 bits ignored
	ld	[rHDMA2],a
	
	ld	b,10
	call	wait_ly
	
	DMA_COPY	$0100,$8020,32,1
	
	ld	b,11
	call	wait_ly

	ld	a, ( $8040 >> 8 )& $1F ; Upper 3 bits ignored
	ld	[rHDMA3],a
	ld	a, $8040 & $F0 ; Lower 4 bits ignored
	ld	[rHDMA4],a

	ld	b,144
	call	wait_ly
	
	ld	bc,$50
	ld	hl,$8000
	ld	de,$A000
	
	call	memcopy
	
	ld	h,d ; get ending ptr
	ld	l,e
	
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
	
	; -------------------------------------------------------
	
	ld	a,$00
	ld	[$0000],a ; disable ram
	
.endloop:
	halt
	jr	.endloop
