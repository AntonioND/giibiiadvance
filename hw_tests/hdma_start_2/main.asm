
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

	ld	c,$FF
.loopwaitnolcd:
	nop
	nop
	nop
	nop
	dec	c
	jr	nz,.loopwaitnolcd
	
	ld	a,[rHDMA5]
	ld	c,a
	call	output_value
	
	ld	a,[rLY]
	ld	c,a
	call	output_value
	
	ld	a,[rSTAT]
	ld	c,a
	call	output_value
	
	ld	a,$80
	ld	[rLCDC],a
	
	ld	a,[rLY]
	ld	c,a
	call	output_value
	
	ld	a,[rHDMA5]
	ld	c,a
	call	output_value
	
	ld	b,2
	call	wait_ly
	call	wait_screen_blank
	
	ld	a,[rHDMA5]
	ld	c,a
	call	output_value
	
.loopme:
	ld	a,[rHDMA5]
	and	a,$80
	jr	z,.loopme
	
	ld	a,[rLY]
	ld	c,a
	call	output_value
	
	
	ld	a,[rHDMA1]
	ld	c,a
	call	output_value
	ld	a,[rHDMA2]
	ld	c,a
	call	output_value
	ld	a,[rHDMA3]
	ld	c,a
	call	output_value
	ld	a,[rHDMA4]
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
	
	ld	a,$80 ; reduce to 16 bytes
	ld	[rHDMA5],a

.loopme4:
	ld	a,[rHDMA5]
	and	a,$80
	jr	z,.loopme4

	ld	a,[rLY]
	ld	c,a
	call	output_value
	
	; -----------------
	
	ld	b,2
	call	wait_ly
	
	DMA_COPY	0,$8000,20*16,1
	
	ld	b,10
	call	wait_ly
	
	ld	a,$8F ; set to 16*15 bytes
	ld	[rHDMA5],a

.loopme5:
	ld	a,[rHDMA5]
	and	a,$80
	jr	z,.loopme5
	
	ld	a,[rLY]
	ld	c,a
	call	output_value
	
	; -----------------
	
.end:
	halt
	jr	.end
