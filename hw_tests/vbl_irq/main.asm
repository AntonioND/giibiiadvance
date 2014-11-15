
	INCLUDE	"hardware.inc"
	INCLUDE "header.inc"

	
	SECTION	"var",BSS
	

ram_ptr:	DS 2
	

	SECTION	"Main",HOME

;--------------------------------------------------------------------------
;- Main()                                                                 -
;--------------------------------------------------------------------------

output_value:
	
	ld	a,$0A
	ld	[$0000],a ; enable ram
	
	; c = data
	
	ld	a,[ram_ptr+0]
	ld	l,a
	ld	a,[ram_ptr+1]
	ld	h,a
	
	ld	[hl],c
	inc hl
	
	ld	a,h
	ld	[ram_ptr+1],a
	ld	a,l
	ld	[ram_ptr+0],a
	
	ld	[hl],$12
	inc hl
	ld	[hl],$34
	inc hl
	ld	[hl],$56
	inc hl
	ld	[hl],$78
	
	ld	a,$00
	ld	[$0000],a ; disable ram
	
	reti
	

Main:
	
	di
	ld	a,1
	ld	[rIE],a
	ld	a,0
	ld	[rIF],a
	ld	[ram_ptr+0],a
	ld	a,$A0
	ld	[ram_ptr+1],a
	
	ld	bc,output_value
	call	irq_set_VBL
	
	ei
	
	ld	a,$80
	ld	[rLCDC],a
	
.loop:
	
	ld	b,143
	call	wait_ly
	call	wait_screen_blank
	
	REPT 8
	nop
	ENDR
	
	ld	c,0
	REPT 200
	inc	c
	ENDR
	
	jp	.loop
