
	INCLUDE	"hardware.inc"
	INCLUDE "header.inc"

	
	SECTION	"var",BSS
	
repeat_loop:	DS 1

	SECTION	"Main",ROM0

;--------------------------------------------------------------------------
;- Main()                                                                 -
;--------------------------------------------------------------------------

Main:
	
	ld	hl,$A000
	
	ld	a,[Init_Reg_A]
	cp	a,$11
	jr	nz,.skip1
	ld	a,0
	ld	[repeat_loop],a
	call	CPU_slow
.skip1:

repeat_all:
	
	; -------------------------------------------------------
	
	ld	a,$0A
	ld	[$0000],a ; enable ram
	
	; -------------------------------------------------------

	di
	ld	a,0
	ld	[hl+],a
	
	ld	a,IEF_SERIAL
	ld	[rIE],a
	ld	[rIF],a
	
	ld	c,rDIV & $FF
	
	ld	a,$81
	ld	[rSC],a
	
	xor	a,a
	ei
	
	REPT	$800
	ld	[$FF00+c],a
	ENDR
	di
	
	; --------------------
	
	di
	ld	a,1
	ld	[hl+],a
	ld	a,IEF_SERIAL
	ld	[rIE],a
	ld	[rIF],a
	
	ld	c,rDIV & $FF
	
	ld	a,$81|2
	ld	[rSC],a
	
	xor	a,a
	ei
	
	REPT	$800
	ld	[$FF00+c],a
	ENDR
	di
	
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
	jr	nz,.dontchange
	ld	a,[repeat_loop]
	and	a,a
	jr	nz,.endloop
	; -------------------------------------------------------

	call	CPU_fast
	ld	a,1
	ld	[repeat_loop],a
	jp	repeat_all
.dontchange:
	
.endloop:
	halt
	jr	.endloop
