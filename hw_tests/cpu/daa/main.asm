
	INCLUDE	"hardware.inc"
	INCLUDE "header.inc"
	
	SECTION	"Main",HOME

;--------------------------------------------------------------------------
;- Main()                                                                 -
;--------------------------------------------------------------------------

Main:
	
	ld	a,$0A
	ld	[$0000],a ; enable ram
	
	
	ld 	hl,$A000
	
	ld d,$0
.loopA:
	
		ld e,$00
		
.loopF:
	
			push de
			pop	 af
		
			daa
			
			push af
			pop	bc
			ld	[hl],b ; save A
			inc hl
			ld [hl],c ; save F
			inc hl			
		
			ld  a,e
			add a,$10
			ld	e,a
			cp 	a,$80
			
		jr nz,.loopF
	
	inc	d
	jr	nz,.loopA
	
	ld	a,$00
	ld	[$0000],a ; disable ram
	
	call	nz,NotColorGB
	
	ret

;--------------------------------------------------------------------------
;- NotColorGB()                                                           -
;--------------------------------------------------------------------------

NotColorGB:

	di
	
	
	
	ld	a,$01
	ld	[rIE],a
	
	ld 	a,0
	ld	hl,$0000
	call bg_set_palette
	
	ld	a,LCDCF_BGON|LCDCF_ON
	ld	[rLCDC],a
	
	ei
	
.loop:
	
	ld	a,[rBGP]
	cpl
	ld	[rBGP],a
	
	ld	e,60
	call	wait_frames
	
	jr	.loop
	
	ret

;--------------------------------------------------------------------------

;	SECTION	"Nops",HOME[$1000]

;	REPT	$3000
;	nop
;	ENDR

