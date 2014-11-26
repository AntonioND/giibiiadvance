
	INCLUDE	"hardware.inc"
	INCLUDE "header.inc"
	
SET_VALUES: MACRO
	db \1,\1+1,\1+2,\1+3,\1+4,\1+5,\1+6,\1+7,\1+8,\1+9,\1+10,\1+11,\1+12,\1+13,\1+14,\1+15
ENDM
	

	; $C000 4000 A000
	; $FE00 7E00 BE00
	; $FEA0 7EA0 BEA0
	; $FFE0 7FE0 BFE0
	
	
	SECTION	"11",HOME[$0000]
	SET_VALUES 0
	
	SECTION	"22",ROMX[$4000],BANK[1]
	SET_VALUES 1
	
	SECTION	"33",ROMX[$7E00],BANK[1]
	SET_VALUES 2
	
	SECTION	"44",ROMX[$7EA0],BANK[1]
	SET_VALUES 3
	
	SECTION	"55",ROMX[$7FE0],BANK[1]
	SET_VALUES 4


	SECTION	"Main",HOME

;--------------------------------------------------------------------------

mem_prepare: ; a = starting value, hl = ptr
	ld	b,16
.continue:
	ld	[hl+],a
	inc	a
	dec	b
	jr	nz,.continue
	ret
	
;--------------------------------------------------------------------------
;- Main()                                                                 -
;--------------------------------------------------------------------------


Main:

	di
	
	ld	a,$0A
	ld	[$0000],a ; enable ram
	
	ld	a,5
	ld	hl,$8000
	call	mem_prepare
	
	ld	a,6
	ld	hl,$A000
	call	mem_prepare
	
	ld	a,7
	ld	hl,$BE00
	call	mem_prepare

	ld	a,8
	ld	hl,$BEA0
	call	mem_prepare
	
	ld	a,9
	ld	hl,$BFE0
	call	mem_prepare

	ld	a,10
	ld	hl,$C000
	call	mem_prepare
	
	ld	a,11
	ld	hl,$FE00
	call	mem_prepare
	
	ld	a,12
	ld	hl,$FEA0
	call	mem_prepare
	
	ld	a,13
	ld	hl,$FFE0
	call	mem_prepare
	

	; -------------------------------------------------------
	
	DMA_COPY $0000,$9000,16,0
	DMA_COPY $4000,$9010,16,0
	DMA_COPY $8000,$9020,16,0
	DMA_COPY $A000,$9030,16,0
	DMA_COPY $C000,$9040,16,0
	DMA_COPY $E000,$9050,16,0
	DMA_COPY $FE00,$9060,16,0
	DMA_COPY $FEA0,$9070,16,0
	DMA_COPY $FFE0,$9080,16,0
	
	; -------------------------------------------------------
	
	ld	d,0
	ld	bc,$90
	ld	hl,$A000
	call	memset
	
	ld	bc,$90
	ld	hl,$9000
	ld	de,$A000
	call	memcopy
	
	ld	h,d ; get ending ptr
	ld	l,e
	
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
	
	; -------------------------------------------------------
	
	ld	a,$00
	ld	[$0000],a ; disable ram
	
.endloop:
	halt
	jr	.endloop
