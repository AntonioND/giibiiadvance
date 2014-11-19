
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

DELAY_253: MACRO
	ld 		b,49 		; 2
.loop\@:
	nop 				; 1
	dec		b 			; 1
	jr		nz,.loop\@ 	; 3 when jump, 2 when not
	; 2 + 48*(1+1+3) + 1+1+2 = 246
	push	bc			; 4
	pop		bc			; 3
	; 246 + 4 + 3 = 253
ENDM
	
	; Write TAC: DISABLED, WHEN INC, FROM 00
	
VALUE_WRITEN SET 0
	REPT	8
	
	ld	a,TACF_STOP|TACF_262KHZ
	ld	[rTAC],a
	ld	a,0
	ld	[rTMA],a
	ld	[rDIV],a
	ld	[rIF],a
	ld	[rTIMA],a
	
	ld	a,VALUE_WRITEN & $FF
	ld	[rDIV],a ; sync
	; 1
	DELAY_253
	; 2
	ld	[rTAC],a ; write just when internal counter goes from 0x00FF to 0x0100
	
	ld	a,[rTIMA]
	ld	[hl+],a
	ld	a,[rIF]
	and	a,4
	ld	[hl+],a

VALUE_WRITEN SET VALUE_WRITEN+1
	ENDR	

	; -------------------------------------------------------
	
	; Write TAC: DISABLED, WHEN INC, FROM FF
	
VALUE_WRITEN SET 0
	REPT	8
	
	ld	a,TACF_STOP|TACF_262KHZ
	ld	[rTAC],a
	ld	a,0
	ld	[rTMA],a
	ld	[rDIV],a
	ld	[rIF],a
	dec	a
	ld	[rTIMA],a
	
	ld	a,VALUE_WRITEN & $FF
	ld	[rDIV],a ; sync
	
	DELAY_253
	
	ld	[rTAC],a ; write just when internal counter goes from 0x00FF to 0x0100
	
	ld	a,[rTIMA]
	ld	[hl+],a
	ld	a,[rIF]
	and	a,4
	ld	[hl+],a

VALUE_WRITEN SET VALUE_WRITEN+1
	ENDR	
	
	; -------------------------------------------------------
	
	; Write TAC: DISABLED, NOT WHEN INC, FROM 00
	
VALUE_WRITEN SET 0
	REPT	8
	
	ld	a,TACF_STOP|TACF_262KHZ
	ld	[rTAC],a
	ld	a,0
	ld	[rTMA],a
	ld	[rDIV],a
	ld	[rIF],a
	ld	[rTIMA],a
	
	ld	a,VALUE_WRITEN & $FF
	ld	[rDIV],a ; sync
	
	nop
	nop
	nop
	
	ld	[rTAC],a
	
	ld	a,[rTIMA]
	ld	[hl+],a
	ld	a,[rIF]
	and	a,4
	ld	[hl+],a

VALUE_WRITEN SET VALUE_WRITEN+1
	ENDR	

	; -------------------------------------------------------
	
	; Write TAC: DISABLED, NOT WHEN INC, FROM FF
	
VALUE_WRITEN SET 0
	REPT	8
	
	ld	a,TACF_STOP|TACF_262KHZ
	ld	[rTAC],a
	ld	a,0
	ld	[rTMA],a
	ld	[rDIV],a
	ld	[rIF],a
	dec	a
	ld	[rTIMA],a
	
	ld	a,VALUE_WRITEN & $FF
	ld	[rDIV],a ; sync
	
	nop
	nop
	nop
	
	ld	[rTAC],a
	
	ld	a,[rTIMA]
	ld	[hl+],a
	ld	a,[rIF]
	and	a,4
	ld	[hl+],a

VALUE_WRITEN SET VALUE_WRITEN+1
	ENDR	

	; -------------------------------------------------------

	; -------------------------------------------------------
	
	; Write TAC: ENABLED, WHEN INC, FROM 00
	
