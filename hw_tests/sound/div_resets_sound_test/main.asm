
	INCLUDE	"hardware.inc"
	INCLUDE "header.inc"

	
	SECTION	"var",BSS
	
ram_ptr:	DS 2
repeat_loop:	DS 1

	SECTION	"Main",HOME

;--------------------------------------------------------------------------

wait_no_halt:
	call	scan_keys
	ld	a,[joy_held]
	and	a,PAD_A
	jr	z,wait_no_halt
	xor	a,a
	ld	[rNR10],a
	ld	[rNR11],a
	ld	[rNR12],a
	ld	[rNR13],a
	ld	[rNR14],a
	ld	[rNR41],a
	ld	[rNR42],a
	ld	[rNR43],a
	ld	[rNR44],a
.wait_no_press1:
	call	scan_keys
	ld	a,[joy_held]
	and	a,PAD_B
	jr	z,.wait_no_press1
	ret

wait_halt:
	xor	a,a
	ld	[rDIV],a
	call	scan_keys
	ld	a,[joy_held]
	and	a,PAD_A
	jr	z,wait_halt
	xor	a,a
	ld	[rNR10],a
	ld	[rNR11],a
	ld	[rNR12],a
	ld	[rNR13],a
	ld	[rNR14],a
	ld	[rNR41],a
	ld	[rNR42],a
	ld	[rNR43],a
	ld	[rNR44],a
.wait_no_press2:
	call	scan_keys
	ld	a,[joy_held]
	and	a,PAD_B
	jr	z,.wait_no_press2
	ret

;--------------------------------------------------------------------------
;- Main()                                                                 -
;--------------------------------------------------------------------------

Main:
	
	ld	a,[Init_Reg_A]
	cp	a,$11
	jr	nz,.non_gbc
	call	scan_keys
	ld	a,[joy_held]
	and	a,PAD_B
	call	nz,CPU_fast
.non_gbc:
	
	ld	a,$80
	ld	[rNR52],a
	ld	a,$FF
	ld	[rNR51],a
	ld	a,$77
	ld	[rNR50],a
	
	;--------------------------------
	
	ld	a,$C0
	ld	[rNR11],a
	ld	a,$E0
	ld	[rNR12],a
	ld	a,$00
	ld	[rNR13],a
	ld	a,$C7
	ld	[rNR14],a
	
	call	wait_no_halt
	
	ld	a,$C0
	ld	[rNR11],a
	ld	a,$E0
	ld	[rNR12],a
	ld	a,$00
	ld	[rNR13],a
	ld	a,$C7
	ld	[rNR14],a

	call	wait_halt
	
	;--------------------------------

	ld	a,$C0
	ld	[rNR11],a
	ld	a,$E7
	ld	[rNR12],a
	ld	a,$00
	ld	[rNR13],a
	ld	a,$87
	ld	[rNR14],a
	
	call	wait_no_halt
	
	ld	a,$C0
	ld	[rNR11],a
	ld	a,$E7
	ld	[rNR12],a
	ld	a,$00
	ld	[rNR13],a
	ld	a,$87
	ld	[rNR14],a

	call	wait_halt

	;--------------------------------
	
	ld	a,$7C
	ld	[rNR10],a
	ld	a,$C0
	ld	[rNR11],a
	ld	a,$E0
	ld	[rNR12],a
	ld	a,$00
	ld	[rNR13],a
	ld	a,$87
	ld	[rNR14],a
	
	call	wait_no_halt
	
	ld	a,$7C
	ld	[rNR10],a
	ld	a,$C0
	ld	[rNR11],a
	ld	a,$E0
	ld	[rNR12],a
	ld	a,$00
	ld	[rNR13],a
	ld	a,$87
	ld	[rNR14],a

	call	wait_halt

	;--------------------------------
	
	ld	a,$C0
	ld	[rNR11],a
	ld	a,$E0
	ld	[rNR12],a
	ld	a,$00
	ld	[rNR13],a
	ld	a,$80
	ld	[rNR14],a
	
	ld	a,$00
	ld	[rNR41],a
	ld	a,$E0
	ld	[rNR42],a
	ld	a,$00
	ld	[rNR43],a
	ld	a,$80
	ld	[rNR44],a

	call	wait_no_halt
	
	ld	a,$C0
	ld	[rNR11],a
	ld	a,$E0
	ld	[rNR12],a
	ld	a,$00
	ld	[rNR13],a
	ld	a,$80
	ld	[rNR14],a
	
	ld	a,$00
	ld	[rNR41],a
	ld	a,$E0
	ld	[rNR42],a
	ld	a,$00
	ld	[rNR43],a
	ld	a,$80
	ld	[rNR44],a

	xor	a,a
	ld	c,rDIV&$FF
.loopend:
	REPT	64
	ld	[$FF00+c],a
	ENDR
	jr	.loopend


.loop:
	halt
	jr	.loop

;--------------------------------------------------------------------------
