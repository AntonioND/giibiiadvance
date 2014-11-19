
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
	
REPETITIONS_1 SET 0
	REPT 20

REPETITIONS_2 SET 0
	REPT 20
	
	ld	a,0
	ld	[rTIMA],a
	ld	a,TACF_START|TACF_65KHZ
	ld	[rTAC],a
	REPT	REPETITIONS_1
	nop
	ENDR
	ld	a,0
	ld	[rDIV],a
	REPT	REPETITIONS_2
	nop
	ENDR
	ld	a,[rTIMA]
	ld	[hl+],a

REPETITIONS_2 SET REPETITIONS_2+1
	ENDR
	
REPETITIONS_1 SET REPETITIONS_1+1
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
