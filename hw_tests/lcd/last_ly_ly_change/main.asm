
	INCLUDE	"hardware.inc"
	INCLUDE "header.inc"

	
	SECTION	"var",BSS
	
repeat_loop:	DS 1
sram_ptr:		DS 2 ; MSB first

	SECTION	"Main",HOME
	
;--------------------------------------------------------------------------
;- Main()                                                                 -
;--------------------------------------------------------------------------

Main:
	
	ld	a,$0A
	ld	[$0000],a ; enable ram
	
	ld	a,$A0
	ld	[sram_ptr+0],a
	ld	a,$00
	ld	[sram_ptr+1],a
	
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

VBL_TEST : MACRO
	di
	
	ld	b,130
	call	wait_ly
	
	ld	a,152
	ld	[rLYC],a
	ld	a,STATF_LYC
	ld	[rSTAT],a
	
	ld	a,IEF_LCDC
	ld	[rIE],a
	
	xor	a,a
	ld	[rIF],a
	
	
	ld	bc,\1
	call	irq_set_LCD
	
	ei

	halt
ENDM

	VBL_TEST VBL_INT_HANDLER_0
	VBL_TEST VBL_INT_HANDLER_1
	VBL_TEST VBL_INT_HANDLER_2
	VBL_TEST VBL_INT_HANDLER_3
	VBL_TEST VBL_INT_HANDLER_4
	VBL_TEST VBL_INT_HANDLER_5
	VBL_TEST VBL_INT_HANDLER_6
	VBL_TEST VBL_INT_HANDLER_7
	VBL_TEST VBL_INT_HANDLER_8
	VBL_TEST VBL_INT_HANDLER_9
	VBL_TEST VBL_INT_HANDLER_10
	VBL_TEST VBL_INT_HANDLER_11
	VBL_TEST VBL_INT_HANDLER_12
	VBL_TEST VBL_INT_HANDLER_13
	VBL_TEST VBL_INT_HANDLER_14
	VBL_TEST VBL_INT_HANDLER_15

	VBL_TEST VBL_INT_HANDLER_GBC_0
	VBL_TEST VBL_INT_HANDLER_GBC_1
	VBL_TEST VBL_INT_HANDLER_GBC_2
	VBL_TEST VBL_INT_HANDLER_GBC_3
	VBL_TEST VBL_INT_HANDLER_GBC_4
	VBL_TEST VBL_INT_HANDLER_GBC_5
	VBL_TEST VBL_INT_HANDLER_GBC_6
	VBL_TEST VBL_INT_HANDLER_GBC_7
	VBL_TEST VBL_INT_HANDLER_GBC_8
	VBL_TEST VBL_INT_HANDLER_GBC_9
	VBL_TEST VBL_INT_HANDLER_GBC_10
	VBL_TEST VBL_INT_HANDLER_GBC_11
	VBL_TEST VBL_INT_HANDLER_GBC_12
	VBL_TEST VBL_INT_HANDLER_GBC_13
	VBL_TEST VBL_INT_HANDLER_GBC_14
	VBL_TEST VBL_INT_HANDLER_GBC_15







	; -------------------------------------------------------
	
	ld	a,[sram_ptr+0]
	ld	h,a
	ld	a,[sram_ptr+1]
	ld	l,a
	
	ld	[hl],$12
	inc hl
	ld	[hl],$34
	inc hl
	ld	[hl],$56
	inc hl
	ld	[hl],$78
	
	
	
	; -------------------------------------------------------
	
	ld	a,[Init_Reg_A]
	cp	a,$11
	jr	nz,.dontchange
	ld	a,[repeat_loop]
	and	a,a
	jr	nz,.dontchange
	; -------------------------------------------------------

	call	CPU_fast
	ld	a,1
	ld	[repeat_loop],a
	jp	.repeat_all
.dontchange:
	
	; -------------------------------------------------------

	ld	a,$00
	ld	[$0000],a ; disable ram
	
	call	screen_off
	
	ld	a,$80
	ld	[rNR52],a
	ld	a,$FF
	ld	[rNR51],a
	ld	a,$77
	ld	[rNR50],a
	
	ld	a,$C0
	ld	[rNR11],a
	ld	a,$E0
	ld	[rNR12],a
	ld	a,$00
	ld	[rNR13],a
	ld	a,$87
	ld	[rNR14],a
	

.endloop:
	halt
	jr	.endloop

; --------------------------------------------------------------

	SECTION "functions",ROMX,BANK[1]
	