VALUE_WRITEN SET 0
	REPT	8
	
	ld	a,TACF_START|TACF_262KHZ
	ld	[rTAC],a
	ld	a,0
	ld	[rTMA],a
	ld	[rDIV],a
	ld	[rIF],a
	ld	[rTIMA],a
	
	ld	a,VALUE_WRITEN & $FF
	ld	[rDIV],a ; sync
	
	DELAY_253
	
	ld	[rTAC],a ; write just when internal counter goes from 0x00FF to 0x0100
	
	ld	a,[rTIMA]
	ld	[hl+],a
	ld	a,[rIF]
	and	a,4
	ld	[hl+],a

VALUE_WRITEN SET VALUE_WRITEN+1
	ENDR	

	; -------------------------------------------------------
	
	; Write TAC: ENABLED, WHEN INC, FROM FF
	
VALUE_WRITEN SET 0
	REPT	8
	
	ld	a,TACF_START|TACF_262KHZ
	ld	[rTAC],a
	ld	a,0
	ld	[rTMA],a
	ld	[rDIV],a
	ld	[rIF],a
	dec	a
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
	
	ld	[rTAC],a ; write just when internal counter goes from 0x00FF to 0x0100
	
	ld	a,[rTIMA]
	ld	[hl+],a
	ld	a,[rIF]
	and	a,4
	ld	[hl+],a

VALUE_WRITEN SET VALUE_WRITEN+1
	ENDR	
	
	; -------------------------------------------------------
	
	; Write TAC: ENABLED, NOT WHEN INC, FROM 00
	
VALUE_WRITEN SET 0
	REPT	8
	
	ld	a,TACF_START|TACF_262KHZ
	ld	[rTAC],a
	ld	a,0
	ld	[rTMA],a
	ld	[rDIV],a
	ld	[rIF],a
	ld	[rTIMA],a
	
	ld	a,VALUE_WRITEN & $FF
	ld	[rDIV],a ; sync
	
	nop
	nop
	nop
	
	ld	[rTAC],a
	
	ld	a,[rTIMA]
	ld	[hl+],a
	ld	a,[rIF]
	and	a,4
	ld	[hl+],a

VALUE_WRITEN SET VALUE_WRITEN+1
	ENDR	

	; -------------------------------------------------------
	
	; Write TAC: ENABLED, NOT WHEN INC, FROM FF
	
VALUE_WRITEN SET 0
	REPT	8
	
	ld	a,TACF_START|TACF_262KHZ
	ld	[rTAC],a
	ld	a,0
	ld	[rTMA],a
	ld	[rDIV],a
	ld	[rIF],a
	dec	a
	ld	[rTIMA],a
	
	ld	a,VALUE_WRITEN & $FF
	ld	[rDIV],a ; sync
	
	nop
	nop
	nop
	
	ld	[rTAC],a
	
	ld	a,[rTIMA]
	ld	[hl+],a
	ld	a,[rIF]
	and	a,4
	ld	[hl+],a

VALUE_WRITEN SET VALUE_WRITEN+1
	ENDR	


	; -------------------------------------------------------

	; -------------------------------------------------------
	
	; -------------------------------------------------------

	; -------------------------------------------------------
	

	; Write TAC: DISABLED, WHEN INC, FROM 00
	
VALUE_WRITEN SET 0
	REPT	8
	
	ld	a,TACF_STOP|TACF_65KHZ
	ld	[rTAC],a
	ld	a,0
	ld	[rTMA],a
	ld	[rDIV],a
	ld	[rIF],a
	ld	[rTIMA],a
	
	ld	a,VALUE_WRITEN & $FF
	ld	[rDIV],a ; sync
	
	DELAY_253
	
	ld	[rTAC],a ; write just when internal counter goes from 0x00FF to 0x0100
	
	ld	a,[rTIMA]
	ld	[hl+],a
	ld	a,[rIF]
	and	a,4
	ld	[hl+],a

