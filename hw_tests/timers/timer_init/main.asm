
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

REPETITIONS SET 0
	REPT	16
	
	ld	a,TACF_STOP|TACF_262KHZ
	ld	[rTAC],a
	
	ld	a,0
	ld	[rTMA],a
	ld	[rTIMA],a
	ld	[rIF],a
	ld	[rDIV],a
	
	ld	a,TACF_START|TACF_262KHZ
	ld	[rTAC],a
	
	REPT REPETITIONS
	nop
	ENDR
	
	ld	a,[rTIMA]
	ld	[hl+],a

REPETITIONS SET REPETITIONS+1
	ENDR
	
	; -------------------------------------------------------

REPETITIONS SET 0
	REPT	16
	
	ld	a,TACF_STOP|TACF_65KHZ
	ld	[rTAC],a
	
	ld	a,0
	ld	[rTMA],a
	ld	[rTIMA],a
	ld	[rIF],a
	ld	[rDIV],a
	
	ld	a,TACF_START|TACF_65KHZ
	ld	[rTAC],a
	
	inc	de
	dec	de
	REPT REPETITIONS
	nop
	ENDR
	
	ld	a,[rTIMA]
	ld	[hl+],a

REPETITIONS SET REPETITIONS+1
	ENDR
	
	; -------------------------------------------------------

REPETITIONS SET 0
	REPT	16
	
	ld	a,TACF_STOP|TACF_16KHZ
	ld	[rTAC],a
	
	ld	a,0
	ld	[rTMA],a
	ld	[rTIMA],a
	ld	[rIF],a
	ld	[rDIV],a
	
	ld	a,TACF_START|TACF_16KHZ
	ld	[rTAC],a
	
	ld	b,13
.loop\@:
	dec	b
	jr	nz,.loop\@
	REPT REPETITIONS
	nop
	ENDR
	
	ld	a,[rTIMA]
	ld	[hl+],a

REPETITIONS SET REPETITIONS+1
	ENDR
	
	; -------------------------------------------------------

REPETITIONS SET 0
	REPT	16
	
	ld	a,TACF_STOP|TACF_4KHZ
	ld	[rTAC],a
	
	ld	a,0
	ld	[rTMA],a
	ld	[rTIMA],a
	ld	[rIF],a
	ld	[rDIV],a
	
	ld	a,TACF_START|TACF_4KHZ
	ld	[rTAC],a
	
	ld	b,61
.loop\@:
	dec	b
	jr	nz,.loop\@
	REPT REPETITIONS
	nop
	ENDR
	
	ld	a,[rTIMA]
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
