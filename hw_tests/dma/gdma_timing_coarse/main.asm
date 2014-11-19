
	INCLUDE	"hardware.inc"
	INCLUDE "header.inc"

	
	SECTION	"var",BSS
	
repeat_loop:	DS 1

	SECTION	"Main",HOME

;--------------------------------------------------------------------------
;- Main()                                                                 -
;--------------------------------------------------------------------------

Main:
	
	ld	a,0
	ld	[rIE],a

	ld	a,0
	ld	[repeat_loop],a
	call	CPU_slow


	ld	hl,$A000
	
.repeat_all:
	
	ld	a,$0A
	ld	[$0000],a ; enable ram
	
	; -------------------------------------------------------
	; 24 + 1 * 64 = 88 clocks ?
    ; 24 + 128 * 64 = 8216 clocks ?
	; const u32 gb_timer_clocks[4] = {1024,16,64,256} clocks per tick


SIZE_COPY	SET	1
	REPT	128
	
	
	ld	b,4
.testagain\@:

	ld	a,0
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
	
	DMA_COPY	0,$8000,SIZE_COPY*16,0
	
	ld	a,[rTIMA]
	ld	[hl+],a
	
	dec	b
	jr	nz,.testagain\@
	

SIZE_COPY	SET	SIZE_COPY+1
	ENDR
	
	
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
	
	
	ld	a,[repeat_loop]
	and	a,a
	jr	nz,.endloop
	; -------------------------------------------------------

	call	CPU_fast
	ld	a,1
	ld	[repeat_loop],a
	jp	.repeat_all
	
	; -------------------------------------------------------
	
.endloop:
	halt
	jr	.endloop
