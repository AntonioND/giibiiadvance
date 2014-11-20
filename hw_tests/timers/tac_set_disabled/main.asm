
	INCLUDE	"hardware.inc"
	INCLUDE "header.inc"

	
	SECTION	"var",BSS
	
repeat_loop:	DS 1
current_test:	DS 1

	SECTION	"Main",HOME

;--------------------------------------------------------------------------
;- Main()                                                                 -
;--------------------------------------------------------------------------

Main:
	
	ld	a,-1
	ld	[current_test],a
	
	; -------------------------------------------------------
	
	ld	a,$0A
	ld	[$0000],a ; enable ram
	
	ld	a,$00
	ld	[$4000],a ; ram bank change
	
	ld	d,0
	ld	hl,$A000
	ld	bc,$2000
	call	memset
	
	ld	a,$01
	ld	[$4000],a ; ram bank change
	
	ld	d,0
	ld	hl,$A000
	ld	bc,$2000
	call	memset
	
	ld	a,$02
	ld	[$4000],a ; ram bank change
	
	ld	d,0
	ld	hl,$A000
	ld	bc,$2000
	call	memset
	
	ld	a,$03
	ld	[$4000],a ; ram bank change
	
	ld	d,0
	ld	hl,$A000
	ld	bc,$2000
	call	memset
	
	; -------------------------------------------------------

	ld	a,[Init_Reg_A]
	cp	a,$11
	jr	nz,.skipchange1
	ld	a,0
	ld	[repeat_loop],a
	call	CPU_slow
.skipchange1:

.repeat_all:
	
	; -------------------------------------------------------
	
	ld	d,0
.loopram:

	ld	a,[current_test]
	inc	a
	ld	[current_test],a
	srl	a
	ld	[$4000],a
	
	ld	a,[current_test]
	and	a,1
	jr	nz,.skip
	ld	hl,$A000
.skip:
	
	ld	c,251
.loopout:

	ld	b,0
.loop:

REPETITIONS SET 0
	REPT	16
	
	ld	a,d
	ld	[rTAC],a
	ld	a,0
	ld	[rTMA],a
	ld	[rDIV],a
	ld	[rIF],a
	ld	a,c
	ld	[rTIMA],a
	
	ld	a,b
	ld	[rDIV],a ; sync
	
	REPT REPETITIONS
	nop
	ENDR
	
	ld	[rTAC],a
	
	ld	a,[rTIMA]
	ld	[hl+],a
	ld	a,[rIF]
	and	a,4
	ld	[hl+],a

REPETITIONS SET REPETITIONS+1
	ENDR

	inc	b
	ld	a,16
	cp	a,b
	jp	nz,.loop
	
	inc	c
	ld	a,2
	cp	a,c
	jp	nz,.loopout
	
	
	
	push	hl ; magic number
	ld	[hl],$12
	inc hl
	ld	[hl],$34
	inc hl
	ld	[hl],$56
	inc hl
	ld	[hl],$78
	pop	hl
	
	
	
	inc d
	ld	a,4
	cp	a,d
	jp	nz,.loopram
	
	
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
	
	ld	a,$00
	ld	[$0000],a ; disable ram
	
.endloop:
	halt
	jr	.endloop
