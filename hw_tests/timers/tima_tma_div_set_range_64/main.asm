
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

	ld	b,250
.repeat1:

DELAY_CLOCKS SET 0
	REPT	16
	
	ld	a,TACF_START|TACF_65KHZ
	ld	[rTAC],a
	ld	a,0
	ld	[rTMA],a
	ld	[rDIV],a
	ld	[rTIMA],a
	ld	[rIF],a
	
	ld	a,b
	ld	[rDIV],a ; sync
	
	REPT DELAY_CLOCKS
	nop
	ENDR
	
	ld	[rTIMA],a
	
	ld	a,[rTIMA]
	ld	[hl+],a
	ld	a,[rTMA]
	ld	[hl+],a
	ld	a,[rTAC]
	ld	[hl+],a
	ld	a,[rIF]
	and	a,4
	ld	[hl+],a

DELAY_CLOCKS SET DELAY_CLOCKS+1
	ENDR
	
	inc	b
	ld	a,5
	cp	a,b
	jp	nz,.repeat1
	
	; -------------------------------------------------------

	ld	b,250
.repeat2:

DELAY_CLOCKS SET 0
	REPT	16
	
	ld	a,TACF_START|TACF_65KHZ
	ld	[rTAC],a
	ld	a,0
	ld	[rTMA],a
	ld	[rDIV],a
	ld	[rTIMA],a
	ld	[rIF],a
	
	ld	a,b
	ld	[rDIV],a ; sync
	
	REPT DELAY_CLOCKS
	nop
	ENDR
	
	ld	[rTMA],a
	
	ld	a,[rTIMA]
	ld	[hl+],a
	ld	a,[rTMA]
	ld	[hl+],a
	ld	a,[rTAC]
	ld	[hl+],a
	ld	a,[rIF]
	and	a,4
	ld	[hl+],a

DELAY_CLOCKS SET DELAY_CLOCKS+1
	ENDR
	
	inc	b
	ld	a,5
	cp	a,b
	jp	nz,.repeat2
	
	; -------------------------------------------------------

	ld	b,250
.repeat3:

DELAY_CLOCKS SET 0
	REPT	16
	
	ld	a,TACF_START|TACF_65KHZ
	ld	[rTAC],a
	ld	a,0
	ld	[rTMA],a
	ld	[rDIV],a
	ld	[rTIMA],a
	ld	[rIF],a
	
	ld	a,b
	ld	[rDIV],a ; sync
	
	REPT DELAY_CLOCKS
	nop
	ENDR
	
	ld	[rDIV],a
	
	ld	a,[rTIMA]
	ld	[hl+],a
	ld	a,[rTMA]
	ld	[hl+],a
	ld	a,[rTAC]
	ld	[hl+],a
	ld	a,[rIF]
	and	a,4
	ld	[hl+],a

DELAY_CLOCKS SET DELAY_CLOCKS+1
	ENDR
	
	inc	b
	ld	a,5
	cp	a,b
	jp	nz,.repeat3
	
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
