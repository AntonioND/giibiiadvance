
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
	
	di
	ld	a,TACF_STOP
	ld	[rTAC],a
	
	;------------------------
	
	ld	a,IEF_TIMER
	ld	[rIE],a
	ld	[rIF],a
	db	$76 ; halt
	inc	a ; check halt bug
	ld	[hl+],a
	
	ei ; halt executed 
	inc	a
	inc	a
	inc	a
	nop
	nop
	
	; ----------------------------
	
	push	hl ; magic number
	ld	[hl],$12
	inc hl
	ld	[hl],$34
	inc hl
	ld	[hl],$56
	inc hl
	ld	[hl],$78
	pop	hl
	
	; ----------------------------
	
	
	; IE unused bits
	
	di
	ld	a,0
	ld	[rIF],a
	ld	a,$E0
	ld	[rIE],a
	ei
	db	$76 ; halt
	inc	a ; check halt bug
	ld	[hl+],a
	nop
	
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
