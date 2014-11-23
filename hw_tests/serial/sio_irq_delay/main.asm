
	INCLUDE	"hardware.inc"
	INCLUDE "header.inc"

	
	SECTION	"var",BSS
	
repeat_loop:	DS 1

	SECTION	"Main",HOME

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

.repeat_all:
	
	; -------------------------------------------------------
	
	ld	a,$0A
	ld	[$0000],a ; enable ram
	
	; -------------------------------------------------------

	ld	d,128
.loop:
	
	
	di
	xor	a,a
	ld	[rIF],a
	ld	[rSC],a

	ld	a,IEF_SERIAL
	ld	[rIE],a
	
	ld	a,$81
	ld	[rSC],a
	ei
	xor	a,a
	
	REPT	110
	push	de
	pop	de
	nop
	ENDR
	REPT	200
	inc	a
	ENDR
	
	di
	xor	a,a
	ld	[rIF],a
	ld	[rSC],a

	ld	a,IEF_SERIAL
	ld	[rIE],a
	
	ld	a,$83
	ld	[rSC],a
	ei
	xor	a,a
	
	REPT	300
	inc	a
	ENDR
	
	di
	
	
	dec	d
	jp	nz,.loop
	
	
	
	
	
	ld	a,$81
	ld	[rSC],a
	ld	a,$00
	ld	[rSC],a
	ld	a,[rSC]
	ld	[hl+],a
	
	REPT	600
	nop
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
	jr	nz,.dontchange
	ld	a,[repeat_loop]
	and	a,a
	jr	nz,.endloop
	; -------------------------------------------------------

	call	CPU_fast
	ld	a,1
	ld	[repeat_loop],a
	jp	.repeat_all
.dontchange:
	
.endloop:
	halt
	jr	.endloop