VALUE_WRITEN SET VALUE_WRITEN+1
	ENDR	

	; -------------------------------------------------------
	
	; Write TAC: DISABLED, WHEN INC, FROM FF
	
VALUE_WRITEN SET 0
	REPT	8
	
	ld	a,TACF_STOP|TACF_65KHZ
	ld	[rTAC],a
	ld	a,0
	ld	[rTMA],a
	ld	[rDIV],a
	ld	[rIF],a
	dec	a
	ld	[rTIMA],a
	
	ld	a,VALUE_WRITEN & $FF
	ld	[rDIV],a ; sync
	
	DELAY_253
	
	ld	[rTAC],a ; write just when internal counter goes from 0x00FF to 0x0100
	
	ld	a,[rTIMA]
	ld	[hl+],a
	ld	a,[rIF]
	and	a,4
	ld	[hl+],a

VALUE_WRITEN SET VALUE_WRITEN+1
	ENDR	
	
	; -------------------------------------------------------
	
	; Write TAC: DISABLED, NOT WHEN INC, FROM 00
	
VALUE_WRITEN SET 0
	REPT	8
	
	ld	a,TACF_STOP|TACF_65KHZ
	ld	[rTAC],a
	ld	a,0
	ld	[rTMA],a
	ld	[rDIV],a
	ld	[rIF],a
	ld	[rTIMA],a
	
	ld	a,VALUE_WRITEN & $FF
	ld	[rDIV],a ; sync
	
	nop
	nop
	nop
	
	ld	[rTAC],a
	
	ld	a,[rTIMA]
	ld	[hl+],a
	ld	a,[rIF]
	and	a,4
	ld	[hl+],a

VALUE_WRITEN SET VALUE_WRITEN+1
	ENDR	

	; -------------------------------------------------------
	
	; Write TAC: DISABLED, NOT WHEN INC, FROM FF
	
VALUE_WRITEN SET 0
	REPT	8
	
	ld	a,TACF_STOP|TACF_65KHZ
	ld	[rTAC],a
	ld	a,0
	ld	[rTMA],a
	ld	[rDIV],a
	ld	[rIF],a
	dec	a
	ld	[rTIMA],a
	
	ld	a,VALUE_WRITEN & $FF
	ld	[rDIV],a ; sync
	
	nop
	nop
	nop
	
	ld	[rTAC],a
	
	ld	a,[rTIMA]
	ld	[hl+],a
	ld	a,[rIF]
	and	a,4
	ld	[hl+],a

VALUE_WRITEN SET VALUE_WRITEN+1
	ENDR	

	; -------------------------------------------------------

	; -------------------------------------------------------
	
	; Write TAC: ENABLED, WHEN INC, FROM 00
	
VALUE_WRITEN SET 0
	REPT	8
	
	ld	a,TACF_START|TACF_65KHZ
	ld	[rTAC],a
	ld	a,0
	ld	[rTMA],a
	ld	[rDIV],a
	ld	[rIF],a
	ld	[rTIMA],a
	
	ld	a,VALUE_WRITEN & $FF
	ld	[rDIV],a ; sync
	
	DELAY_253
	
	ld	[rTAC],a ; write just when internal counter goes from 0x00FF to 0x0100
	
	ld	a,[rTIMA]
	ld	[hl+],a
	ld	a,[rIF]
	and	a,4
	ld	[hl+],a

VALUE_WRITEN SET VALUE_WRITEN+1
	ENDR	

	; -------------------------------------------------------
	
	; Write TAC: ENABLED, WHEN INC, FROM FF
	
