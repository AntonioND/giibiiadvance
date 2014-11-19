
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
	ld	[rDIV],a
	
	REPT 7
	push	de
	pop		de
	nop
	ENDR
	nop
	nop
	nop
	nop
	nop
	
	ld	[rDIV],a ; write just when internal counter goes from 0x00FF to 0x0100
	
	ld	a,[rDIV]
	ld	[hl+],a
	
	; ------------------------
	
	ld	a,0
	ld	[rDIV],a
	
	REPT 14
	push	de
	pop		de
	nop
	ENDR
	REPT 13
	nop
	ENDR
	
	ld	[rDIV],a ; write just when internal counter goes from 0x01FF to 0x0200
	
	ld	a,[rDIV]
	ld	[hl+],a
	
	; ------------------------

REPETITIONS SET 0
	REPT 8
	
	ld	a,0
	ld	[rDIV],a
	ld	[rDIV],a ; write when internal counter goes to 12
	
	REPT 7
	push	de
	pop		de
	nop
	ENDR
	
	REPT REPETITIONS
	nop
	ENDR

	ld	a,[rDIV]
	ld	[hl+],a

REPETITIONS SET REPETITIONS+1
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