VBL_INT_HANDLER_MACRO : MACRO
	REPT 10
	push	de
	pop		de
	ENDR
	REPT \1
	nop
	ENDR
	ld	a,[rLY]
	ld	b,a
	
	ld	a,[sram_ptr+0]
	ld	h,a
	ld	a,[sram_ptr+1]
	ld	l,a
	
	ld	[hl],b
	inc	hl
	
	ld	a,h
	ld	[sram_ptr+0],a
	ld	a,l
	ld	[sram_ptr+1],a
	ret
ENDM

VBL_INT_HANDLER_0:
	VBL_INT_HANDLER_MACRO 0
VBL_INT_HANDLER_1:
	VBL_INT_HANDLER_MACRO 1
VBL_INT_HANDLER_2:
	VBL_INT_HANDLER_MACRO 2
VBL_INT_HANDLER_3:
	VBL_INT_HANDLER_MACRO 3
VBL_INT_HANDLER_4:
	VBL_INT_HANDLER_MACRO 4
VBL_INT_HANDLER_5:
	VBL_INT_HANDLER_MACRO 5
VBL_INT_HANDLER_6:
	VBL_INT_HANDLER_MACRO 6
VBL_INT_HANDLER_7:
	VBL_INT_HANDLER_MACRO 7
VBL_INT_HANDLER_8:
	VBL_INT_HANDLER_MACRO 8
VBL_INT_HANDLER_9:
	VBL_INT_HANDLER_MACRO 9
VBL_INT_HANDLER_10:
	VBL_INT_HANDLER_MACRO 10
VBL_INT_HANDLER_11:
	VBL_INT_HANDLER_MACRO 11
VBL_INT_HANDLER_12:
	VBL_INT_HANDLER_MACRO 12
VBL_INT_HANDLER_13:
	VBL_INT_HANDLER_MACRO 13
VBL_INT_HANDLER_14:
	VBL_INT_HANDLER_MACRO 14
VBL_INT_HANDLER_15:
	VBL_INT_HANDLER_MACRO 15

	
VBL_INT_HANDLER_GBC_MACRO : MACRO
	REPT 26
	push	de
	pop		de
	ENDR
	REPT \1
	nop
	ENDR
	ld	a,[rLY]
	ld	b,a
	
	ld	a,[sram_ptr+0]
	ld	h,a
	ld	a,[sram_ptr+1]
	ld	l,a
	
	ld	[hl],b
	inc	hl
	
	ld	a,h
	ld	[sram_ptr+0],a
	ld	a,l
	ld	[sram_ptr+1],a
	ret
ENDM

VBL_INT_HANDLER_GBC_0:
	VBL_INT_HANDLER_GBC_MACRO 0
VBL_INT_HANDLER_GBC_1:
	VBL_INT_HANDLER_GBC_MACRO 1
VBL_INT_HANDLER_GBC_2:
	VBL_INT_HANDLER_GBC_MACRO 2
VBL_INT_HANDLER_GBC_3:
	VBL_INT_HANDLER_GBC_MACRO 3
VBL_INT_HANDLER_GBC_4:
	VBL_INT_HANDLER_GBC_MACRO 4
VBL_INT_HANDLER_GBC_5:
	VBL_INT_HANDLER_GBC_MACRO 5
VBL_INT_HANDLER_GBC_6:
	VBL_INT_HANDLER_GBC_MACRO 6
VBL_INT_HANDLER_GBC_7:
	VBL_INT_HANDLER_GBC_MACRO 7
VBL_INT_HANDLER_GBC_8:
	VBL_INT_HANDLER_GBC_MACRO 8
VBL_INT_HANDLER_GBC_9:
	VBL_INT_HANDLER_GBC_MACRO 9
VBL_INT_HANDLER_GBC_10:
	VBL_INT_HANDLER_GBC_MACRO 10
VBL_INT_HANDLER_GBC_11:
	VBL_INT_HANDLER_GBC_MACRO 11
VBL_INT_HANDLER_GBC_12:
	VBL_INT_HANDLER_GBC_MACRO 12
VBL_INT_HANDLER_GBC_13:
	VBL_INT_HANDLER_GBC_MACRO 13
VBL_INT_HANDLER_GBC_14:
	VBL_INT_HANDLER_GBC_MACRO 14
VBL_INT_HANDLER_GBC_15:
	VBL_INT_HANDLER_GBC_MACRO 15

; --------------------------------------------------------------

