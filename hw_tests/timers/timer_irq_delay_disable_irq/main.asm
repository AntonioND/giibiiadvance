
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
	
	di
	ld	a,IEF_TIMER
	ld	[rIE],a
	ld	a,TACF_START|TACF_16KHZ
	ld	[rTAC],a
	ld	a,0
	ld	[rTMA],a
	ld	[rDIV],a
	dec	a ; a = $FF
	ld	[rTIMA],a
	inc a ; a = 0
	ld	[rIF],a
	ld	[rDIV],a
	ei
	ld	b,a ; b = 0
	ld	c,rIF & $FF
	ld	[rDIV],a ; sync
	
	REPT	$40-3
	inc	b
	ENDR
	ld	[$FF00+c],a
	
	REPT	10
	nop
	ENDR
	
	; --------
	
	di
	ld	a,IEF_TIMER
	ld	[rIE],a
	ld	a,TACF_START|TACF_16KHZ
	ld	[rTAC],a
	ld	a,0
	ld	[rTMA],a
	ld	[rDIV],a
	dec	a ; a = $FF
	ld	[rTIMA],a
	inc a ; a = 0
	ld	[rIF],a
	ld	[rDIV],a
	ei
	ld	b,a ; b = 0
	ld	c,rIF & $FF
	ld	[rDIV],a ; sync
	
	REPT	$40-2
	inc	b
	ENDR
	ld	[$FF00+c],a
	
	REPT	10
	nop
	ENDR
	
	; --------
	
	di
	ld	a,IEF_TIMER
	ld	[rIE],a
	ld	a,TACF_START|TACF_16KHZ
	ld	[rTAC],a
	ld	a,0
	ld	[rTMA],a
	ld	[rDIV],a
	dec	a ; a = $FF
	ld	[rTIMA],a
	inc a ; a = 0
	ld	[rIF],a
	ld	[rDIV],a
	ei
	ld	b,a ; b = 0
	ld	c,rIF & $FF
	ld	[rDIV],a ; sync
	
	REPT	$40-1
	inc	b
	ENDR
	ld	[$FF00+c],a
	
	REPT	10
	nop
	ENDR
	
	; --------
	
	di
	ld	a,IEF_TIMER
	ld	[rIE],a
	ld	a,TACF_START|TACF_16KHZ
	ld	[rTAC],a
	ld	a,0
	ld	[rTMA],a
	ld	[rDIV],a
	dec	a ; a = $FF
	ld	[rTIMA],a
	inc a ; a = 0
	ld	[rIF],a
	ld	[rDIV],a
	ei
	ld	b,a ; b = 0
	ld	c,rIF & $FF
	ld	[rDIV],a ; sync
	
	REPT	$40
	inc	b
	ENDR
	ld	[$FF00+c],a
	
	REPT	10
	nop
	ENDR
	
	; --------
	
	di
	ld	a,IEF_TIMER
	ld	[rIE],a
	ld	a,TACF_START|TACF_16KHZ
	ld	[rTAC],a
	ld	a,0
	ld	[rTMA],a
	ld	[rDIV],a
	dec	a ; a = $FF
	ld	[rTIMA],a
	inc a ; a = 0
	ld	[rIF],a
	ld	[rDIV],a
	ei
	ld	b,a ; b = 0
	ld	c,rIF & $FF
	ld	[rDIV],a ; sync
	
	REPT	$41
	inc	b
	ENDR
	ld	[$FF00+c],a
	
	REPT	10
	nop
	ENDR
	
	ld	a,"z"
	ld	[hl+],a
	
	; -------------------------------------------------------
	
	di
	ld	a,IEF_TIMER
	ld	[rIE],a
	ld	a,TACF_START|TACF_16KHZ
	ld	[rTAC],a
	ld	a,0
	ld	[rTMA],a
	ld	[rDIV],a
	dec	a ; a = $FF
	ld	[rTIMA],a
	inc a ; a = 0
	ld	[rIF],a
	ld	[rDIV],a
	ei
	ld	b,a ; b = 0
	ld	c,rTIMA & $FF
	ld	[rDIV],a ; sync
	
	REPT	$40-3
	inc	b
	ENDR
	ld	[$FF00+c],a
	
	REPT	10
	nop
	ENDR
	
	; --------
	
	di
	ld	a,IEF_TIMER
	ld	[rIE],a
	ld	a,TACF_START|TACF_16KHZ
	ld	[rTAC],a
	ld	a,0
	ld	[rTMA],a
	ld	[rDIV],a
	dec	a ; a = $FF
	ld	[rTIMA],a
	inc a ; a = 0
	ld	[rIF],a
	ld	[rDIV],a
	ei
	ld	b,a ; b = 0
	ld	c,rTIMA & $FF
	ld	[rDIV],a ; sync
	
	REPT	$40-2
	inc	b
	ENDR
	ld	[$FF00+c],a
	
	REPT	10
	nop
	ENDR
	
	; --------
	
	di
	ld	a,IEF_TIMER
	ld	[rIE],a
	ld	a,TACF_START|TACF_16KHZ
	ld	[rTAC],a
	ld	a,0
	ld	[rTMA],a
	ld	[rDIV],a
	dec	a ; a = $FF
	ld	[rTIMA],a
	inc a ; a = 0
	ld	[rIF],a
	ld	[rDIV],a
	ei
	ld	b,a ; b = 0
	ld	c,rTIMA & $FF
	ld	[rDIV],a ; sync
	
	REPT	$40-1
	inc	b
	ENDR
	ld	[$FF00+c],a
	
	REPT	10
	nop
	ENDR
	
	; --------
	
	di
	ld	a,IEF_TIMER
	ld	[rIE],a
	ld	a,TACF_START|TACF_16KHZ
	ld	[rTAC],a
	ld	a,0
	ld	[rTMA],a
	ld	[rDIV],a
	dec	a ; a = $FF
	ld	[rTIMA],a
	inc a ; a = 0
	ld	[rIF],a
	ld	[rDIV],a
	ei
	ld	b,a ; b = 0
	ld	c,rTIMA & $FF
	ld	[rDIV],a ; sync
	
	REPT	$40
	inc	b
	ENDR
	ld	[$FF00+c],a
	
	REPT	10
	nop
	ENDR
	
	; --------
	
	di
	ld	a,IEF_TIMER
	ld	[rIE],a
	ld	a,TACF_START|TACF_16KHZ
	ld	[rTAC],a
	ld	a,0
	ld	[rTMA],a
	ld	[rDIV],a
	dec	a ; a = $FF
	ld	[rTIMA],a
	inc a ; a = 0
	ld	[rIF],a
	ld	[rDIV],a
	ei
	ld	b,a ; b = 0
	ld	c,rTIMA & $FF
	ld	[rDIV],a ; sync
	
	REPT	$41
	inc	b
	ENDR
	ld	[$FF00+c],a
	
	REPT	10
	nop
	ENDR
	
	ld	a,"z"
	ld	[hl+],a
	
	; -------------------------------------------------------
	
	di
	ld	a,IEF_TIMER
	ld	[rIE],a
	ld	a,TACF_START|TACF_16KHZ
	ld	[rTAC],a
	ld	a,0
	ld	[rTMA],a
	ld	[rDIV],a
	dec	a ; a = $FF
	ld	[rTIMA],a
	inc a ; a = 0
	ld	[rIF],a
	ld	[rDIV],a
	ei
	ld	b,a ; b = 0
	ld	c,rTAC & $FF
	ld	[rDIV],a ; sync
	
	REPT	$40-3
	inc	b
	ENDR
	ld	[$FF00+c],a
	
	REPT	10
	nop
	ENDR
	
	; --------
	
	di
	ld	a,IEF_TIMER
	ld	[rIE],a
	ld	a,TACF_START|TACF_16KHZ
	ld	[rTAC],a
	ld	a,0
	ld	[rTMA],a
	ld	[rDIV],a
	dec	a ; a = $FF
	ld	[rTIMA],a
	inc a ; a = 0
	ld	[rIF],a
	ld	[rDIV],a
	ei
	ld	b,a ; b = 0
	ld	c,rTAC & $FF
	ld	[rDIV],a ; sync
	
	REPT	$40-2
	inc	b
	ENDR
	ld	[$FF00+c],a
	
	REPT	10
	nop
	ENDR
	
	; --------
	
	di
	ld	a,IEF_TIMER
	ld	[rIE],a
	ld	a,TACF_START|TACF_16KHZ
	ld	[rTAC],a
	ld	a,0
	ld	[rTMA],a
	ld	[rDIV],a
	dec	a ; a = $FF
	ld	[rTIMA],a
	inc a ; a = 0
	ld	[rIF],a
	ld	[rDIV],a
	ei
	ld	b,a ; b = 0
	ld	c,rTAC & $FF
	ld	[rDIV],a ; sync
	
	REPT	$40-1
	inc	b
	ENDR
	ld	[$FF00+c],a
	
	REPT	10
	nop
	ENDR
	
	; --------
	
	di
	ld	a,IEF_TIMER
	ld	[rIE],a
	ld	a,TACF_START|TACF_16KHZ
	ld	[rTAC],a
	ld	a,0
	ld	[rTMA],a
	ld	[rDIV],a
	dec	a ; a = $FF
	ld	[rTIMA],a
	inc a ; a = 0
	ld	[rIF],a
	ld	[rDIV],a
	ei
	ld	b,a ; b = 0
	ld	c,rTAC & $FF
	ld	[rDIV],a ; sync
	
	REPT	$40
	inc	b
	ENDR
	ld	[$FF00+c],a
	
	REPT	10
	nop
	ENDR
	
	; --------
	
	di
	ld	a,IEF_TIMER
	ld	[rIE],a
	ld	a,TACF_START|TACF_16KHZ
	ld	[rTAC],a
	ld	a,0
	ld	[rTMA],a
	ld	[rDIV],a
	dec	a ; a = $FF
	ld	[rTIMA],a
	inc a ; a = 0
	ld	[rIF],a
	ld	[rDIV],a
	ei
	ld	b,a ; b = 0
	ld	c,rTAC & $FF
	ld	[rDIV],a ; sync
	
	REPT	$41
	inc	b
	ENDR
	ld	[$FF00+c],a
	
	REPT	10
	nop
	ENDR
	
	
	ld	a,"z"
	ld	[hl+],a
	
	; -------------------------------------------------------
	
	di
	ld	a,IEF_TIMER
	ld	[rIE],a
	ld	a,TACF_START|TACF_16KHZ
	ld	[rTAC],a
	ld	a,0
	ld	[rTMA],a
	ld	[rDIV],a
	dec	a ; a = $FF
	ld	[rTIMA],a
	inc a ; a = 0
	ld	[rIF],a
	ld	[rDIV],a
	ei
	ld	b,a ; b = 0
	ld	a,$10
	ld	c,rTMA & $FF
	ld	[rDIV],a ; sync
	
	REPT	$40-3
	inc	b
	ENDR
	ld	[$FF00+c],a
	
	REPT	10
	nop
	ENDR
	
	; --------
	
	di
	ld	a,IEF_TIMER
	ld	[rIE],a
	ld	a,TACF_START|TACF_16KHZ
	ld	[rTAC],a
	ld	a,0
	ld	[rTMA],a
	ld	[rDIV],a
	dec	a ; a = $FF
	ld	[rTIMA],a
	inc a ; a = 0
	ld	[rIF],a
	ld	[rDIV],a
	ei
	ld	b,a ; b = 0
	ld	a,$10
	ld	c,rTMA & $FF
	ld	[rDIV],a ; sync
	
	REPT	$40-2
	inc	b
	ENDR
	ld	[$FF00+c],a
	
	REPT	10
	nop
	ENDR
	
	; --------
	
	di
	ld	a,IEF_TIMER
	ld	[rIE],a
	ld	a,TACF_START|TACF_16KHZ
	ld	[rTAC],a
	ld	a,0
	ld	[rTMA],a
	ld	[rDIV],a
	dec	a ; a = $FF
	ld	[rTIMA],a
	inc a ; a = 0
	ld	[rIF],a
	ld	[rDIV],a
	ei
	ld	b,a ; b = 0
	ld	a,$10
	ld	c,rTMA & $FF
	ld	[rDIV],a ; sync
	
	REPT	$40-1
	inc	b
	ENDR
	ld	[$FF00+c],a
	
	REPT	10
	nop
	ENDR
	
	; --------
	
	di
	ld	a,IEF_TIMER
	ld	[rIE],a
	ld	a,TACF_START|TACF_16KHZ
	ld	[rTAC],a
	ld	a,0
	ld	[rTMA],a
	ld	[rDIV],a
	dec	a ; a = $FF
	ld	[rTIMA],a
	inc a ; a = 0
	ld	[rIF],a
	ld	[rDIV],a
	ei
	ld	b,a ; b = 0
	ld	a,$10
	ld	c,rTMA & $FF
	ld	[rDIV],a ; sync
	
	REPT	$40
	inc	b
	ENDR
	ld	[$FF00+c],a
	
	REPT	10
	nop
	ENDR
	
	; --------
	
	di
	ld	a,IEF_TIMER
	ld	[rIE],a
	ld	a,TACF_START|TACF_16KHZ
	ld	[rTAC],a
	ld	a,0
	ld	[rTMA],a
	ld	[rDIV],a
	dec	a ; a = $FF
	ld	[rTIMA],a
	inc a ; a = 0
	ld	[rIF],a
	ld	[rDIV],a
	ei
	ld	b,a ; b = 0
	ld	a,$10
	ld	c,rTMA & $FF
	ld	[rDIV],a ; sync
	
	REPT	$41
	inc	b
	ENDR
	ld	[$FF00+c],a
	
	REPT	10
	nop
	ENDR
	
	
	ld	a,"z"
	ld	[hl+],a
	
	; -------------------------------------------------------
	
	di
	ld	a,IEF_TIMER
	ld	[rIE],a
	ld	a,TACF_START|TACF_16KHZ
	ld	[rTAC],a
	ld	a,0
	ld	[rTMA],a
	ld	[rDIV],a
	dec	a ; a = $FF
	ld	[rTIMA],a
	inc a ; a = 0
	ld	[rIF],a
	ld	[rDIV],a
	ei
	ld	b,a ; b = 0
	ld	c,rIE & $FF
	ld	[rDIV],a ; sync
	
	REPT	$40-3
	inc	b
	ENDR
	ld	[$FF00+c],a
	
	REPT	10
	nop
	ENDR
	
	; --------
	
	di
	ld	a,IEF_TIMER
	ld	[rIE],a
	ld	a,TACF_START|TACF_16KHZ
	ld	[rTAC],a
	ld	a,0
	ld	[rTMA],a
	ld	[rDIV],a
	dec	a ; a = $FF
	ld	[rTIMA],a
	inc a ; a = 0
	ld	[rIF],a
	ld	[rDIV],a
	ei
	ld	b,a ; b = 0
	ld	c,rIE & $FF
	ld	[rDIV],a ; sync
	
	REPT	$40-2
	inc	b
	ENDR
	ld	[$FF00+c],a
	
	REPT	10
	nop
	ENDR
	
	; --------
	
	di
	ld	a,IEF_TIMER
	ld	[rIE],a
	ld	a,TACF_START|TACF_16KHZ
	ld	[rTAC],a
	ld	a,0
	ld	[rTMA],a
	ld	[rDIV],a
	dec	a ; a = $FF
	ld	[rTIMA],a
	inc a ; a = 0
	ld	[rIF],a
	ld	[rDIV],a
	ei
	ld	b,a ; b = 0
	ld	c,rIE & $FF
	ld	[rDIV],a ; sync
	
	REPT	$40-1
	inc	b
	ENDR
	ld	[$FF00+c],a
	
	REPT	10
	nop
	ENDR
	
	; --------
	
	di
	ld	a,IEF_TIMER
	ld	[rIE],a
	ld	a,TACF_START|TACF_16KHZ
	ld	[rTAC],a
	ld	a,0
	ld	[rTMA],a
	ld	[rDIV],a
	dec	a ; a = $FF
	ld	[rTIMA],a
	inc a ; a = 0
	ld	[rIF],a
	ld	[rDIV],a
	ei
	ld	b,a ; b = 0
	ld	c,rIE & $FF
	ld	[rDIV],a ; sync
	
	REPT	$40
	inc	b
	ENDR
	ld	[$FF00+c],a
	
	REPT	10
	nop
	ENDR
	
	; --------
	
	di
	ld	a,IEF_TIMER
	ld	[rIE],a
	ld	a,TACF_START|TACF_16KHZ
	ld	[rTAC],a
	ld	a,0
	ld	[rTMA],a
	ld	[rDIV],a
	dec	a ; a = $FF
	ld	[rTIMA],a
	inc a ; a = 0
	ld	[rIF],a
	ld	[rDIV],a
	ei
	ld	b,a ; b = 0
	ld	c,rIE & $FF
	ld	[rDIV],a ; sync
	
	REPT	$41
	inc	b
	ENDR
	ld	[$FF00+c],a
	
	REPT	10
	nop
	ENDR

	
	ld	a,"z"
	ld	[hl+],a
	
	; -------------------------------------------------------
	
	di
	ld	a,IEF_TIMER
	ld	[rIE],a
	ld	a,TACF_START|TACF_16KHZ
	ld	[rTAC],a
	ld	a,0
	ld	[rTMA],a
	ld	[rDIV],a
	dec	a ; a = $FF
	ld	[rTIMA],a
	inc a ; a = 0
	ld	[rIF],a
	ld	[rDIV],a
	ei
	ld	b,a ; b = 0
	ld	[rDIV],a ; sync
	
	REPT	$40-2
	inc	b
	ENDR
	di
	
	REPT	10
	nop
	ENDR
	
	; --------
	
	di
	ld	a,IEF_TIMER
	ld	[rIE],a
	ld	a,TACF_START|TACF_16KHZ
	ld	[rTAC],a
	ld	a,0
	ld	[rTMA],a
	ld	[rDIV],a
	dec	a ; a = $FF
	ld	[rTIMA],a
	inc a ; a = 0
	ld	[rIF],a
	ld	[rDIV],a
	ei
	ld	b,a ; b = 0
	ld	[rDIV],a ; sync
	
	REPT	$40-1
	inc	b
	ENDR
	di
	
	REPT	10
	nop
	ENDR
	
	; --------
	
	di
	ld	a,IEF_TIMER
	ld	[rIE],a
	ld	a,TACF_START|TACF_16KHZ
	ld	[rTAC],a
	ld	a,0
	ld	[rTMA],a
	ld	[rDIV],a
	dec	a ; a = $FF
	ld	[rTIMA],a
	inc a ; a = 0
	ld	[rIF],a
	ld	[rDIV],a
	ei
	ld	b,a ; b = 0
	ld	[rDIV],a ; sync
	
	REPT	$40
	inc	b
	ENDR
	di
	
	REPT	10
	nop
	ENDR
	
	; --------
	
	di
	ld	a,IEF_TIMER
	ld	[rIE],a
	ld	a,TACF_START|TACF_16KHZ
	ld	[rTAC],a
	ld	a,0
	ld	[rTMA],a
	ld	[rDIV],a
	dec	a ; a = $FF
	ld	[rTIMA],a
	inc a ; a = 0
	ld	[rIF],a
	ld	[rDIV],a
	ei
	ld	b,a ; b = 0
	ld	[rDIV],a ; sync
	
	REPT	$41
	inc	b
	ENDR
	di
	
	REPT	10
	nop
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
