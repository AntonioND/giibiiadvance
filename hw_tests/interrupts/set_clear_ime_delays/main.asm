
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
	
	ld	b,0
	
	; test EI delay
	
	di
	ld	a,IEF_TIMER
	ld	[rIE],a
	ld	[rIF],a
	xor	a,a
	ei
	inc	a
	inc	a
	inc	a
	inc	a
	inc	a
	inc	a
	inc	a
	inc	a
	inc	a
	
	; test RETI delay
	
	di
	ld	a,IEF_SERIAL
	ld	[rIE],a
	ld	[rIF],a
	xor	a,a
	ei
	inc	a
	inc	a
	inc	a
	inc	a
	inc	a
	inc	a
	inc	a
	inc	a
	inc	a

	di
	
	nop
	nop
	nop
	
	; now, test di for delays
	;------------------------
	
	ld	a,0
	ld	[rIE],a
	
	ld	a,TACF_STOP
	ld	[rTAC],a
	
	xor	a,a
	ld	[rIF],a
	
	ld	a,IEF_TIMER
	ld	[rIE],a
	
	
	
REPETITIONS SET 0
	REPT 64
	
	di
	ld	a,TACF_65KHZ|TACF_STOP
	ld	[rTAC],a
	
	xor	a,a
	ld	[rTMA],a  ; 00
	ld	[rIF],a
	dec	a
	dec	a
	ld	[rTIMA],a ; FE
	ld	[rDIV],a
	
	ld	a,TACF_65KHZ|TACF_START
	ld	[rTAC],a
	
	xor	a,a ; a = 0
	ld	b,a ; b = 0
	ei
	
	REPT REPETITIONS
	inc a
	ENDR
	di
	inc b
	inc b
	inc b
	inc b
	inc b	
	
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
