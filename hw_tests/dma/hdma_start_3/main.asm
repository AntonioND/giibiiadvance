
	INCLUDE	"hardware.inc"
	INCLUDE "header.inc"

	
	SECTION	"var",BSS
	

ram_ptr:	DS 2
	

	SECTION	"Main",HOME

;--------------------------------------------------------------------------
;- Main()                                                                 -
;--------------------------------------------------------------------------

Main:
	
	; -----------------
	
	ld	a,$0A
	ld	[$0000],a ; enable ram
	
	ld	hl,$A000
	
	; -----------------
	
	ld	a,LCDCF_ON
	ld	[rLCDC],a
	
	ld	b,142
	call	wait_ly
	
	DMA_COPY	0,$8000,20*16,1
	
	ld	b,150
	call	wait_ly
	
	ld	a,[rHDMA5]
	ld	[hl+],a
	
	ld	a,LCDCF_OFF
	ld	[rLCDC],a
	
	ld	a,[rHDMA5]
	ld	[hl+],a
	
	ld	a,LCDCF_ON
	ld	[rLCDC],a
	
	ld	a,[rHDMA5]
	ld	[hl+],a
	
.loop1:
	ld	a,[rHDMA5]
	and	a,$80
	jr	z,.loop1
	
	ld	a,[rLY]
	ld	[hl+],a
	
	; -----------------
	
	ld	b,150
	call	wait_ly

	ld	a,LCDCF_OFF
	ld	[rLCDC],a
	
	
REPETITIONS	SET	0
	REPT	8
	
	
	ld	b,4
.testagain\@:

	ld	a,0
	ld	[rHDMA5],a
	ld	[rTIMA],a
	ld	[rTMA],a
	ld	[rDIV],a
	ld	a,TACF_STOP|TACF_262KHZ
	ld	[rTAC],a
	ld	a,TACF_START|TACF_262KHZ
	ld	[rTAC],a
	xor	a,a
	ld	[rDIV],a
	ld	[rTIMA],a
	ld	[rDIV],a
	
	DMA_COPY	0,$8000,1*16,1
	
	REPT	REPETITIONS
	nop
	ENDR
	
	ld	a,[rTIMA]
	ld	[hl+],a
	
	dec	b
	jr	nz,.testagain\@
	

REPETITIONS	SET	REPETITIONS+1
	ENDR
	
	; -----------------
	
	; continue ptrs GDMA
	DMA_COPY	0,$8000,1*16,0
	ld	a,0
	ld	[rHDMA5],a ; continue copy
	
	push	hl
	ld	hl,$0000
	ld	de,$8000
	ld	b,32
	ld	c,0
.memcmp:
	ld	a,[de]
	inc	de
	sub	a,[hl]
	inc	hl
	or	a,c
	ld	c,a
	dec	b
	jr	nz,.memcmp
	ld	a,c
	pop	hl
	
	ld	[hl+],a
	
	; -----------------
	; continue ptrs HDMA
	
	ld	a,LCDCF_ON
	ld	[rLCDC],a
	
	ld	b,2
	call	wait_ly
	
	DMA_COPY	0,$8000,1*16,1
	
	ld	b,5
	call	wait_ly
	
	ld	a,$80
	ld	[rHDMA5],a ; continue copy
	
.loop2:
	ld	a,[rHDMA5]
	and	a,$80
	jr	z,.loop2
	
	ld	a,[rLY]
	ld	[hl+],a
	
	ld	b,150
	call	wait_ly
	
	ld	a,LCDCF_OFF
	ld	[rLCDC],a
	
	push	hl
	ld	hl,$0000
	ld	de,$8000
	ld	b,32
	ld	c,0
.memcmp1:
	ld	a,[de]
	inc	de
	sub	a,[hl]
	inc	hl
	or	a,c
	ld	c,a
	dec	b
	jr	nz,.memcmp1
	ld	a,c
	pop	hl
	
	ld	[hl+],a
	
	; -----------------
	; continue ptrs HDMA after stop (no wait HBL)
	
	ld	a,LCDCF_ON
	ld	[rLCDC],a
	
	ld	b,2
	call	wait_ly
	
	DMA_COPY	0,$8000,3*16,1
	
	ld	b,3
	call	wait_ly
	
	ld	a,0
	ld	[rHDMA5],a ; stop copy

	ld	a,[rHDMA5]
	ld	[hl+],a
	
	ld	a,$80
	ld	[rHDMA5],a ; continue copy

.loop4:
	ld	a,[rHDMA5]
	and	a,$80
	jr	z,.loop4
	
	ld	a,[rLY]
	ld	[hl+],a
	
	ld	b,150
	call	wait_ly
	
	ld	a,LCDCF_OFF
	ld	[rLCDC],a
	
	push	hl
	ld	hl,$0000
	ld	de,$8000
	ld	b,32
	ld	c,0
.memcmp2:
	ld	a,[de]
	inc	de
	sub	a,[hl]
	inc	hl
	or	a,c
	ld	c,a
	dec	b
	jr	nz,.memcmp2
	ld	a,c
	pop	hl
	
	ld	[hl+],a
	
	; -----------------
	; continue ptrs HDMA after stop (wait HBL)
	
	ld	a,LCDCF_ON
	ld	[rLCDC],a
	
	ld	b,2
	call	wait_ly
	
	DMA_COPY	0,$8000,3*16,1
	
	ld	b,3
	call	wait_ly
	
	ld	a,0
	ld	[rHDMA5],a ; stop copy
	
	ld	b,4
	call	wait_ly
	
	ld	a,$80
	ld	[rHDMA5],a ; continue copy
	
.loop5:
	ld	a,[rHDMA5]
	and	a,$80
	jr	z,.loop5
	
	ld	a,[rLY]
	ld	[hl+],a
	
	ld	b,150
	call	wait_ly
	
	ld	a,LCDCF_OFF
	ld	[rLCDC],a
	
	push	hl
	ld	hl,$0000
	ld	de,$8000
	ld	b,32
	ld	c,0
.memcmp3:
	ld	a,[de]
	inc	de
	sub	a,[hl]
	inc	hl
	or	a,c
	ld	c,a
	dec	b
	jr	nz,.memcmp3
	ld	a,c
	pop	hl
	
	ld	[hl+],a
	
	; -----------------
	
	push	hl
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
	
	; -----------------
	
.end:
	halt
	jr	.end