VALUE_WRITEN SET 0
	REPT	8
	
	ld	a,TACF_START|TACF_65KHZ
	ld	[rTAC],a
	ld	a,0
	ld	[rTMA],a
	ld	[rDIV],a
	ld	[rIF],a
	dec	a
	ld	[rTIMA],a
	
	ld	a,VALUE_WRITEN & $FF
	ld	[rDIV],a ; sync
	
	DELAY_253
	
	ld	[rTAC],a ; write just when internal counter goes from 0x00FF to 0x0100
	
	ld	a,[rTIMA]
	ld	[hl+],a
	ld	a,[rIF]
	and	a,4
	ld	[hl+],a

VALUE_WRITEN SET VALUE_WRITEN+1
	ENDR	
	
	; -------------------------------------------------------
	
	; Write TAC: ENABLED, NOT WHEN INC, FROM 00
	
VALUE_WRITEN SET 0
	REPT	8
	
	ld	a,TACF_START|TACF_65KHZ
	ld	[rTAC],a
	ld	a,0
	ld	[rTMA],a
	ld	[rDIV],a
	ld	[rIF],a
	ld	[rTIMA],a
	
	ld	a,VALUE_WRITEN & $FF
	ld	[rDIV],a ; sync
	
	nop
	nop
	nop
	
	ld	[rTAC],a
	
	ld	a,[rTIMA]
	ld	[hl+],a
	ld	a,[rIF]
	and	a,4
	ld	[hl+],a

VALUE_WRITEN SET VALUE_WRITEN+1
	ENDR	

	; -------------------------------------------------------
	
	; Write TAC: ENABLED, NOT WHEN INC, FROM FF
	
VALUE_WRITEN SET 0
	REPT	8
	
	ld	a,TACF_START|TACF_65KHZ
	ld	[rTAC],a
	ld	a,0
	ld	[rTMA],a
	ld	[rDIV],a
	ld	[rIF],a
	dec	a
	ld	[rTIMA],a
	
	ld	a,VALUE_WRITEN & $FF
	ld	[rDIV],a ; sync
	
	nop
	nop
	nop
	
	ld	[rTAC],a
	
	ld	a,[rTIMA]
	ld	[hl+],a
	ld	a,[rIF]
	and	a,4
	ld	[hl+],a

VALUE_WRITEN SET VALUE_WRITEN+1
	ENDR	


	; -------------------------------------------------------

	; -------------------------------------------------------
	
	; -------------------------------------------------------

	; -------------------------------------------------------
	

	; -------------------------------------------------------
	
	; Write TAC: DISABLED, WHEN INC, FROM 00
	
VALUE_WRITEN SET 0
	REPT	8
	
	ld	a,TACF_STOP|TACF_16KHZ
	ld	[rTAC],a
	ld	a,0
	ld	[rTMA],a
	ld	[rDIV],a
	ld	[rIF],a
	ld	[rTIMA],a
	
	ld	a,VALUE_WRITEN & $FF
	ld	[rDIV],a ; sync
	
	DELAY_253
	
	ld	[rTAC],a ; write just when internal counter goes from 0x00FF to 0x0100
	
	ld	a,[rTIMA]
	ld	[hl+],a
	ld	a,[rIF]
	and	a,4
	ld	[hl+],a

VALUE_WRITEN SET VALUE_WRITEN+1
	ENDR	

	; -------------------------------------------------------
	
	; Write TAC: DISABLED, WHEN INC, FROM FF
	
VALUE_WRITEN SET 0
	REPT	8
	
	ld	a,TACF_STOP|TACF_16KHZ
	ld	[rTAC],a
	ld	a,0
	ld	[rTMA],a
	ld	[rDIV],a
	ld	[rIF],a
	dec	a
	ld	[rTIMA],a
	
	ld	a,VALUE_WRITEN & $FF
	ld	[rDIV],a ; sync
	
	DELAY_253
	
	ld	[rTAC],a ; write just when internal counter goes from 0x00FF to 0x0100
	
	ld	a,[rTIMA]
	ld	[hl+],a
	ld	a,[rIF]
	and	a,4
	ld	[hl+],a

