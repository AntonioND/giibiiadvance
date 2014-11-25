
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
	
	; -----------------
	
	ld	b,4
	call	wait_ly
	
	DMA_COPY	0,$8000,20*16,1
	
	ld	a,0
	ld	[rIF],a
	ld	a,IEF_VBLANK
	ld	[rIE],a
	
	ld	a,[rHDMA5]
	ld	[hl+],a
	
	halt
	
	ld	a,[rHDMA5]
	ld	[hl+],a
	
	ld	a,0
	ld	[rHDMA5],a ; stop now, who cares about the actual copy?
	
	push	hl
	ld	[hl],$12
	inc hl
	ld	[hl],$34
	inc hl
	ld	[hl],$56
	inc hl
	ld	[hl],$78
	pop	hl
	
	; -----------------
	
	ld	b,4
	call	wait_ly
	
	DMA_COPY	0,$8000,20*16,1
	
	ld	a,0
	ld	[rIF],a
	ld	a,IEF_VBLANK
	ld	[rIE],a
	
	ld	a,[rHDMA5]
	ld	[hl+],a
	
	ld	a,0
	ld	[rP1],a
	
	stop
	
	ld	a,[rHDMA5]
	ld	[hl+],a
	
	ld	a,[rLY]
	ld	[hl+],a
	add	a,5
	ld	b,a
	call	wait_ly
	
	ld	a,[rHDMA5]
	ld	[hl+],a
	
	ld	a,0
	ld	[rHDMA5],a ; stop now, who cares about the actual copy?
	
	push	hl
	ld	[hl],$12
	inc hl
	ld	[hl],$34
	inc hl
	ld	[hl],$56
	inc hl
	ld	[hl],$78
	pop	hl
	
	; -----------------
	
	ld	b,4
	call	wait_ly
	
	DMA_COPY	0,$8000,20*16,1
	
	ld	a,0
	ld	[rIF],a
	ld	a,IEF_VBLANK
	ld	[rIE],a
	
	ld	a,[rHDMA5]
	ld	[hl+],a
	
	ld	a,0
	ld	[rP1],a
	
	call	CPU_fast
	
	ld	a,[rHDMA5]
	ld	[hl+],a
	
	ld	a,[rLY]
	ld	[hl+],a
	add	a,5
	ld	b,a
	call	wait_ly
	
	ld	a,[rHDMA5]
	ld	[hl+],a
	
	ld	a,0
	ld	[rHDMA5],a ; stop now, who cares about the actual copy?
	
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
