
	INCLUDE	"hardware.inc"
	INCLUDE "header.inc"

	
	SECTION	"var",BSS
	
repeat_loop:	DS 1

	SECTION	"Main",HOME


	GLOBAL lcd_func
lcd_func::
	push	af
	ld	a,[rLY]
	cp	a,11
	jr	z,.line_11
	pop	af
	ld	[hl+],a
	ret
	
.line_11:
	ld	a,13
	ld	[rLYC],a
	pop	af
	xor	a,a
	reti
	
	
;--------------------------------------------------------------------------
;- Main()                                                                 -
;--------------------------------------------------------------------------

Main:
	
	ld	a,IEF_LCDC
	ld	[rIE],a
	
	ld	a,LCDCF_ON
	ld	[rLCDC],a
	
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
	

REPETITIONS	SET 0
	REPT	16
	
	di
	
	ld	a,0
	ld	[rTIMA],a
	ld	[rTMA],a
	ld	[rDIV],a
	ld	a,TACF_STOP|TACF_262KHZ
	ld	[rTAC],a
	ld	a,TACF_START|TACF_262KHZ
	ld	[rTAC],a
	
	ld	a,11
	ld	[rLYC],a
	ld	a,STATF_LYC
	ld	[rSTAT],a
	
	ld	a,0
	ld	[rHDMA1],a
	ld	a,0
	ld	[rHDMA2],a
	ld	a, $80 & $1F ; Upper 3 bits ignored
	ld	[rHDMA3],a
	ld	a, $00
	ld	[rHDMA4],a ; setup source and dest
	
	ld	b,11
	call	wait_ly
	ld	b,10
	call	wait_ly
	
	xor	a,a
	ld	[rIF],a
	
	ei
	
	ld	[rDIV],a
	ld	[rTIMA],a
	ld	[rDIV],a
	
	ld	a,$81
	ld	[rHDMA5],a

	REPT REPETITIONS
	nop
	ENDR
	
	ld	a,[rTIMA]
	ld	[hl+],a
	
	REPT 15-REPETITIONS
	nop
	ENDR
	
	xor	a,a
	
	REPT	800
	inc	a
	ENDR

REPETITIONS	SET	REPETITIONS+1
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