VALUE_WRITEN SET VALUE_WRITEN+1
	ENDR	
	
	; -------------------------------------------------------
	
	; Write TAC: DISABLED, NOT WHEN INC, FROM 00
	
VALUE_WRITEN SET 0
	REPT	8
	
	ld	a,TACF_STOP|TACF_16KHZ
	ld	[rTAC],a
	ld	a,0
	ld	[rTMA],a
	ld	[rDIV],a
	ld	[rIF],a
	ld	[rTIMA],a
	
	ld	a,VALUE_WRITEN & $FF
	ld	[rDIV],a ; sync
	
	nop
	nop
	nop
	
	ld	[rTAC],a
	
	ld	a,[rTIMA]
	ld	[hl+],a
	ld	a,[rIF]
	and	a,4
	ld	[hl+],a

VALUE_WRITEN SET VALUE_WRITEN+1
	ENDR	

	; -------------------------------------------------------
	
	; Write TAC: DISABLED, NOT WHEN INC, FROM FF
	
VALUE_WRITEN SET 0
	REPT	8
	
	ld	a,TACF_STOP|TACF_16KHZ
	ld	[rTAC],a
	ld	a,0
	ld	[rTMA],a
	ld	[rDIV],a
	ld	[rIF],a
	dec	a
	ld	[rTIMA],a
	
	ld	a,VALUE_WRITEN & $FF
	ld	[rDIV],a ; sync
	
	nop
	nop
	nop
	
	ld	[rTAC],a
	
	ld	a,[rTIMA]
	ld	[hl+],a
	ld	a,[rIF]
	and	a,4
	ld	[hl+],a

VALUE_WRITEN SET VALUE_WRITEN+1
	ENDR	

	; -------------------------------------------------------

	; -------------------------------------------------------
	
	; Write TAC: ENABLED, WHEN INC, FROM 00
	
VALUE_WRITEN SET 0
	REPT	8
	
	ld	a,TACF_START|TACF_16KHZ
	ld	[rTAC],a
	ld	a,0
	ld	[rTMA],a
	ld	[rDIV],a
	ld	[rIF],a
	ld	[rTIMA],a
	
	ld	a,VALUE_WRITEN & $FF
	ld	[rDIV],a ; sync
	
	DELAY_253
	
	ld	[rTAC],a ; write just when internal counter goes from 0x00FF to 0x0100
	
	ld	a,[rTIMA]
	ld	[hl+],a
	ld	a,[rIF]
	and	a,4
	ld	[hl+],a

VALUE_WRITEN SET VALUE_WRITEN+1
	ENDR	

	; -------------------------------------------------------
	
	; Write TAC: ENABLED, WHEN INC, FROM FF
	
VALUE_WRITEN SET 0
	REPT	8
	
	ld	a,TACF_START|TACF_16KHZ
	ld	[rTAC],a
	ld	a,0
	ld	[rTMA],a
	ld	[rDIV],a
	ld	[rIF],a
	dec	a
	ld	[rTIMA],a
	
	ld	a,VALUE_WRITEN & $FF
	ld	[rDIV],a ; sync
	
	DELAY_253
	
	ld	[rTAC],a ; write just when internal counter goes from 0x00FF to 0x0100
	
	ld	a,[rTIMA]
	ld	[hl+],a
	ld	a,[rIF]
	and	a,4
	ld	[hl+],a

VALUE_WRITEN SET VALUE_WRITEN+1
	ENDR	
	
	; -------------------------------------------------------
	
	; Write TAC: ENABLED, NOT WHEN INC, FROM 00
	
VALUE_WRITEN SET 0
	REPT	8
	
	ld	a,TACF_START|TACF_16KHZ
	ld	[rTAC],a
	ld	a,0
	ld	[rTMA],a
	ld	[rDIV],a
	ld	[rIF],a
	ld	[rTIMA],a
	
	ld	a,VALUE_WRITEN & $FF
	ld	[rDIV],a ; sync
	
	nop
	nop
	nop
	
	ld	[rTAC],a
	
	ld	a,[rTIMA]
	ld	[hl+],a
	ld	a,[rIF]
	and	a,4
	ld	[hl+],a

