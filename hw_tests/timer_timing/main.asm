
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
	
	ld	de,rDIV
	ld	c,rTIMA&$FF
	
REPETITIONS_1 SET 0 ; 4 nops
	REPT 8
	
	ld	a,TACF_START|TACF_262KHZ
	ld	[rTAC],a
	xor	a,a
	ld	[de],a ; DIV
	ld	[$FF00+c],a ; TIMA
	REPT	REPETITIONS_1
	nop
	ENDR
	ld	a,[rTIMA]
	ld	[hl+],a

REPETITIONS_1 SET REPETITIONS_1+1
	ENDR
	
	; -----------------------
	
REPETITIONS_1 SET 0 ; 16 nops
	REPT 8

	ld	a,TACF_START|TACF_65KHZ
	ld	[rTAC],a
	xor	a,a
	ld	[de],a ; DIV
	ld	[$FF00+c],a ; TIMA
	push	de ; |
	pop		de ; | 8 nops
	nop        ; |
	REPT	REPETITIONS_1
	nop
	ENDR
	ld	a,[rTIMA]
	ld	[hl+],a

REPETITIONS_1 SET REPETITIONS_1+1
	ENDR
	
	; -----------------------

REPETITIONS_1 SET 0 ; 64 nops
	REPT 8

	ld	a,TACF_START|TACF_16KHZ
	ld	[rTAC],a
	xor	a,a
	ld	[de],a ; DIV
	ld	[$FF00+c],a ; TIMA
	push	de ; |
	pop		de ; | 7 nops
	push	de
	pop		de
	push	de
	pop		de
	push	de
	pop		de
	push	de
	pop		de
	push	de
	pop		de
	push	de
	pop		de
	push	de
	pop		de ; 56 nops
	REPT	REPETITIONS_1
	nop
	ENDR
	ld	a,[rTIMA]
	ld	[hl+],a

REPETITIONS_1 SET REPETITIONS_1+1
	ENDR
	
	; -----------------------

REPETITIONS_1 SET 0 ; 256 nops
	REPT 8

	ld	a,TACF_START|TACF_4KHZ
	ld	[rTAC],a
	xor	a,a
	ld	[de],a ; DIV
	ld	[$FF00+c],a ; TIMA
	push	de ; |
	pop		de ; | 7 nops
	push	de
	pop		de
	push	de
	pop		de
	push	de
	pop		de
	push	de
	pop		de
	push	de
	pop		de
	push	de
	pop		de
	push	de
	pop		de ; 56 nops
	
	push	de ; |
	pop		de ; | 7 nops
	push	de
	pop		de
	push	de
	pop		de
	push	de
	pop		de
	push	de
	pop		de
	push	de
	pop		de
	push	de
	pop		de
	push	de
	pop		de ; 56 nops
	
	push	de ; |
	pop		de ; | 7 nops
	push	de
	pop		de
	push	de
	pop		de
	push	de
	pop		de
	push	de
	pop		de
	push	de
	pop		de
	push	de
	pop		de
	push	de
	pop		de ; 112 nops
	
	push	de ; |
	pop		de ; | 7 nops
	push	de
	pop		de
	push	de
	pop		de
	push	de
	pop		de
	push	de
	pop		de
	push	de
	pop		de
	push	de
	pop		de
	push	de
	pop		de ; 168 nops
	
	push	de
	pop		de
	push	de
	pop		de ; 182 nops

	push	de
	pop		de
	nop ; 190
	
	REPT	REPETITIONS_1
	nop
	ENDR
	ld	a,[rTIMA]
	ld	[hl+],a

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
