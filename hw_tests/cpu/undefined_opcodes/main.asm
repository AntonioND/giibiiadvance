
	INCLUDE	"hardware.inc"
	INCLUDE "header.inc"


	SECTION	"Main",HOME

;--------------------------------------------------------------------------
;- Main()                                                                 -
;--------------------------------------------------------------------------

Main:
	
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
	
.wait_no_press:
	call	scan_keys
	ld	a,[joy_held]
	and	a,(~PAD_START)&$FF
	jr	z,.wait_no_press


	
	ld	a,[joy_held]
	and	a,PAD_START
	jr	nz,.start
	
	
	
	ld	a,[joy_held]
	and	a,PAD_A
	jr	z,.pad_a1
	DB	$D3
.pad_a1:
	
	ld	a,[joy_held]
	and	a,PAD_B
	jr	z,.pad_b1
	DB	$DB
.pad_b1:
	
	ld	a,[joy_held]
	and	a,PAD_UP
	jr	z,.pad_up1
	DB	$DD
.pad_up1:
	
	ld	a,[joy_held]
	and	a,PAD_RIGHT
	jr	z,.pad_ri1
	DB	$E3
.pad_ri1:
	
	
	ld	a,[joy_held]
	and	a,PAD_DOWN
	jr	z,.pad_do1
	DB	$E4
.pad_do1:
	
	ld	a,[joy_held]
	and	a,PAD_LEFT
	jr	z,.pad_le1
	DB	$EB
.pad_le1:
	
	ld	a,[joy_held]
	and	a,PAD_SELECT
	jr	z,.pad_sel1
	DB	$EC
.pad_sel1:
	


	jp	.end

.start:
	
	
	
	
	
	ld	a,[joy_held]
	and	a,PAD_UP
	jr	z,.pad_up2
	DB	$ED
.pad_up2:
	
	ld	a,[joy_held]
	and	a,PAD_RIGHT
	jr	z,.pad_ri2
	DB	$F4
.pad_ri2:
	
	
	ld	a,[joy_held]
	and	a,PAD_DOWN
	jr	z,.pad_do2
	DB	$FC
.pad_do2:
	
	ld	a,[joy_held]
	and	a,PAD_LEFT
	jr	z,.pad_le2
	DB	$FD
.pad_le2:
	

	
	
	
.end:
	
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
	
	ld	a,$C0
	ld	[rNR11],a
	ld	a,$E0
	ld	[rNR12],a
	ld	a,$00
	ld	[rNR13],a
	ld	a,$80
	ld	[rNR14],a
	
.loop:
	halt
	jr	.loop
	
	
	
	
	