VALUE_WRITEN SET VALUE_WRITEN+1
	ENDR	

	; -------------------------------------------------------
	
	; Write TAC: ENABLED, NOT WHEN INC, FROM FF
	
VALUE_WRITEN SET 0
	REPT	8
	
	ld	a,TACF_START|TACF_16KHZ
	ld	[rTAC],a
	ld	a,0
	ld	[rTMA],a
	ld	[rDIV],a
	ld	[rIF],a
	dec	a
	ld	[rTIMA],a
	
	ld	a,VALUE_WRITEN & $FF
	ld	[rDIV],a ; sync
	
	nop
	nop
	nop
	
	ld	[rTAC],a
	
	ld	a,[rTIMA]
	ld	[hl+],a
	ld	a,[rIF]
	and	a,4
	ld	[hl+],a

VALUE_WRITEN SET VALUE_WRITEN+1
	ENDR	


	; -------------------------------------------------------

	; -------------------------------------------------------
	
	; -------------------------------------------------------

	; -------------------------------------------------------
	

	; -------------------------------------------------------
	
	; Write TAC: DISABLED, WHEN INC, FROM 00
	
VALUE_WRITEN SET 0
	REPT	8
	
	ld	a,TACF_STOP|TACF_4KHZ
	ld	[rTAC],a
	ld	a,0
	ld	[rTMA],a
	ld	[rDIV],a
	ld	[rIF],a
	ld	[rTIMA],a
	
	ld	a,VALUE_WRITEN & $FF
	ld	[rDIV],a ; sync
	
	DELAY_253
	
	ld	[rTAC],a ; write just when internal counter goes from 0x00FF to 0x0100
	
	ld	a,[rTIMA]
	ld	[hl+],a
	ld	a,[rIF]
	and	a,4
	ld	[hl+],a

VALUE_WRITEN SET VALUE_WRITEN+1
	ENDR	

	; -------------------------------------------------------
	
	; Write TAC: DISABLED, WHEN INC, FROM FF
	
VALUE_WRITEN SET 0
	REPT	8
	
	ld	a,TACF_STOP|TACF_4KHZ
	ld	[rTAC],a
	ld	a,0
	ld	[rTMA],a
	ld	[rDIV],a
	ld	[rIF],a
	dec	a
	ld	[rTIMA],a
	
	ld	a,VALUE_WRITEN & $FF
	ld	[rDIV],a ; sync
	
	DELAY_253
	
	ld	[rTAC],a ; write just when internal counter goes from 0x00FF to 0x0100
	
	ld	a,[rTIMA]
	ld	[hl+],a
	ld	a,[rIF]
	and	a,4
	ld	[hl+],a

VALUE_WRITEN SET VALUE_WRITEN+1
	ENDR	
	
	; -------------------------------------------------------
	
	; Write TAC: DISABLED, NOT WHEN INC, FROM 00
	
VALUE_WRITEN SET 0
	REPT	8
	
	ld	a,TACF_STOP|TACF_4KHZ
	ld	[rTAC],a
	ld	a,0
	ld	[rTMA],a
	ld	[rDIV],a
	ld	[rIF],a
	ld	[rTIMA],a
	
	ld	a,VALUE_WRITEN & $FF
	ld	[rDIV],a ; sync
	
	nop
	nop
	nop
	
	ld	[rTAC],a
	
	ld	a,[rTIMA]
	ld	[hl+],a
	ld	a,[rIF]
	and	a,4
	ld	[hl+],a

VALUE_WRITEN SET VALUE_WRITEN+1
	ENDR	

	; -------------------------------------------------------
	
	; Write TAC: DISABLED, NOT WHEN INC, FROM FF
	
