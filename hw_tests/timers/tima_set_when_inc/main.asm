
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

VALUE_WRITEN SET 253
	REPT	16
	
	ld	a,TACF_START|TACF_16KHZ
	ld	[rTAC],a
	ld	a,0
	ld	[rTMA],a
	ld	[rDIV],a
	ld	[rTIMA],a
	ld	[rIF],a
	
	ld	a,VALUE_WRITEN & $FF
	ld	[rDIV],a ; sync
	
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
	
	ld	[rTIMA],a ; write just when internal counter goes from 0x00FF to 0x0100
	
	ld	a,[rTIMA]
	ld	[hl+],a
	ld	a,[rIF]
	and	a,4
	ld	[hl+],a

VALUE_WRITEN SET VALUE_WRITEN+1
	ENDR	

	; -----------------------

VALUE_WRITEN SET 253
	REPT	16
	
	ld	a,TACF_START|TACF_16KHZ
	ld	[rTAC],a
	ld	a,0
	ld	[rTMA],a
	ld	[rDIV],a
	ld	[rIF],a
	ld	a,$FF
	ld	[rTIMA],a
	
	ld	a,VALUE_WRITEN & $FF
	ld	[rDIV],a ; sync
	
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
	
	ld	[rTIMA],a ; write just when internal counter goes from 0x00FF to 0x0100
	
	ld	a,[rTIMA]
	ld	[hl+],a
	ld	a,[rIF]
	and	a,4
	ld	[hl+],a

VALUE_WRITEN SET VALUE_WRITEN+1
	ENDR	

	; -----------------------

VALUE_WRITEN SET 253
	REPT	16
	
	ld	a,TACF_START|TACF_16KHZ
	ld	[rTAC],a
	ld	a,0
	ld	[rTMA],a
	ld	[rDIV],a
	ld	[rTIMA],a
	ld	[rIF],a
	
	ld	a,VALUE_WRITEN & $FF
	ld	[rDIV],a ; sync
	
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
	
	ld	[rTMA],a ; write just when internal counter goes from 0x00FF to 0x0100
	
	ld	a,[rTMA]
	ld	[hl+],a
	ld	a,[rTIMA]
	ld	[hl+],a
	ld	a,[rIF]
	and	a,4
	ld	[hl+],a

VALUE_WRITEN SET VALUE_WRITEN+1
	ENDR	

	; -----------------------

VALUE_WRITEN SET 253
	REPT	16
	
	ld	a,TACF_START|TACF_16KHZ
	ld	[rTAC],a
	ld	a,0
	ld	[rTMA],a
	ld	[rDIV],a
	ld	[rIF],a
	ld	a,$FF
	ld	[rTIMA],a
	
	ld	a,VALUE_WRITEN & $FF
	ld	[rDIV],a ; sync
	
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
	
	ld	[rTMA],a ; write just when internal counter goes from 0x00FF to 0x0100
	
	ld	a,[rTMA]
	ld	[hl+],a
	ld	a,[rTIMA]
	ld	[hl+],a
	ld	a,[rIF]
	and	a,4
	ld	[hl+],a

VALUE_WRITEN SET VALUE_WRITEN+1
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
