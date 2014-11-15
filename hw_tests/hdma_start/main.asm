
	INCLUDE	"hardware.inc"
	INCLUDE "header.inc"

	
	SECTION	"var",BSS
	

ram_ptr:	DS 2
	

	SECTION	"Main",HOME

;--------------------------------------------------------------------------
;- Main()                                                                 -
;--------------------------------------------------------------------------

output_value: ; c = value
	
	ld	a,$0A
	ld	[$0000],a ; enable ram
	
	; c = data
	
	ld	a,[ram_ptr+0]
	ld	l,a
	ld	a,[ram_ptr+1]
	ld	h,a
	
	ld	[hl],c
	inc hl
	
	ld	a,h
	ld	[ram_ptr+1],a
	ld	a,l
	ld	[ram_ptr+0],a
	
	ld	[hl],$12
	inc hl
	ld	[hl],$34
	inc hl
	ld	[hl],$56
	inc hl
	ld	[hl],$78
	
	ld	a,$00
	ld	[$0000],a ; disable ram
	
	ret
	

Main:
	
	ld	a,0
	ld	[ram_ptr+0],a
	ld	a,$A0
	ld	[ram_ptr+1],a
	
	DMA_COPY	0,$8000,20*16,1
	
	ld	a,$80
	ld	[rLCDC],a
	
.loopme:
	ld	a,[rHDMA5]
	and	a,$80
	jr	z,.loopme
	
	ld	a,[rLY]
	ld	c,a
	call	output_value
	
	; -----------------
	
	ld	b,2
	call	wait_ly
	call	wait_screen_blank
	
	DMA_COPY	0,$8000,20*16,1
	
.loopme2:
	ld	a,[rHDMA5]
	and	a,$80
	jr	z,.loopme2
	
	ld	a,[rLY]
	ld	c,a
	call	output_value
	
	; -----------------
	
	ld	b,2
	call	wait_ly
	
	DMA_COPY	0,$8000,20*16,1
	
.loopme3:
	ld	a,[rHDMA5]
	and	a,$80
	jr	z,.loopme3
	
	ld	a,[rLY]
	ld	c,a
	call	output_value
	
	; -----------------
	
	ld	b,2
	call	wait_ly
	
	DMA_COPY	0,$8000,20*16,1
	
	ld	b,10
	call	wait_ly
	
	ld	a,$00 ; stop
	ld	[rHDMA5],a
	
	ld	a,[rHDMA5]
	ld	c,a
	call	output_value
	
	; -----------------
	
	ld	b,2
	call	wait_ly
	
	DMA_COPY	0,$8000,20*16,1
	
	ld	b,10
	call	wait_ly
	
	ld	a,$0F ; stop
	ld	[rHDMA5],a
	
	ld	a,[rHDMA5]
	ld	c,a
	call	output_value
	
	; -----------------
	
.end:
	halt
	jr	.end