VALUE_WRITEN SET 0
	REPT	8
	
	ld	a,TACF_STOP|TACF_4KHZ
	ld	[rTAC],a
	ld	a,0
	ld	[rTMA],a
	ld	[rDIV],a
	ld	[rIF],a
	dec	a
	ld	[rTIMA],a
	
	ld	a,VALUE_WRITEN & $FF
	ld	[rDIV],a ; sync
	
	nop
	nop
	nop
	
	ld	[rTAC],a
	
	ld	a,[rTIMA]
	ld	[hl+],a
	ld	a,[rIF]
	and	a,4
	ld	[hl+],a

VALUE_WRITEN SET VALUE_WRITEN+1
	ENDR	

	; -------------------------------------------------------

	; -------------------------------------------------------
	
	; Write TAC: ENABLED, WHEN INC, FROM 00
	
VALUE_WRITEN SET 0
	REPT	8
	
	ld	a,TACF_START|TACF_4KHZ
	ld	[rTAC],a
	ld	a,0
	ld	[rTMA],a
	ld	[rDIV],a
	ld	[rIF],a
	ld	[rTIMA],a
	
	ld	a,VALUE_WRITEN & $FF
	ld	[rDIV],a ; sync
	
	DELAY_253
	
	ld	[rTAC],a ; write just when internal counter goes from 0x00FF to 0x0100
	
	ld	a,[rTIMA]
	ld	[hl+],a
	ld	a,[rIF]
	and	a,4
	ld	[hl+],a

VALUE_WRITEN SET VALUE_WRITEN+1
	ENDR	

	; -------------------------------------------------------
	
	; Write TAC: ENABLED, WHEN INC, FROM FF
	
VALUE_WRITEN SET 0
	REPT	8
	
	ld	a,TACF_START|TACF_4KHZ
	ld	[rTAC],a
	ld	a,0
	ld	[rTMA],a
	ld	[rDIV],a
	ld	[rIF],a
	dec	a
	ld	[rTIMA],a
	
	ld	a,VALUE_WRITEN & $FF
	ld	[rDIV],a ; sync
	
	DELAY_253
	
	ld	[rTAC],a ; write just when internal counter goes from 0x00FF to 0x0100
	
	ld	a,[rTIMA]
	ld	[hl+],a
	ld	a,[rIF]
	and	a,4
	ld	[hl+],a

VALUE_WRITEN SET VALUE_WRITEN+1
	ENDR	
	
	; -------------------------------------------------------
	
	; Write TAC: ENABLED, NOT WHEN INC, FROM 00
	
VALUE_WRITEN SET 0
	REPT	8
	
	ld	a,TACF_START|TACF_4KHZ
	ld	[rTAC],a
	ld	a,0
	ld	[rTMA],a
	ld	[rDIV],a
	ld	[rIF],a
	ld	[rTIMA],a
	
	ld	a,VALUE_WRITEN & $FF
	ld	[rDIV],a ; sync
	
	nop
	nop
	nop
	
	ld	[rTAC],a
	
	ld	a,[rTIMA]
	ld	[hl+],a
	ld	a,[rIF]
	and	a,4
	ld	[hl+],a

VALUE_WRITEN SET VALUE_WRITEN+1
	ENDR	

	; -------------------------------------------------------
	
	; Write TAC: ENABLED, NOT WHEN INC, FROM FF
	
VALUE_WRITEN SET 0
	REPT	8
	
	ld	a,TACF_START|TACF_4KHZ
	ld	[rTAC],a
	ld	a,0
	ld	[rTMA],a
	ld	[rDIV],a
	ld	[rIF],a
	dec	a
	ld	[rTIMA],a
	
	ld	a,VALUE_WRITEN & $FF
	ld	[rDIV],a ; sync
	
	nop
	nop
	nop
	
	ld	[rTAC],a
	
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
