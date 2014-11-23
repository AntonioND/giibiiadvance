
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
	
	ld	a,$0A
	ld	[$0000],a ; enable ram
	
.repeat_all:
	
	; -------------------------------------------------------

	ld	de,$800
.loop:
	
	
	di
	xor	a,a
	ld	[rIF],a
	ld	[rSC],a

	ld	a,IEF_SERIAL
	ld	[rIE],a
	
	ld	a,$81
	ld	[rSC],a
	ld	a,$83
	ei
	
	ld	[rSC],a
	xor	a,a
	REPT	$500
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
	ld	a,$81
	ei
	
	ld	[rSC],a
	xor	a,a
	REPT	$500
	inc	a
	ENDR
	
	di
	
	
	dec	de
	ld	a,d
	or	e
	jp	nz,.loop

	; -------------------------------------------------------
	
	ld	a,[repeat_loop]
	and	a,a
	jr	nz,.dontchange
	; -------------------------------------------------------

	call	CPU_fast
	ld	a,1
	ld	[repeat_loop],a
	jp	.repeat_all
.dontchange:
	
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
	
.endloop:
	halt
	jr	.endloop
