
	INCLUDE	"hardware.inc"
	INCLUDE "header.inc"

	
	SECTION	"var",BSS
	
repeat_loop:	DS 1

	SECTION	"Main",HOME

	GLOBAL LCD_function
LCD_function::
	push	af
	ld	a,[rLY]
	cp	a,152
	jr nz,._not_152
	; 152
	xor	a,a
	ld	[rLYC],a
	pop	af
	xor	a,a
	reti

._not_152:
	pop	af
	ld	[hl+],a
	ret
	
;--------------------------------------------------------------------------
;- Main()                                                                 -
;--------------------------------------------------------------------------

Main:
	
	ld	hl,$A000
	
	ld	a,LCDCF_ON
	ld	[rLCDC],a
	
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

	di
	
	ld	a,IEF_LCDC
	ld	[rIE],a
	ld	a,152
	ld	[rLYC],a
	ld	a,STATF_LYC
	ld	[rSTAT],a
	
	ld	b,152
	call	wait_ly
	ld	b,151
	call	wait_ly
	
	ld	a,0
	ld	[rIF],a
	
	ei
	
	REPT	2048
	inc	a
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
