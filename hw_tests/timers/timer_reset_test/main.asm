
	INCLUDE	"hardware.inc"
	INCLUDE "header.inc"

	
	SECTION	"var",BSS
	
ram_ptr:	DS 2
repeat_loop:	DS 1

	SECTION	"Main",HOME

;--------------------------------------------------------------------------
;- Main()                                                                 -
;--------------------------------------------------------------------------

Main:
	
	ld	hl,$A000
	
	ld	a,[Init_Reg_A]
	cp	a,$11
	jr	nz,.skipchange1
	ld	a,0
	ld	[repeat_loop],a
	call	CPU_slow
.skipchange1:

.repeat_all:
	
	; -------------------------------------------------------
	
	ld	a,$0A
	ld	[$0000],a ; enable ram
	
	; -------------------------------------------------------
	
	ld	a,0
	ld	[rTMA],a
	ld	[rDIV],a
	
	; -----------------------
	
	ld	de,rTAC
	ld	c,rTIMA&$FF
	
	; -----------------------

	ld	a,TACF_START|TACF_65KHZ
	ld	[de],a
	xor	a,a
	ld	[$FF00+c],a ; TIMA
	REPT 32
	ld	a,TACF_START|TACF_262KHZ
	ld	[de],a
	ENDR
	ld	a,[rTIMA]
	ld	[hl+],a
	
	; -----------------------

	ld	a,TACF_START|TACF_65KHZ
	ld	[de],a
	xor	a,a
	ld	[$FF00+c],a ; TIMA
	REPT 32
	ld	[rDIV],a
	ENDR
	ld	a,[rTIMA]
	ld	[hl+],a
	
	; -----------------------
	
	xor	a,a
	ld	[rDIV],a
	ld	[rTIMA],a
	ld	a,TACF_START|TACF_65KHZ
	ld	[de],a
	
	REPT 16
	ld	[de],a ; TAC
	ENDR
	
	ld	a,[rDIV]
	ld	[hl+],a
	ld	a,[rTIMA]
	ld	[hl+],a
	
	REPT 16
	ld	[de],a ; TAC
	ENDR
	
	ld	a,[rDIV]
	ld	[hl+],a
	ld	a,[rTIMA]
	ld	[hl+],a
	
	REPT 16
	ld	[de],a ; TAC
	ENDR
	
	ld	a,[rDIV]
	ld	[hl+],a
	ld	a,[rTIMA]
	ld	[hl+],a
	
	; -----------------------
	
REPEATS	SET	0
	REPT 20
	
	xor	a,a
	ld	[rDIV],a
	ld	[rTIMA],a
	ld	a,TACF_START|TACF_65KHZ
	ld	[de],a
	
	nop
	
	ld	a,TACF_START|TACF_16KHZ
	ld	[de],a
	
	REPT REPEATS
	nop
	ENDR

	ld	a,[rTIMA]
	ld	[hl+],a
	
REPEATS SET REPEATS+1
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
	
	ld	a,[Init_Reg_A]
	cp	a,$11
	jr	nz,.skipchange2
	ld	a,[repeat_loop]
	and	a,a
	jr	nz,.endloop
	; -------------------------------------------------------

	call	CPU_fast
	ld	a,1
	ld	[repeat_loop],a
	jp	.repeat_all
.skipchange2:
	
.endloop:
	halt
	jr	.endloop
